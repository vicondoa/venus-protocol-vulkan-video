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

    def __init__(self, api, is_driver):
        self.api = copy.deepcopy(api)
        self.is_driver = is_driver
        self._fixup_api()

    def _set_type_attr(self, ty, key, val):
        ty = ty.base

        ty.attrs[key] = val
        for var in ty.variables:
            self._set_type_attr(var.ty, key, val)
        for next_ty in ty.p_next:
            self._set_type_attr(next_ty, key, val)

    def _set_type_needs(self, ty):
        for var in ty.variables:
            if var.ty.is_pointer() or var.ty.is_array():
                var.ty.base.attrs['need_array'] = True
                if var.ty.base.typedef:
                    var.ty.base.typedef.base.attrs['need_array'] = True

            if 'var_in' in var.attrs:
                if self.is_driver:
                    self._set_type_attr(var.ty, 'need_encode', True)
                else:
                    self._set_type_attr(var.ty, 'need_decode', True)

            if 'var_out' in var.attrs:
                if self.is_driver:
                    self._set_type_attr(var.ty, 'need_inout', True)
                    self._set_type_attr(var.ty, 'need_decode', True)
                else:
                    self._set_type_attr(var.ty, 'need_inout', True)
                    self._set_type_attr(var.ty, 'need_encode', True)

        if ty.ret:
            if ty.ret.is_pointer() or ty.ret.is_array():
                ty.ret.base.attrs['need_array'] = True
                if var.ty.base.typedef:
                    var.ty.base.typedef.base.attrs['need_array'] = True

            if self.is_driver:
                self._set_type_attr(var.ty, 'need_decode', True)
            else:
                self._set_type_attr(var.ty, 'need_encode', True)

    def _fixup_api(self):
        c_type_enums = self.api.type_table['VnCommandType'].enums

        for ty in self.api.type_table.values():
            if ty.category == ty.STRUCT:
                if ty.name == 'VkPhysicalDeviceProperties':
                    # work around a vk.xml bug
                    real_ty = self.api.type_table['VkPhysicalDeviceProperties2']
                    real_ty.p_next.extend(ty.p_next)
                    ty.p_next = []
            elif ty.category == ty.COMMAND:
                c_type = 'VN_COMMAND_TYPE_' + self.api.uppercase_name(ty)
                assert(c_type in c_type_enums.values)
                ty.attrs['c_type'] = c_type

                for var in ty.variables:
                    # non-const pointers are considerer outs
                    if var.ty.is_pointer() and not var.ty.is_const_pointer():
                        var.attrs['var_out'] = True
                    else:
                        var.attrs['var_in'] = True

                # outs appear in 'len' are in/out
                for var in ty.variables:
                    for name in var.attrs.get('len', []):
                        v = ty.find_variable(name)
                        if v:
                            v = v[0]
                        if v and 'var_out' in v.attrs:
                            v.attrs['var_in'] = var
                            v.attrs['var_out'] = var

            self._set_type_needs(ty)

    def _command_type(self, cmd):
        words = []
        begin = len('vk')
        end = begin + 1
        while end < len(cmd.name):
            if cmd.name[end].isupper():
                words.append(cmd.name[begin:end].upper())
                begin = end
                if cmd.name[begin:] in self.api.tags:
                    break
            end += 1
        words.append(cmd.name[begin:].upper())

        return 'VN_COMMAND_TYPE_' + '_'.join(words)

    def is_serializable(self, var):
        if isinstance(var, VkType):
            return self.is_serializable(VkVariable('', var, {}))

        ty = var.ty.base
        if ty.category == ty.BASETYPE:
            ty = ty.typedef

        # ignore platform extensions for now
        if ty.platforms:
            return False

        if ty.category in [ty.FUNCPOINTER]:
            return False

        if ty.category in [ty.HANDLE,
                           ty.ND_HANDLE,
                           ty.BITMASK,
                           ty.ENUM]:
            return True
        elif ty.category == ty.DEFINE:
            if ty.name in self.PRIMITIVE_TYPES:
                return True
            elif ty.name in ['char', 'size_t']:
                return True
            elif ty.name == 'void':
                return var.is_buffer()
            return False
        elif ty.category == ty.UNION:
            return ty.name in self.UNION_DEFAULT_TAGS

        assert(ty.category in [ty.STRUCT, ty.COMMAND])
        if ty.category == ty.STRUCT:
            if ty.name in ['VkBaseOutStructure', 'VkBaseInStructure']:
                return False
        elif ty.category == ty.COMMAND:
            if ty.ret and not self.is_serializable(ty.ret):
                return False

        for var in ty.variables:
            if var.maybe_null() or var.is_pnext():
                continue
            if not self.is_serializable(var):
                return False

        return True

    def get_pnext_chain(self, ty):
        types = []
        skipped = []
        for next_ty in ty.p_next:
            if self.is_serializable(next_ty):
                types.append(next_ty)
            else:
                skipped.append(next_ty)
        return (types, skipped)

    def _variable_func_info(self, ty, var):
        if var.is_string():
            func_name = 'string'
        elif var.is_buffer():
            func_name = 'blob'
        elif var.ty.base.category == var.ty.BITMASK:
            func_name = 'VkFlags'
        else:
            func_name = var.ty.base.name

        return func_name

    def _variable_loop_info(self, ty, var, prefix):
        loop_type = None
        loop_count = None
        if 'len' in var.attrs:
            # this is very limiting
            assert(len(var.attrs['len']) == 1)
            loop_name = var.attrs['len'][0]
            if 'altlen' in var.attrs:
                loop_count = var.attrs['altlen'][0]
            else:
                loop_count = loop_name

            loop_vars = ty.find_variable(loop_name)
            loop_type = loop_vars[-1].ty.base.name

            val = prefix + '->'.join([v.name for v in loop_vars])
            val = '*' * loop_vars[-1].ty.indirection_depth() + val
            loop_count = loop_count.replace(loop_name, val)
        elif var.ty.is_array():
            loop_type = 'uint32_t'
            loop_count = var.ty.array_size()

        return (loop_type, loop_count)

    def _variable_info(self, ty, var, prefix):
        func_name = self._variable_func_info(ty, var)
        loop_type, loop_count = self._variable_loop_info(ty, var, prefix)

        # check if we should unroll the loop
        if (loop_type and
            var.ty.base.category in [ty.DEFINE, ty.BASETYPE] and
            (var.ty.indirection_depth() + var.ty.is_array()) == 1):
            loop_type = None
            if not var.is_buffer():
                func_name += '_array'

        return (func_name, loop_type, loop_count)

    def _encode_variable_info(self, ty, var, prefix, is_inout):
        var_name = prefix + var.name
        func_name, loop_type, loop_count = self._variable_info(ty, var, prefix)
        if is_inout and var.ty.base.category == ty.STRUCT:
            func_name += '_inout'

        if_cond = None
        if var.ty.is_pointer():
            if_cond = 'vn_encode_pointer(cs, %s)' % var_name

        loop_cond = None
        if loop_type:
            loop_cond = '%s i = 0; i < %s; i++' % (loop_type, loop_count)

        deref_count = var.ty.indirection_depth() + var.ty.is_array() - 1
        func_args = var_name
        if loop_type:
            func_args += '[i]'
            deref_count -= 1
        elif loop_count:
            func_args += ', ' + loop_count

        deref = ''
        if deref_count > 0:
            deref = '*' * deref_count
        elif deref_count < 0:
            deref = '&' * -deref_count

        func_stmt = 'vn_encode_%s(cs, %s%s)' % (func_name, deref, func_args)

        return (if_cond, loop_cond, loop_count, func_stmt)

    def _decode_variable_info(self, ty, var, prefix, is_inout, alloc_storage):
        var_name = prefix + var.name
        func_name, loop_type, loop_count = self._variable_info(ty, var, prefix)
        if is_inout:
            if var.ty.base.category == ty.STRUCT:
                func_name += '_inout'
        else:
            if var.ty.base.category in [ty.HANDLE, ty.ND_HANDLE]:
                if not self.is_driver:
                    func_name += '_lookup'

        if_cond = None
        if var.ty.is_pointer():
            if_cond = 'vn_decode_pointer(cs)'

        loop_cond = None
        if loop_type:
            loop_cond = '%s i = 0; i < %s; i++' % (loop_type, loop_count)

        alloc_stmt = None
        simple_string = var.is_string() and not loop_type
        if alloc_storage and var.ty.is_pointer() and not simple_string:
            if var.is_buffer():
                alloc_size = loop_count
            else:
                alloc_size = 'sizeof(*%s)' % var_name
                if loop_count:
                    alloc_size += ' * ' + loop_count

            alloc_stmt = '%s = vn_cs_alloc_temp(cs, %s)' % (var_name, alloc_size)

        deref_count = var.ty.indirection_depth() + var.ty.is_array() - 1
        func_args = var_name
        if loop_type:
            func_args += '[i]'
            deref_count -= 1
        elif loop_count:
            func_args += ', ' + loop_count

        if alloc_storage and var.ty.base.category in [ty.STRUCT, ty.UNION]:
            func_name += '_temp'
        elif var.ty.base.category == ty.HANDLE and is_inout:
            func_name += '_temp'

        cast = ''
        if var.is_string():
            if alloc_storage:
                func_name += '_temp'
                deref_count -= 1
                cast = '(char **)'
        elif var.ty.is_const_pointer() or var.ty.is_const_array():
            cast = '(%s *)' % var.ty.base.name

        deref = ''
        if deref_count > 0:
            deref = '*' * deref_count
        elif deref_count < 0:
            deref = '&' * -deref_count

        func_stmt = 'vn_decode_%s(cs, %s%s%s)' % (func_name, cast, deref, func_args)

        return (if_cond, loop_cond, loop_count, alloc_stmt, func_stmt)

    def _replace_variable_handle_info(self, ty, var, prefix):
        var_name = prefix + var.name
        func_name, loop_type, loop_count = self._variable_info(ty, var, prefix)

        if_cond = None
        if var.ty.is_pointer():
            if_cond = var_name

        loop_cond = None
        if loop_type:
            loop_cond = '%s i = 0; i < %s; i++' % (loop_type, loop_count)

        cast = ''
        if var.is_string():
            if alloc_storage:
                func_name += '_temp'
                deref_count -= 1
                cast = '(char **)'
        elif var.ty.is_const_pointer() or var.ty.is_const_array():
            cast = '(%s *)' % var.ty.base.name

        deref_count = var.ty.indirection_depth() + var.ty.is_array() - 1
        func_args = var_name
        if loop_type:
            func_args += '[i]'
            deref_count -= 1
        elif loop_count:
            func_args += ', ' + loop_count

        deref = ''
        if deref_count > 0:
            deref = '*' * deref_count
        elif deref_count < 0:
            deref = '&' * -deref_count

        func_stmt = 'vn_replace_%s_handle(%s%s%s)' % (func_name, cast, deref, func_args)

        return (if_cond, loop_cond, loop_count, func_stmt)

    def _encode_variable(self, ty, var, prefix, is_inout):
        var_name = prefix + var.name
        if not self.is_serializable(var):
            if var.maybe_null():
                code = 'if (vn_encode_pointer(cs, %s))\n    ' % var_name
                code += '    assert(false);'
            else:
                assert(False)
            return code

        code = ''
        if_cond, loop_cond, loop_count, func_stmt = \
                self._encode_variable_info(ty, var, prefix, is_inout)

        if if_cond and loop_cond:
            code += 'if (%s) {\n    ' % if_cond
            code += '    vn_encode_uint64_t(cs, &(uint64_t){%s});\n    ' % loop_count
            code += '    for (%s)\n    ' % loop_cond
            code += '        %s;\n    ' % func_stmt
            code += '}'
        elif if_cond:
            code += 'if (%s)\n    ' % if_cond
            code += '    %s;' % func_stmt
        elif loop_cond:
            code += 'vn_encode_uint64_t(cs, &(uint64_t){%s});\n    ' % loop_count
            code += 'for (%s)\n    ' % loop_cond
            code += '    %s;' % func_stmt
        else:
            code += '%s;' % func_stmt

        return code

    def _decode_variable(self, ty, var, prefix, is_inout, alloc_storage):
        var_name = prefix + var.name
        if not self.is_serializable(var):
            if var.maybe_null():
                code = 'if (vn_decode_pointer(cs))\n    '
                code += '    assert(false);\n    '
                code += '%s = NULL;' % var_name
            else:
                assert(False)
            return code

        code = ''
        indent = ''
        if_cond, loop_cond, loop_count, alloc_stmt, func_stmt = \
                self._decode_variable_info(ty, var, prefix, is_inout, alloc_storage)

        if if_cond:
            code += 'if (%s) {\n    ' % if_cond
            indent += '    '

        if is_inout and var.ty.base.category not in [ty.STRUCT, ty.UNION, ty.HANDLE, ty.ND_HANDLE]:
            assert(alloc_stmt)
            code += '%s%s;\n    ' % (indent, alloc_stmt)
            code += '%sif (!%s) return;' % (indent, var_name)
        else:
            if alloc_stmt:
                code += '%s%s;\n    ' % (indent, alloc_stmt)
                code += '%sif (!%s) return;\n    ' % (indent, var_name)
            if loop_cond:
                code += '%svn_decode_uint64_t(cs, &(uint64_t){0});\n    ' % indent
                code += '%sfor (%s)\n    ' % (indent, loop_cond)
                code += '    %s%s;' % (indent, func_stmt)
            else:
                code += '%s%s;' % (indent, func_stmt)

        if if_cond:
            code += '\n    '
            code += '} else {\n    '
            code += '    %s = NULL;\n    ' % var_name
            code += '}'

        return code

    def _replace_variable_handle(self, ty, var, prefix, is_inout):
        var_name = prefix + var.name

        if is_inout or \
           var.ty.base.category not in [ty.HANDLE, ty.ND_HANDLE, ty.STRUCT] or \
           not self.is_serializable(var):
            return '/* skip %s */' % var_name

        code = ''
        if_cond, loop_cond, loop_count, func_stmt = \
                self._replace_variable_handle_info(ty, var, prefix)

        if if_cond and loop_cond:
            code += 'if (%s) {\n    ' % if_cond
            code += '    for (%s)\n    ' % loop_cond
            code += '        %s;\n    ' % func_stmt
            code += '}'
        elif if_cond:
            code += 'if (%s)\n    ' % if_cond
            code += '    %s;' % func_stmt
        elif loop_cond:
            code += 'for (%s)\n    ' % loop_cond
            code += '    %s;' % func_stmt
        else:
            code += '%s;' % func_stmt

        return code

    def encode_struct_member(self, ty, var, prefix):
        return self._encode_variable(ty, var, prefix, False)

    def decode_struct_member(self, ty, var, prefix, alloc_storage):
        return self._decode_variable(ty, var, prefix, False, alloc_storage)

    def replace_struct_member_handle(self, ty, var, prefix):
        return self._replace_variable_handle(ty, var, prefix, False)

    def encode_struct_inout(self, ty, var, prefix):
        if var.ty.base.category in [ty.HANDLE, ty.ND_HANDLE]:
            return self._encode_variable(ty, var, prefix, True)
        elif var.ty.base.category in [ty.STRUCT]:
            return self._encode_variable(ty, var, prefix, True)
        else:
            return '/* skip %s%s */' % (prefix, var.name)

    def decode_struct_inout(self, ty, var, prefix, alloc_storage):
        if var.ty.base.category in [ty.HANDLE, ty.ND_HANDLE]:
            return self._decode_variable(ty, var, prefix, True, alloc_storage)
        elif var.ty.base.category in [ty.STRUCT]:
            return self._decode_variable(ty, var, prefix, True, alloc_storage)
        else:
            return '/* skip %s%s */' % (prefix, var.name)

    def encode_command_arg(self, ty, var, prefix):
        if 'var_in' in var.attrs:
            return self._encode_variable(ty, var, prefix, False)
        elif var.ty.base.category in [ty.HANDLE, ty.ND_HANDLE]:
            return self._encode_variable(ty, var, prefix, True)
        elif var.ty.base.category in [ty.STRUCT, ty.UNION]:
            return self._encode_variable(ty, var, prefix, True)

        var_name = prefix + var.name
        if var.ty.is_pointer():
            return 'vn_encode_pointer(cs, %s); /* out */' % var_name
        else:
            return '(void)%s; /* out */' % var_name

    def decode_command_arg(self, ty, var, prefix):
        if 'var_in' in var.attrs:
            return self._decode_variable(ty, var, prefix, False, True)
        else:
            return self._decode_variable(ty, var, prefix, True, True)

    def replace_command_arg_handle(self, ty, var, prefix):
        if 'var_in' in var.attrs:
            return self._replace_variable_handle(ty, var, prefix, False)
        else:
            return self._replace_variable_handle(ty, var, prefix, True)

    def encode_command_reply(self, ty, var, prefix):
        if 'var_out' not in var.attrs:
            return '(void)%s%s; /* in */' % (prefix, var.name)
        return self._encode_variable(ty, var, prefix, False)

    def decode_command_reply(self, ty, var, prefix):
        if 'var_out' not in var.attrs:
            return '(void)%s%s; /* in */' % (prefix, var.name)
        return self._decode_variable(ty, var, prefix, False, False)

    def encode_command_ret(self, ty, ret_name, prefix):
        var = VkVariable(ret_name, ty.ret, {'var_out': True})
        return self.encode_command_reply(ty, var, prefix)

    def decode_command_ret(self, ty, ret_name, prefix):
        var = VkVariable(ret_name, ty.ret, {'var_out': True})
        return self.decode_command_reply(ty, var, prefix)

class GenCS(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.api = gen.api
        self.template = template

    def generate(self):
        return self.template.render()

class GenDefines(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.api = gen.api
        self.template = template

    def generate(self):
        typedef_types = []
        enum_types = []
        bitmask_types = []
        for ext in self.api.extensions:
            # venus only
            if ext.api != 'venus':
                continue
            for ty in ext.types:
                if ty.category == ty.BASETYPE:
                    typedef_types.append(ty)
                elif ty.category == ty.ENUM:
                    enum_types.append(ty)
                elif ty.category == ty.BITMASK:
                    bitmask_types.append(ty)

        command_types = []
        for ty in self.api.type_table.values():
            if ty.category != ty.COMMAND:
                continue
            if ty.platforms:
                continue

            if ty not in command_types:
                command_types.append(ty)

        return self.template.render(
                TYPEDEF_TYPES=typedef_types,
                ENUM_TYPES=enum_types,
                BITMASK_TYPES=bitmask_types,
                COMMAND_TYPES=command_types)

class GenCapset(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.api = gen.api
        self.template = template

    def generate(self):
        ext_table = []
        for ext in self.api.extensions:
            if ext.number >= len(ext_table):
                ext_table.extend([None] * (ext.number - len(ext_table) + 1))
            if ext.name in VK_XML_EXTENSION_LIST:
                ext_table[ext.number] = ext.name

        return self.template.render(
                WIRE_FORMAT_VERSION=VN_WIRE_FORMAT_VERSION,
                VN_XML_VERSION=self.api.vn_xml_version,
                VK_XML_VERSION=self.api.vk_xml_version,
                VK_XML_EXTENSION_TABLE=ext_table)

class GenTypes(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.api = gen.api
        self.template = template

    def generate(self):
        types = {
            VkType.DEFINE: [],
            VkType.BASETYPE: [],
            VkType.ENUM: [],
        }

        for ty in self.api.type_table.values():
            if ty.platforms:
                continue

            need = False
            if ty.category == ty.DEFINE:
                need = ty.name in self.gen.PRIMITIVE_TYPES
            elif ty.category == ty.ENUM:
                need = bool(ty.enums)
            elif ty.category in [ty.BASETYPE]:
                need = True

            if need and ty not in types[ty.category]:
                assert(self.gen.is_serializable(ty))
                types[ty.category].append(ty)

        scalar_types = [(ty, self.gen.PRIMITIVE_TYPES[ty.name])
                for ty in types[VkType.DEFINE]]

        return self.template.render(
                GEN=self.gen,
                SCALAR_TYPES=scalar_types,
                TYPEDEF_TYPES=types[VkType.BASETYPE],
                ENUM_TYPES=types[VkType.ENUM])

class GenHandles(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.api = gen.api
        self.template = template

    def generate(self):
        types = {
            VkType.HANDLE: [],
            VkType.ND_HANDLE: [],
        }

        for ty in self.api.type_table.values():
            if ty.platforms:
                continue

            need = False
            if ty.category in [ty.HANDLE, ty.ND_HANDLE]:
                need = True

            if need and ty not in types[ty.category]:
                assert(self.gen.is_serializable(ty))
                types[ty.category].append(ty)

        return self.template.render(
                GEN=self.gen,
                HANDLE_TYPES=types[VkType.HANDLE],
                ND_HANDLE_TYPES=types[VkType.ND_HANDLE])

class GenStructs(object):
    def __init__(self, gen, template):
        self.gen = gen
        self.api = gen.api
        self.template = template

        self.generated = set()
        self.structs = []
        self.manual_unions = []
        self.skipped = []
        for ty in self.api.type_table.values():
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
        self.api = gen.api
        self.template = template

    def generate(self):
        command_types, command_skipped = self.get_commands()
        return self.template.render(
                GEN=self.gen,
                COMMAND_TYPES=command_types,
                COMMAND_SKIPPED=command_skipped)

    def get_commands(self):
        types = []
        skipped = []
        for ty in self.api.type_table.values():
            if ty.category != ty.COMMAND:
                continue
            if ty in types or ty in skipped:
                continue

            if self.gen.is_serializable(ty):
                types.append(ty)
            else:
                skipped.append(ty)

        return (types, skipped)


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

    gen = Gen(api, not args.renderer)

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
