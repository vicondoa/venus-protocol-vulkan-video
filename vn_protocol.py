# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

import argparse
import copy
from pathlib import Path

from mako.lookup import TemplateLookup
from mako.template import Template

from vkxml import VkApi, VkType, VkVariable

# this is bumped whenever a backward-incompatible change is made
VN_WIRE_FORMAT_VERSION = 0

# list of supported extensions
VK_XML_EXTENSION_LIST = [
]

VN_PROTOCOL_DIR = Path(__file__).parent.resolve()
VK_XML = VN_PROTOCOL_DIR.joinpath('xml/vk.xml')
VN_XML = VN_PROTOCOL_DIR.joinpath('xml/vn.xml')

class Gen(object):
    TEMPLATE_DIR = VN_PROTOCOL_DIR.joinpath('templates')
    TEMPLATE_LOOKUP = TemplateLookup(str(TEMPLATE_DIR))

    PRIMITIVE_TYPES = {
        'float': 4,
        'double': 8,
        'uint8_t': 1,
        'uint16_t': 2,
        'uint32_t': 4,
        'uint64_t': 8,
        'int32_t': 4,
        'int64_t': 8,
    }

    UNION_DEFAULT_TAGS = {
        'VkClearColorValue': 2,
        'VkClearValue': 0,
        'VkPipelineExecutableStatisticValueKHR': 2,
    }

    def __init__(self, is_driver, api):
        self.is_driver = is_driver

        self.api = copy.deepcopy(api)
        self._fixup_api()

        self.supported_types = {}
        self._init_supported_types()

        # validate VnCommandType
        vn_command_type_ty = self.api.type_table['VnCommandType']
        for cmd in self.supported_types[VkType.COMMAND]:
            key = 'VN_COMMAND_TYPE_' + cmd.name
            assert(key in vn_command_type_ty.enums.values)
        assert(len(self.supported_types[VkType.COMMAND]) ==
                len(vn_command_type_ty.enums.values))

    def _set_type_needs(self, ty):
        for var in ty.variables:
            if var.ty.is_pointer() or var.ty.is_array():
                var.ty.base.attrs['need_array'] = True
                if var.ty.base.typedef:
                    var.ty.base.typedef.base.attrs['need_array'] = True

            if 'var_in' in var.attrs:
                if self.is_driver:
                    var.ty.set_attribute('need_encode', True)
                else:
                    var.ty.set_attribute('need_decode', True)

            if 'var_out' in var.attrs:
                if self.is_driver:
                    var.ty.set_attribute('need_partial', True)
                    var.ty.set_attribute('need_decode', True)
                else:
                    var.ty.set_attribute('need_partial', True)
                    var.ty.set_attribute('need_encode', True)

        if ty.ret:
            if ty.ret.ty.is_pointer() or ty.ret.ty.is_array():
                ty.ret.ty.base.attrs['need_array'] = True
                if ty.ret.ty.base.typedef:
                    ty.ret.ty.base.typedef.base.attrs['need_array'] = True

            if self.is_driver:
                var.ty.set_attribute('need_decode', True)
            else:
                var.ty.set_attribute('need_encode', True)

    def _fixup_api(self):
        for ty in self.api.type_table.values():
            if ty.category == ty.COMMAND:
                ty.attrs['c_type'] = 'VN_COMMAND_TYPE_' + ty.name

                if ty.ret:
                    ty.ret.attrs['var_out'] = True
                for var in ty.variables:
                    # non-const pointers are considerer outs
                    if var.ty.is_pointer() and not var.ty.is_const_pointer():
                        var.attrs['var_out'] = True
                    else:
                        var.attrs['var_in'] = True

                # outs appear in 'len_names' are in/out
                for var in ty.variables:
                    for name in var.attrs.get('len_names', []):
                        v = ty.find_variables(name)
                        if v:
                            v = v[0]
                        if v and 'var_out' in v.attrs:
                            v.attrs['var_in'] = var
                            v.attrs['var_out'] = var

            self._set_type_needs(ty)

    def _get_supported_types(self):
        # collect types from features and extensions
        types = []
        types.extend(self.api.venus.types)
        for feat in self.api.vulkan:
            types.extend(feat.types)
        for ext in self.api.extensions:
            if ext.name in VK_XML_EXTENSION_LIST:
                types.extend(ext.types)

        types_with_deps = set()
        for ty in types:
            if ty not in types_with_deps:
                types_with_deps.update(ty.get_dependencies())

        return types_with_deps

    def _init_supported_types(self):
        supported_types = self._get_supported_types()

        # filter p_next
        for ty in supported_types:
            p_next = []
            for tmp in ty.p_next:
                if tmp in supported_types:
                    p_next.append(tmp)
            ty.p_next = p_next

        # keep type_table order
        for ty in self.api.type_table.values():
            if ty not in supported_types:
                continue

            if ty.category not in self.supported_types:
                self.supported_types[ty.category] = []
            if ty not in self.supported_types[ty.category]:
                self.supported_types[ty.category].append(ty)

    def is_serializable(self, var):
        if isinstance(var, VkType):
            return self.is_serializable(VkVariable(var))

        ty = var.ty.base
        if ty.category == ty.BASETYPE:
            if not ty.typedef:
                return False
            ty = ty.typedef

        if ty.category in [ty.INCLUDE, ty.DEFINE, ty.FUNCPOINTER]:
            return False
        elif ty.category == ty.DEFAULT:
            if ty.name in self.PRIMITIVE_TYPES:
                return True
            elif ty.name in ['char', 'size_t']:
                return True
            elif ty.name == 'void':
                return var.is_data()
            return False
        elif ty.category in [ty.HANDLE, ty.ENUM, ty.BITMASK]:
            return True
        elif ty.category == ty.UNION:
            return ty.name in self.UNION_DEFAULT_TAGS

        assert(ty.category in [ty.STRUCT, ty.COMMAND])
        if ty.category == ty.STRUCT:
            if ty.name in ['VkBaseInStructure', 'VkBaseOutStructure']:
                return False
        elif ty.category == ty.COMMAND:
            if ty.ret and not self.is_serializable(ty.ret):
                return False

        for var in ty.variables:
            if var.maybe_null() or var.is_p_next():
                continue
            if not self.is_serializable(var):
                return False

        return True

    def get_chain(self, ty):
        types = []
        skipped = []
        for next_ty in ty.p_next:
            if self.is_serializable(next_ty):
                types.append(next_ty)
            else:
                skipped.append(next_ty)
        return (types, skipped)

    class VariableInfo(object):
        def __init__(self, ty, var, prefix):
            self.ty = ty
            self.var = var
            self.prefix = prefix

            self.func_stem = None
            self.loop_types = []
            self.loop_counts = []
            self.array_size = None

            self._init_func_stem()
            self._init_loop_info()
            self._unroll_loop()

            self.func_stmt = None
            self.alloc_stmts = []

        def _init_func_stem(self):
            if self.var.is_data() or self.var.is_string():
                self.func_stem = 'data'
            elif self.var.ty.base.category == VkType.BITMASK:
                self.func_stem = 'VkFlags'
            else:
                self.func_stem = self.var.ty.base.name

        def _init_loop_info(self):
            if 'len_exprs' not in self.var.attrs:
                if self.var.ty.is_array():
                    self.loop_types.append('uint32_t')
                    self.loop_counts.append(self.var.ty.array_size())
                return

            len_exprs = self.var.attrs['len_exprs']
            len_names = self.var.attrs['len_names']
            for expr, name in zip(len_exprs, len_names):
                if expr == 'null-terminated':
                    loop_ty = self.ty.find_variables(name)[-1].ty
                    suffix = ''
                    if loop_ty.indirection_depth() > 1:
                        suffix = '[%c]' % (chr(ord('i') + loop_ty.indirection_depth() - 2))

                    self.loop_types.append('size_t')
                    self.loop_counts.append('strlen(%s%s%s) + 1' % (self.prefix, name, suffix))
                elif name:
                    loop_ty = self.ty.find_variables(name)[-1].ty
                    deref = '*' * loop_ty.indirection_depth()
                    count = expr.replace(name, deref + self.prefix + name)

                    self.loop_types.append(loop_ty.base.name)
                    self.loop_counts.append(count)
                else:
                    self.loop_types.append('uint32_t')
                    self.loop_counts.append(expr)

        def _unroll_loop(self):
            if not self.loop_types:
                return

            # unroll loops for scalar arrays to get padding right
            scalar_categories = [VkType.DEFAULT, VkType.BASETYPE, VkType.ENUM]
            if self.var.ty.base.category in scalar_categories:
                self.func_stem += '_array'
                self.loop_types.pop()
                self.array_size = self.loop_counts.pop()

        def init_alloc_stmts(self):
            var_name = self.prefix + self.var.name

            alloc_counts = self.loop_counts
            if self.array_size or not alloc_counts:
                alloc_counts.append(self.array_size)

            for level, count in enumerate(alloc_counts):
                if self.var.is_data():
                    size = count
                else:
                    size = 'sizeof(%s%s)' % ('*' * (level + 1), var_name)
                    if count:
                        size += ' * ' + count

                suffix = ''
                for i in range(level):
                    suffix += '[%c]' % (chr(ord('i') + i))
                if suffix:
                    stmt = '((void **)%s)%s = vn_cs_alloc_temp(cs, %s)' % (var_name, suffix, size)
                else:
                    stmt = '%s = vn_cs_alloc_temp(cs, %s)' % (var_name, size)
                self.alloc_stmts.append(stmt)

        def args(self, const_cast):
            var_name = self.prefix + self.var.name

            deref_count = self.var.ty.indirection_depth() + self.var.ty.is_array() - 1
            for i in range(len(self.loop_types)):
                var_name += '[%c]' % chr(ord('i') + i)
                deref_count -= 1

            deref = ''
            if deref_count > 0:
                deref = '*' * deref_count
            elif deref_count < 0:
                deref = '&' * -deref_count

            cast = ''
            if const_cast and (self.var.ty.is_const_pointer() or self.var.ty.is_const_array()):
                cast = '(%s *)' % self.var.ty.base.name

            args = '%s%s%s' % (cast, deref, var_name)
            if self.array_size:
                args += ', ' + self.array_size

            return args

    def _encode_variable_info(self, ty, var, prefix, is_out):
        info = self.VariableInfo(ty, var, prefix)

        if not self.is_serializable(var):
            assert(var.maybe_null())
            info.func_stmt = 'assert(false)'
            return info

        func_name = 'vn_encode_' + info.func_stem
        if is_out and var.ty.base.category == ty.STRUCT:
            func_name += '_partial'

        info.func_stmt = '%s(cs, %s)' % (func_name, info.args(False))

        return info

    def _encode_variable(self, ty, var, prefix, is_out):
        var_name = prefix + var.name

        partially_initialized = [ty.HANDLE, ty.STRUCT]
        if is_out and var.ty.base.category not in partially_initialized:
            if var.ty.is_pointer():
                return 'vn_encode_pointer(cs, %s); /* out */' % var_name
            else:
                return '/* skip %s */' % var_name

        info = self._encode_variable_info(ty, var, prefix, is_out)
        loop_type = info.loop_types[0] if info.loop_types else None
        loop_count = info.loop_counts[0] if info.loop_counts else None

        code = ''
        if var.ty.is_pointer():
            code += 'if (vn_encode_pointer(cs, %s)) ' % var_name
            if loop_type:
                code += '{ '
        if loop_type:
            code += 'vn_encode_array_size(cs, %s); for (%s i = 0; i < %s; i++) ' % (loop_count, loop_type, loop_count)
        code += '%s;' % info.func_stmt
        if var.ty.is_pointer() and loop_type:
            code += ' }'

        return code

    def _decode_variable_info(self, ty, var, prefix, is_out, alloc_storage):
        info = self.VariableInfo(ty, var, prefix)

        if not self.is_serializable(var):
            assert(var.maybe_null())
            info.func_stmt = 'assert(false)'
            return info

        if var.is_string() and info.array_size:
            info.array_size = 'vn_peek_array_size(cs)'

        if alloc_storage and var.ty.is_pointer():
            info.init_alloc_stmts()

        func_name = 'vn_decode_' + info.func_stem

        # automatic handle lookup
        if not self.is_driver:
            if var.ty.base.category == ty.HANDLE and not is_out:
                func_name += '_lookup'

        if var.ty.base.category == ty.STRUCT and is_out:
            func_name += '_partial'

        if alloc_storage:
            if var.ty.base.category in [ty.STRUCT, ty.UNION]:
                func_name += '_temp'
            elif var.ty.base.category == ty.HANDLE and var.ty.base.dispatchable and is_out:
                func_name += '_temp'

        info.func_stmt = '%s(cs, %s)' % (func_name, info.args(True))

        return info

    def _decode_variable(self, ty, var, prefix, is_out, alloc_storage):
        var_name = prefix + var.name

        partially_initialized = [ty.HANDLE, ty.STRUCT]
        if is_out and var.ty.base.category not in partially_initialized:
            if alloc_storage and var.ty.is_pointer():
                # we still need to allocate the storage
                pass
            else:
                return '/* skip %s%s */' % (prefix, var.name)

        info = self._decode_variable_info(ty, var, prefix, is_out, alloc_storage)
        loop_type = info.loop_types[0] if info.loop_types else None
        loop_count = info.loop_counts[0] if info.loop_counts else None

        code = ''
        indent = ''
        if var.ty.is_pointer():
            code += 'if (vn_decode_pointer(cs)) {\n    '
            indent += '    '

        if is_out and var.ty.base.category not in partially_initialized:
            assert(len(info.alloc_stmts) == 1)
            code += '%s%s;\n    ' % (indent, info.alloc_stmts[0])
            code += '%sif (!%s) return;' % (indent, var_name)
        elif info.alloc_stmts and len(info.alloc_stmts) > 1:
            for level, alloc_stmt in enumerate(info.alloc_stmts):
                if alloc_stmt:
                    code += '%s%s;\n    ' % (indent, alloc_stmt)
                    code += '%sif (!%s) return;\n    ' % (indent, var_name)
                if level == 0 and loop_count:
                    code += '%svn_decode_array_size(cs, %s);\n    ' % (indent, loop_count)
                    code += '%sfor (uint32_t i = 0; i < %s; i++) {\n    ' % (indent, loop_count)
                    indent += '    '
            code += '%s%s;' % (indent, info.func_stmt)
            if loop_count:
                code += '\n        }'
        else:
            if info.alloc_stmts:
                code += '%s%s;\n    ' % (indent, info.alloc_stmts[0])
                code += '%sif (!%s) return;\n    ' % (indent, var_name)
            if loop_type:
                code += '%svn_decode_array_size(cs, %s);\n    ' % (indent, loop_count)
                code += '%sfor (uint32_t i = 0; i < %s; i++)\n    ' % (indent, loop_count)
                indent += '    '
            code += '%s%s;' % (indent, info.func_stmt)

        if var.ty.is_pointer():
            code += '\n    '
            code += '} else {\n    '
            code += '    %s = NULL;\n    ' % var_name
            code += '}'

        return code

    def _replace_variable_handle_info(self, ty, var, prefix):
        info = self.VariableInfo(ty, var, prefix)
        info.func_stmt = 'vn_replace_%s_handle(%s)' % (
                info.func_stem, info.args(True))
        return info

    def _replace_variable_handle(self, ty, var, prefix, is_out):
        var_name = prefix + var.name

        might_contain_handle = [ty.HANDLE, ty.STRUCT]
        if is_out or var.ty.base.category not in might_contain_handle or \
           not self.is_serializable(var):
            return '/* skip %s */' % var_name

        info = self._replace_variable_handle_info(ty, var, prefix)
        loop_type = info.loop_types[0] if info.loop_types else None
        loop_count = info.loop_counts[0] if info.loop_counts else None

        code = ''
        if var.ty.is_pointer():
            code += 'if (%s) ' % var_name
        if loop_type:
            code += 'for (%s i = 0; i < %s; i++) ' % (loop_type, loop_count)
        code += '%s;' % info.func_stmt

        return code

    def encode_struct_member(self, ty, var, prefix, is_out):
        return self._encode_variable(ty, var, prefix, is_out)

    def decode_struct_member(self, ty, var, prefix, is_out, alloc_storage):
        return self._decode_variable(ty, var, prefix, is_out, alloc_storage)

    def replace_struct_member_handle(self, ty, var, prefix):
        return self._replace_variable_handle(ty, var, prefix, False)

    def encode_command_arg(self, ty, var, prefix):
        is_out = 'var_in' not in var.attrs
        return self._encode_variable(ty, var, prefix, is_out)

    def decode_command_arg(self, ty, var, prefix):
        is_out = 'var_in' not in var.attrs
        return self._decode_variable(ty, var, prefix, is_out, True)

    def replace_command_arg_handle(self, ty, var, prefix):
        is_out = 'var_in' not in var.attrs
        return self._replace_variable_handle(ty, var, prefix, is_out)

    def encode_command_reply(self, ty, var, prefix):
        if 'var_out' not in var.attrs:
            return '/* skip %s%s */' % (prefix, var.name)
        return self._encode_variable(ty, var, prefix, False)

    def decode_command_reply(self, ty, var, prefix):
        if 'var_out' not in var.attrs:
            return '/* skip %s%s */' % (prefix, var.name)
        return self._decode_variable(ty, var, prefix, False, False)

    def encode_command_ret(self, ty, prefix):
        return self._encode_variable(ty, ty.ret, prefix, False)

    def decode_command_ret(self, ty, prefix):
        return self._decode_variable(ty, ty.ret, prefix, False, False)

class GenCS(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

    def generate(self):
        return self.template.render()

class GenDefines(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

    def generate(self):
        # venus only
        typedef_types = []
        enum_types = []
        bitmask_types = []
        for ty in self.gen.api.venus.types:
            if ty.category == ty.BASETYPE and ty.typedef:
                typedef_types.append(ty)
            elif ty.category == ty.ENUM:
                enum_types.append(ty)
            elif ty.category == ty.BITMASK:
                bitmask_types.append(ty)

        command_types = self.gen.supported_types[VkType.COMMAND]

        return self.template.render(
                TYPEDEF_TYPES=typedef_types,
                ENUM_TYPES=enum_types,
                BITMASK_TYPES=bitmask_types,
                COMMAND_TYPES=command_types)

class GenCapset(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

    def generate(self):
        ext_table = []
        for ext in self.gen.api.extensions:
            if ext.number >= len(ext_table):
                ext_table.extend([None] * (ext.number - len(ext_table) + 1))
            if ext.name in VK_XML_EXTENSION_LIST:
                ext_table[ext.number] = ext.name

        return self.template.render(
                WIRE_FORMAT_VERSION=VN_WIRE_FORMAT_VERSION,
                VN_XML_VERSION=self.gen.api.vn_xml_version,
                VK_XML_VERSION=self.gen.api.vk_xml_version,
                VK_XML_EXTENSION_TABLE=ext_table)

class GenTypes(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

    def generate(self):
        early_scalar_types = []
        scalar_types = []
        for ty in self.gen.supported_types[VkType.DEFAULT]:
            if ty.name in self.gen.PRIMITIVE_TYPES:
                if ty.name in ['uint64_t', 'int32_t']:
                    early_scalar_types.append(ty)
                scalar_types.append(ty)
        for ty in self.gen.supported_types[VkType.BASETYPE]:
            if ty.typedef and self.gen.is_serializable(ty.typedef):
                scalar_types.append(ty)
        for ty in self.gen.supported_types[VkType.ENUM]:
            if ty.enums.values:
                if ty.name == 'VkStructureType':
                    early_scalar_types.append(ty)
                scalar_types.append(ty)

        return self.template.render(
                GEN=self.gen,
                EARLY_SCALAR_TYPES=early_scalar_types,
                SCALAR_TYPES=scalar_types)

class GenHandles(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

    def generate(self):
        handle_types = self.gen.supported_types[VkType.HANDLE]
        return self.template.render(
                GEN=self.gen,
                HANDLE_TYPES=handle_types)

class GenStructs(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

        self.generated = set()
        self.structs = []
        self.manual_unions = []
        self.skipped = []
        for ty in self.gen.supported_types[VkType.STRUCT]:
            self._add_struct(ty)
        for ty in self.gen.supported_types[VkType.UNION]:
            self._add_struct(ty)

    def _add_struct(self, ty):
        # both structs and unions
        if ty.category not in [ty.STRUCT, ty.UNION]:
            return
        if ty in self.generated:
            return
        self.generated.add(ty)

        if self.gen.is_serializable(ty):
            # add dependencies first
            deps = [var.ty.base for var in ty.variables] + ty.p_next
            for dep in deps:
                self._add_struct(dep)
            self.structs.append(ty)
        elif ty.category == ty.UNION:
            can_manual = True
            for var in ty.variables:
                if not self.gen.is_serializable(var):
                    can_manual = False
                    break

            if can_manual:
                self.manual_unions.append(ty)
            else:
                self.skipped.append(ty)
        else:
            self.skipped.append(ty)

    def generate(self):
        return self.template.render(
                GEN=self.gen,
                STRUCT_TYPES=self.structs,
                STRUCT_SKIPPED=self.skipped,
                MANUAL_UNION_TYPES=self.manual_unions)

class GenCommands(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.template = template

    def generate(self):
        types = []
        skipped = []
        for ty in self.gen.supported_types[VkType.COMMAND]:
            if self.gen.is_serializable(ty):
                types.append(ty)
            else:
                skipped.append(ty)

        return self.template.render(
                GEN=self.gen,
                COMMAND_TABLE_SIZE=self.gen.api.max_vn_command_type_value + 1,
                COMMAND_TYPES=types,
                COMMAND_SKIPPED=skipped)

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--outdir', help='Where to write the files.',
                        required=True)
    parser.add_argument('--renderer',
                        help='Generate for the renderer.',
                        action='store_true')
    return parser.parse_args()

def main():
    args = get_args()

    api = VkApi()
    api.parse_xml(VK_XML)
    api.parse_xml(VN_XML)
    api.validate()

    gen = Gen(not args.renderer, api)

    if gen.is_driver:
        outputs = [
            (GenCS,         'driver_cs.h'),
            (GenDefines,    'driver_defines.h'),
            (GenCapset,     'driver_capset.h'),
            (GenTypes,      'driver_types.h'),
            (GenHandles,    'driver_handles.h'),
            (GenStructs,    'driver_structs.h'),
            (GenCommands,   'driver_commands.h'),
            (GenCommands,   'driver_calls.h'),
        ]
    else:
        outputs = [
            (GenCS,         'renderer_cs.h'),
            (GenDefines,    'renderer_defines.h'),
            (GenCapset,     'renderer_capset.h'),
            (GenTypes,      'renderer_types.h'),
            (GenHandles,    'renderer_handles.h'),
            (GenStructs,    'renderer_structs.h'),
            (GenCommands,   'renderer_commands.h'),
            (GenCommands,   'renderer_dispatches.h'),
        ]

    for generator_cls, filename in outputs:
        template = Template(
            filename=str(gen.TEMPLATE_DIR.joinpath(filename)),
            lookup=gen.TEMPLATE_LOOKUP, output_encoding='utf-8')
        generator = generator_cls(gen, template)

        output = Path(args.outdir).joinpath('vn_protocol_' + filename)
        with open(output, 'wb') as f:
            f.write(generator.generate())

    # generate a header that includes all other headers
    filename = 'driver.h' if gen.is_driver else 'renderer.h'
    template = Template(
        filename=str(gen.TEMPLATE_DIR.joinpath(filename)),
        lookup=gen.TEMPLATE_LOOKUP, output_encoding='utf-8')
    output = Path(args.outdir).joinpath('vn_protocol_' + filename)
    with open(output, 'wb') as f:
        template_filenames = [out[1] for out in outputs]
        f.write(template.render(TEMPLATE_FILENAMES=template_filenames))

if __name__ == '__main__':
    main()
