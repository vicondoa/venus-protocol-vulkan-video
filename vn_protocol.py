#!/usr/bin/env python3

# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

import argparse
import copy
from pathlib import Path

from mako.lookup import TemplateLookup
from mako.template import Template

from vkxml import VkApi, VkType, VkVariable

VN_PROTOCOL_DIR = Path(__file__).resolve().parent
VN_TEMPLATE_DIR = VN_PROTOCOL_DIR.joinpath('templates')
VN_TEMPLATE_LOOKUP = TemplateLookup(str(VN_TEMPLATE_DIR))

VN_PROTOCOL_XMLS = [
    VN_PROTOCOL_DIR.joinpath('xmls/vk.xml'),
    VN_PROTOCOL_DIR.joinpath('xmls/VK_EXT_command_serialization.xml'),
    VN_PROTOCOL_DIR.joinpath('xmls/VK_MESA_venus_protocol.xml'),
]

# this is bumped whenever a backward-incompatible change is made
VN_WIRE_FORMAT_VERSION = 0

# list of supported extensions
VK_XML_EXTENSION_LIST = [
    # Venus extensions
    'VK_EXT_command_serialization',
    'VK_MESA_venus_protocol',
    # promoted to VK_VERSION_1_1
    'VK_KHR_16bit_storage',
    'VK_KHR_bind_memory2',
    'VK_KHR_dedicated_allocation',
    'VK_KHR_descriptor_update_template',
    'VK_KHR_device_group',
    'VK_KHR_device_group_creation',
    'VK_KHR_external_fence',
    'VK_KHR_external_fence_capabilities',
    'VK_KHR_external_memory',
    'VK_KHR_external_memory_capabilities',
    'VK_KHR_external_semaphore',
    'VK_KHR_external_semaphore_capabilities',
    'VK_KHR_get_memory_requirements2',
    'VK_KHR_get_physical_device_properties2',
    'VK_KHR_maintenance1',
    'VK_KHR_maintenance2',
    'VK_KHR_maintenance3',
    'VK_KHR_multiview',
    'VK_KHR_relaxed_block_layout',
    'VK_KHR_sampler_ycbcr_conversion',
    'VK_KHR_shader_draw_parameters',
    'VK_KHR_storage_buffer_storage_class',
    'VK_KHR_variable_pointers',
    # promoted to VK_VERSION_1_2
    'VK_KHR_8bit_storage',
    'VK_KHR_buffer_device_address',
    'VK_KHR_create_renderpass2',
    'VK_KHR_depth_stencil_resolve',
    'VK_KHR_draw_indirect_count',
    'VK_KHR_driver_properties',
    'VK_KHR_image_format_list',
    'VK_KHR_imageless_framebuffer',
    'VK_KHR_sampler_mirror_clamp_to_edge',
    'VK_KHR_separate_depth_stencil_layouts',
    'VK_KHR_shader_atomic_int64',
    'VK_KHR_shader_float16_int8',
    'VK_KHR_shader_float_controls',
    'VK_KHR_shader_subgroup_extended_types',
    'VK_KHR_spirv_1_4',
    'VK_KHR_timeline_semaphore',
    'VK_KHR_uniform_buffer_standard_layout',
    'VK_KHR_vulkan_memory_model',
    'VK_EXT_descriptor_indexing',
    'VK_EXT_host_query_reset',
    'VK_EXT_sampler_filter_minmax',
    'VK_EXT_scalar_block_layout',
    'VK_EXT_separate_stencil_usage',
    'VK_EXT_shader_viewport_index_layer',
    # KHR extensions
    'VK_KHR_external_memory_fd',
    # EXT extensions
    'VK_EXT_external_memory_dma_buf',
    'VK_EXT_image_drm_format_modifier',
    'VK_EXT_queue_family_foreign',
    'VK_EXT_transform_feedback',
]

class Gen:
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

        # validate VkCommandTypeEXT
        command_type_ty = self.api.type_table['VkCommandTypeEXT']
        enum_value_count = 0
        for cmd in self.supported_types[VkType.COMMAND]:
            key = 'VK_COMMAND_TYPE_' + cmd.name + '_EXT'
            if key not in command_type_ty.enums.values:
                raise KeyError('%s not defined for %s' % (key, command_type_ty.name))
            for alias in cmd.aliases:
                key = 'VK_COMMAND_TYPE_' + alias + '_EXT'
                if key not in command_type_ty.enums.values:
                    raise KeyError('%s not defined for %s' % (key, command_type_ty.name))
            enum_value_count += 1 + len(cmd.aliases)
        assert enum_value_count == len(command_type_ty.enums.values)

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
                ty.attrs['c_type'] = 'VK_COMMAND_TYPE_' + ty.name + '_EXT'

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
            elif ty.category == ty.HANDLE:
                objtype = 'VK_OBJECT_TYPE_' + self.api.upper_name(ty.name[2:])
                ty.attrs['c_objtype'] = objtype

            self._set_type_needs(ty)

    def _get_supported_types(self):
        # collect types from features and extensions
        types = []
        for feat in self.api.features:
            types.extend(feat.types)
        for ext in self.api.extensions:
            if ext.name not in VK_XML_EXTENSION_LIST:
                continue

            types.extend(ext.types)
            for key in ext.optional_types:
                if key in VK_XML_EXTENSION_LIST:
                    types.extend(ext.optional_types[key])

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
                return var.is_blob()
            return False
        elif ty.category in [ty.HANDLE, ty.ENUM, ty.BITMASK]:
            return True
        elif ty.category == ty.UNION:
            return ty.name in self.UNION_DEFAULT_TAGS

        assert ty.category in [ty.STRUCT, ty.COMMAND]
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
        return types, skipped

    class VariableInfo:
        VALID = 0
        INVALID = 1
        PARTIAL = 2

        def __init__(self, ty, var, prefix, validity):
            self.ty = ty
            self.var = var
            self.prefix = prefix
            self.validity = validity

            self.func_stem = None
            self._init_func_stem()

            self.loop_level = None
            self.loop_types = []
            self.loop_indices = []
            self.loop_counts = []
            self._init_loop_info()

            self.array_size = None
            self._unroll_loop()

            self.loop_alloc_stmts = []
            self.loop_extra_stmts = []
            self.func_array_size_stmt = None
            self.func_alloc_stmt = None
            self.func_extra_stmt = None
            self.func_stmt = None

        def _init_func_stem(self):
            if self.var.is_blob() or self.var.ty.base.name == 'char':
                self.func_stem = 'blob'
            elif self.var.ty.base.category == VkType.BITMASK:
                self.func_stem = self.var.ty.base.typedef.name
            else:
                self.func_stem = self.var.ty.base.name

        def _var_deref(self, var=None):
            if not var:
                var = self.var
            return '*' * (var.ty.indirection_depth() + var.ty.is_array())

        def _var_const_cast(self):
            return '(%s %s)' % (self.var.ty.base.name, self._var_deref())

        def _var_loop_indices(self, loop_level):
            if loop_level:
                return '[' + ']['.join(self.loop_indices[:loop_level]) + ']'
            else:
                return ''

        def _var_name(self, loop_level=0, const_cast=False):
            var_name = self.prefix + self.var.name
            indices = self._var_loop_indices(loop_level)
            if const_cast and (self.var.ty.is_const_pointer() or
                    self.var.ty.is_const_array()):
                if indices:
                    return '(%s%s)%s' % (self._var_const_cast(), var_name, indices)
                else:
                    return '%s%s' % (self._var_const_cast(), var_name)
            else:
                return '%s%s' % (var_name, indices)

        def _init_loop_info(self):
            if 'len_exprs' not in self.var.attrs:
                if self.var.ty.is_array():
                    self.loop_level = 1
                    self.loop_types.append('uint32_t')
                    self.loop_indices.append('i')
                    self.loop_counts.append(self.var.ty.array_size())
                else:
                    self.loop_level = 0
                return

            len_exprs = self.var.attrs['len_exprs']
            len_names = self.var.attrs['len_names']
            self.loop_level = len(len_exprs)
            self.loop_indices = [chr(ord('i') + i) for i in range(self.loop_level)]
            for level, (expr, name) in enumerate(zip(len_exprs, len_names)):
                if expr == 'null-terminated':
                    loop_type = 'size_t'
                    loop_count = 'strlen(%s) + 1' % self._var_name(level)
                elif name:
                    loop_vars = self.ty.find_variables(name)

                    deref = self._var_deref(loop_vars[-1])
                    loop_type = loop_vars[-1].ty.base.name
                    loop_count = expr.replace(name, deref + self.prefix + name)

                    assert len(loop_vars) <= 2
                    if len(loop_vars) > 1 or loop_vars[-1].ty.is_pointer():
                        loop_count = '(%s%s ? %s : 0)' % (self.prefix,
                                loop_vars[0].name, loop_count)
                else:
                    loop_type = 'uint32_t'
                    loop_count = expr

                self.loop_types.append(loop_type)
                self.loop_counts.append(loop_count)

        def _unroll_loop(self):
            # unroll loops for scalar arrays to get padding right
            scalar_categories = [VkType.DEFAULT, VkType.BASETYPE, VkType.ENUM]
            if not self.loop_level:
                return
            if not self.var.ty.base.category in scalar_categories:
                return

            self.func_stem += '_array'
            self.loop_level -= 1
            self.loop_types.pop()
            self.loop_indices.pop()
            self.array_size = self.loop_counts.pop()

        def init_alloc_stmts(self):
            alloc_counts = self.loop_counts[:]
            if self.array_size:
                alloc_counts.append(self.array_size)

            if not alloc_counts:
                var_name = self._var_name()
                deref = self._var_deref()
                stmt = '%s = vn_cs_decoder_alloc_temp(dec, sizeof(%s%s))' % (
                        var_name, deref, var_name)
                self.func_alloc_stmt = stmt
                return

            alloc_stmts = []
            for level, count in enumerate(alloc_counts):
                if self.var.is_blob():
                    size = count
                else:
                    size = 'sizeof(*%s) * %s' % (self._var_name(level), count)

                stmt = '%s = vn_cs_decoder_alloc_temp(dec, %s)' % (
                        self._var_name(level, level > 0), size)
                alloc_stmts.append(stmt)

            if self.array_size:
                self.func_alloc_stmt = alloc_stmts.pop()
            self.loop_alloc_stmts = alloc_stmts

        def func_args(self, const_cast):
            var_name = self.prefix + self.var.name

            deref_count = self.var.ty.indirection_depth() + self.var.ty.is_array()
            deref_count -= self.loop_level
            # we want a pointer to var
            deref_count -= 1

            deref = ''
            if deref_count > 0:
                deref = '*' * deref_count
            elif deref_count < 0:
                deref = '&' * -deref_count

            args = '%s%s' % (deref, self._var_name(self.loop_level, const_cast))
            if self.array_size:
                args += ', ' + self.array_size

            return args

        def will_handle_array_size(self):
            return self.loop_extra_stmts or self.func_extra_stmt or \
                   (self.func_array_size_stmt and 'decode' in
                           self.func_array_size_stmt)

        def _code_enter_loops(self, indent_level, bracket_last):
            code = ''
            indent = '    ' * indent_level
            for level in range(self.loop_level):
                if level < len(self.loop_alloc_stmts):
                    code += '%s%s;\n' % (indent, self.loop_alloc_stmts[level])
                    code += '%sif (!%s) return;\n' % (indent, self._var_name(level))

                if level < len(self.loop_extra_stmts):
                    code += '%s%s;\n' % (indent, self.loop_extra_stmts[level])

                init_expr = '%s %c = 0' % (self.loop_types[level],
                        self.loop_indices[level])
                cond_expr = '%c < %s' % (self.loop_indices[level],
                        self.loop_counts[level])
                incr_expr = '%c++' % self.loop_indices[level]

                bracket = ' {'
                if not bracket_last and level == self.loop_level - 1:
                    bracket = ''

                code += '%sfor (%s; %s; %s)%s\n' % (indent, init_expr,
                        cond_expr, incr_expr, bracket)

                indent += '    '

            return code

        def _code_body(self, indent_level):
            code = ''
            indent = '    ' * (indent_level + self.loop_level)

            if self.func_array_size_stmt:
                code += '%s%s;\n' % (indent, self.func_array_size_stmt)

            if self.func_alloc_stmt:
                code += '%s%s;\n' % (indent, self.func_alloc_stmt)
                code += '%sif (!%s) return;\n' % (
                        indent, self._var_name(self.loop_level))

            if self.func_extra_stmt:
                code += '%s%s;\n' % (indent, self.func_extra_stmt)

            if self.func_stmt:
                code += '%s%s;\n' % (indent, self.func_stmt)

            return code

        def _code_leave_loops(self, indent_level, bracket_last):
            code = ''
            for level in reversed(range(self.loop_level)):
                if bracket_last or level < self.loop_level - 1:
                    indent = '    ' * (indent_level + level)
                    code += '%s}\n' % indent
            return code

        def need_bracket(self):
            if self.loop_alloc_stmts or self.loop_extra_stmts:
                return True
            count = (bool(self.func_array_size_stmt) +
                     bool(self.func_extra_stmt) +
                     bool(self.func_stmt))
            if count > 1:
                return True

            if self.loop_level:
                return True

            return False

        def code(self, indent_level):
            code_body = self._code_body(indent_level)

            bracket_last = code_body.count(';') > 1
            code_enter_loops = self._code_enter_loops(
                    indent_level, bracket_last)
            code_leave_loops = self._code_leave_loops(
                    indent_level, bracket_last)

            return code_enter_loops + code_body + code_leave_loops

    def _sizeof_variable(self, info, dst):
        if info.validity == info.INVALID:
            if info.var.ty.is_pointer():
                return '%s += vn_sizeof_simple_pointer(%s); /* out */' % (
                        dst, info._var_name())
            else:
                return '/* skip %s */' % info._var_name()

        code = ''
        if info.var.ty.is_pointer() and info.will_handle_array_size():
            code += 'if (%s) {\n    ' % info._var_name()
            code += '    %s\n    ' % info.code(2).strip()
            code += '} else {\n    '
            code += '    %s += vn_sizeof_array_size(0);\n    ' % dst
            code += '}'
        elif info.var.ty.is_pointer() and info.need_bracket():
            code += '%s += vn_sizeof_simple_pointer(%s);\n    ' % (
                        dst, info._var_name())
            code += 'if (%s) {\n    ' % info._var_name()
            code += '    %s\n    ' % info.code(2).strip()
            code += '}'
        elif info.var.ty.is_pointer():
            code += '%s += vn_sizeof_simple_pointer(%s);\n    ' % (
                        dst, info._var_name())
            code += 'if (%s)\n    ' % info._var_name()
            code += '    %s' % info.code(2).strip()
        else:
            code += info.code(1).strip()

        return code

    def _encode_variable(self, info):
        if info.validity == info.INVALID:
            if info.will_handle_array_size() and not info.var.ty.is_array():
                return 'vn_encode_array_size(enc, %s ? %s : 0); /* out */' % (
                        info._var_name(), info.array_size)
            elif info.var.ty.is_pointer():
                return 'vn_encode_simple_pointer(enc, %s); /* out */' % info._var_name()
            else:
                return '/* skip %s */' % info._var_name()

        code = ''
        if info.var.ty.is_pointer() and info.will_handle_array_size():
            code += 'if (%s) {\n    ' % info._var_name()
            code += '    %s\n    ' % info.code(2).strip()
            code += '} else {\n    '
            code += '    vn_encode_array_size(enc, 0);\n    '
            code += '}'
        elif info.var.ty.is_pointer() and info.need_bracket():
            code += 'if (vn_encode_simple_pointer(enc, %s)) {\n    ' % info._var_name()
            code += '    %s\n    ' % info.code(2).strip()
            code += '}'
        elif info.var.ty.is_pointer():
            code += 'if (vn_encode_simple_pointer(enc, %s))\n    ' % info._var_name()
            code += '    %s' % info.code(2).strip()
        else:
            code += info.code(1).strip()

        return code

    def _decode_variable(self, info):
        if info.validity == info.INVALID:
            if not info.var.ty.is_pointer():
                return '/* skip %s */' % info._var_name()

        code = ''
        if info.var.ty.is_pointer() and info.will_handle_array_size():
            code += 'if (vn_peek_array_size(dec)) {\n    '
            code += '    %s\n    ' % info.code(2).strip()
            code += '} else {\n    '
            code += '    vn_decode_array_size(dec, 0);\n    '
            code += '    %s = NULL;\n    ' % info._var_name()
            code += '}'
        elif info.var.ty.is_pointer():
            code += 'if (vn_decode_simple_pointer(dec)) {\n    '
            code += '    %s\n    ' % info.code(2).strip()
            code += '} else {\n    '
            code += '    %s = NULL;\n    ' % info._var_name()
            code += '}'
        elif info.need_bracket():
            code += '{\n    '
            code += '    %s\n    ' % info.code(2).strip()
            code += '}'
        else:
            code += info.code(1).strip()

        return code

    def _replace_variable_handle(self, info):
        if info.validity != info.VALID or \
           info.var.ty.base.category not in [VkType.HANDLE, VkType.STRUCT] or \
           not self.is_serializable(info.var):
            return '/* skip %s */' % info._var_name()

        code = ''
        if info.var.ty.is_pointer() and info.need_bracket():
            code += 'if (%s) {\n    ' % info._var_name()
            code += '   %s\n    ' % info.code(2).strip()
            code += '}'
        elif info.var.ty.is_pointer():
            code += 'if (%s)\n    ' % info._var_name()
            code += '    %s' % info.code(2).strip()
        else:
            code += info.code(1).strip()

        return code

    def _sizeof_variable_info(self, ty, var, prefix, validity, dst):
        info = self.VariableInfo(ty, var, prefix, validity)
        if not self.is_serializable(var):
            assert var.maybe_null()
            info.func_stmt = 'assert(false)'
            return info

        # save strlen result to a temp
        if var.is_string():
            assert info.array_size.startswith('strlen')
            info.func_array_size_stmt = \
                    'const size_t string_size = %s' % info.array_size
            info.array_size = 'string_size'

        # encode array sizes
        for loop_count in info.loop_counts:
            stmt = '%s += vn_sizeof_array_size(%s)' % (dst, loop_count)
            info.loop_extra_stmts.append(stmt)
        if info.array_size:
            info.func_extra_stmt = \
                    '%s += vn_sizeof_array_size(%s)' % (dst, info.array_size)

        # nothing to encode
        if validity == info.INVALID:
            return info

        func_name = 'vn_sizeof_' + info.func_stem
        if validity == info.PARTIAL and var.ty.base.category == ty.STRUCT:
            func_name += '_partial'

        info.func_stmt = '%s += %s(%s)' % (dst, func_name, info.func_args(False))

        return info

    def _encode_variable_info(self, ty, var, prefix, validity):
        info = self.VariableInfo(ty, var, prefix, validity)
        if not self.is_serializable(var):
            assert var.maybe_null()
            info.func_stmt = 'assert(false)'
            return info

        # save strlen result to a temp
        if var.is_string():
            assert info.array_size.startswith('strlen')
            info.func_array_size_stmt = \
                    'const size_t string_size = %s' % info.array_size
            info.array_size = 'string_size'

        # encode array sizes
        for loop_count in info.loop_counts:
            stmt = 'vn_encode_array_size(enc, %s)' % loop_count
            info.loop_extra_stmts.append(stmt)
        if info.array_size:
            info.func_extra_stmt = \
                    'vn_encode_array_size(enc, %s)' % info.array_size

        # nothing to encode
        if validity == info.INVALID:
            return info

        func_name = 'vn_encode_' + info.func_stem
        if validity == info.PARTIAL and var.ty.base.category == ty.STRUCT:
            func_name += '_partial'

        info.func_stmt = '%s(enc, %s)' % (func_name, info.func_args(False))

        return info

    def _decode_variable_info(self, ty, var, prefix, validity, alloc_storage):
        info = self.VariableInfo(ty, var, prefix, validity)
        if not self.is_serializable(var):
            assert var.maybe_null()
            if self.is_driver:
                info.func_stmt = 'assert(false)'
            else:
                info.func_stmt = 'vn_cs_decoder_set_fatal(dec)'
            return info

        # decode the encoded array size
        if info.array_size:
            if var.is_string():
                assert info.array_size.startswith('strlen')
                info.func_array_size_stmt = \
                        'const size_t string_size = vn_decode_array_size(dec, UINT64_MAX)'
                info.array_size = 'string_size'
            else:
                info.func_array_size_stmt = \
                        'const size_t array_size = vn_decode_array_size(dec, %s)' % info.array_size
                info.array_size = 'array_size'

        if alloc_storage and var.ty.is_pointer():
            info.init_alloc_stmts()

        # decode array sizes
        for loop_count in info.loop_counts:
            stmt = 'vn_decode_array_size(dec, %s)' % loop_count
            info.loop_extra_stmts.append(stmt)

        # nothing to decode
        if validity == info.INVALID:
            return info

        func_name = 'vn_decode_' + info.func_stem
        if validity == info.PARTIAL and var.ty.base.category == ty.STRUCT:
            func_name += '_partial'

        if alloc_storage and var.ty.base.category in [ty.STRUCT, ty.UNION]:
            func_name += '_temp'

        if var.ty.base.category == ty.HANDLE:
            if validity == info.VALID:
                # automatic handle lookup
                if not self.is_driver:
                    func_name += '_lookup'
            elif alloc_storage and var.ty.base.dispatchable:
                func_name += '_temp'

        info.func_stmt = '%s(dec, %s)' % (func_name, info.func_args(True))

        return info

    def _replace_variable_handle_info(self, ty, var, prefix, validity):
        info = self.VariableInfo(ty, var, prefix, validity)

        # nothing to replace
        might_contain_handle = [ty.HANDLE, ty.STRUCT]
        if (validity != info.VALID or
            var.ty.base.category not in might_contain_handle or
            not self.is_serializable(var)):
            return info

        info.func_stmt = 'vn_replace_%s_handle(%s)' % (
                info.func_stem, info.func_args(True))

        return info

    def _get_variable_validity(self, ty, var, initialized):
        if initialized:
            validity = self.VariableInfo.VALID
        else:
            partially_initialized = [ty.HANDLE, ty.STRUCT]
            if var.ty.base.category in partially_initialized:
                validity = self.VariableInfo.PARTIAL
            else:
                validity = self.VariableInfo.INVALID
        return validity

    def sizeof_struct_member(self, ty, var, prefix, struct_is_partial, dst):
        validity = self._get_variable_validity(ty, var, not struct_is_partial)
        info = self._sizeof_variable_info(ty, var, prefix, validity, dst)
        return self._sizeof_variable(info, dst)

    def encode_struct_member(self, ty, var, prefix, struct_is_partial):
        validity = self._get_variable_validity(ty, var, not struct_is_partial)
        info = self._encode_variable_info(ty, var, prefix, validity)
        return self._encode_variable(info)

    def decode_struct_member(self, ty, var, prefix, struct_is_partial, alloc_storage):
        validity = self._get_variable_validity(ty, var, not struct_is_partial)
        info = self._decode_variable_info(ty, var, prefix, validity, alloc_storage)
        return self._decode_variable(info)

    def replace_struct_member_handle(self, ty, var, prefix):
        validity = self._get_variable_validity(ty, var, True)
        info = self._replace_variable_handle_info(ty, var, prefix, validity)
        return self._replace_variable_handle(info)

    def sizeof_command_arg(self, ty, var, prefix, dst):
        validity = self._get_variable_validity(ty, var, 'var_in' in var.attrs)
        info = self._sizeof_variable_info(ty, var, prefix, validity, dst)
        return self._sizeof_variable(info, dst)

    def encode_command_arg(self, ty, var, prefix):
        validity = self._get_variable_validity(ty, var, 'var_in' in var.attrs)
        info = self._encode_variable_info(ty, var, prefix, validity)
        return self._encode_variable(info)

    def decode_command_arg(self, ty, var, prefix):
        validity = self._get_variable_validity(ty, var, 'var_in' in var.attrs)
        info = self._decode_variable_info(ty, var, prefix, validity, True)
        return self._decode_variable(info)

    def replace_command_arg_handle(self, ty, var, prefix):
        validity = self._get_variable_validity(ty, var, 'var_in' in var.attrs)
        info = self._replace_variable_handle_info(ty, var, prefix, validity)
        return self._replace_variable_handle(info)

    def sizeof_command_reply(self, ty, var, prefix, dst):
        if 'var_out' not in var.attrs:
            return '/* skip %s%s */' % (prefix, var.name)

        info = self._sizeof_variable_info(ty, var, prefix,
                self.VariableInfo.VALID, dst)
        return self._sizeof_variable(info, dst)

    def encode_command_reply(self, ty, var, prefix):
        if 'var_out' not in var.attrs:
            return '/* skip %s%s */' % (prefix, var.name)

        info = self._encode_variable_info(ty, var, prefix,
                self.VariableInfo.VALID)
        return self._encode_variable(info)

    def decode_command_reply(self, ty, var, prefix):
        if 'var_out' not in var.attrs:
            return '/* skip %s%s */' % (prefix, var.name)

        info = self._decode_variable_info(ty, var, prefix,
                self.VariableInfo.VALID, False)
        return self._decode_variable(info)

class GenCS:
    def __init__(self, gen):
        self.gen = gen

    def generate(self, template):
        return template.render()

class GenDefines:
    def __init__(self, gen):
        self.gen = gen

        self.enum_extends = []
        for ty in self.gen.api.type_table.values():
            if ty.category == ty.ENUM and ty.enums.vk_xml_values:
                if len(ty.enums.values) != len(ty.enums.vk_xml_values):
                    self.enum_extends.append(ty)

        self.typedef_types = []
        self.enum_types = []
        self.bitmask_types = []
        self.struct_types = []
        exts = self.gen.api.extensions[self.gen.api.vk_xml_extension_count:]
        for ext in exts:
            for ty in ext.types:
                if ty.category == ty.BASETYPE and ty.typedef:
                    self.typedef_types.append(ty)
                elif ty.category == ty.ENUM and ty.enums.values:
                    self.enum_types.append(ty)
                elif ty.category == ty.BITMASK:
                    self.bitmask_types.append(ty)
                elif ty.category == ty.STRUCT:
                    self.struct_types.append(ty)

        self.command_types = self.gen.supported_types[VkType.COMMAND]

    def generate(self, template):
        return template.render(
                TYPEDEF_TYPES=self.typedef_types,
                ENUM_EXTENDS=self.enum_extends,
                ENUM_TYPES=self.enum_types,
                BITMASK_TYPES=self.bitmask_types,
                STRUCT_TYPES=self.struct_types,
                COMMAND_TYPES=self.command_types)

class GenInfo:
    def __init__(self, gen):
        self.gen = gen

        self.exts = []
        for ext in self.gen.api.extensions:
            if ext.name in VK_XML_EXTENSION_LIST:
                self.exts.append(ext)
        self.exts.sort(key=lambda ext: ext.name)

    def generate(self, template):
        return template.render(
                WIRE_FORMAT_VERSION=VN_WIRE_FORMAT_VERSION,
                VK_XML_VERSION=self.gen.api.vk_xml_version,
                EXTENSIONS=self.exts)

class GenTypes:
    def __init__(self, gen):
        self.gen = gen

        self.early_scalar_types = []
        self.scalar_types = []

        for ty in self.gen.supported_types[VkType.DEFAULT]:
            if ty.name in self.gen.PRIMITIVE_TYPES:
                if ty.name in ['uint64_t', 'int32_t']:
                    self.early_scalar_types.append(ty)
                else:
                    self.scalar_types.append(ty)

        for ty in self.gen.supported_types[VkType.BASETYPE]:
            if ty.typedef and self.gen.is_serializable(ty.typedef):
                self.scalar_types.append(ty)

        for ty in self.gen.supported_types[VkType.ENUM]:
            if ty.enums.values:
                if ty.name == 'VkStructureType':
                    self.early_scalar_types.append(ty)
                else:
                    self.scalar_types.append(ty)

    def generate(self, template):
        return template.render(
                GEN=self.gen,
                EARLY_SCALAR_TYPES=self.early_scalar_types,
                SCALAR_TYPES=self.scalar_types)

class GenHandles:
    def __init__(self, gen):
        self.gen = gen

        self.handle_types = self.gen.supported_types[VkType.HANDLE]

    def generate(self, template):
        return template.render(
                GEN=self.gen,
                HANDLE_TYPES=self.handle_types)

class GenStructsAndCommands:
    # the order matters!
    RULES = {
        'structs': [],
        'transport': [], # matches all
        'instance': [
            'CreateInstance',
            'DestroyInstance',
            'EnumerateInstance',
            'GetInstance',
        ],
        # put VkPhysicalDevice and VkDevice in the same group because
        # VkPhysicalDeviceFeatures2 causes too much code to be generated in
        # the common header
        'device': [
            'EnumeratePhysicalDevice',
            'CreateDevice',
            'DestroyDevice',
            'Device',
            'GetDevice',
            'GetPhysicalDevice',
            'EnumerateDevice',
        ],
        'queue': [
            'Queue',
        ],
        'fence': [
            'CreateFence',
            'DestroyFence',
            'WaitForFence',
            'ResetFence',
            'GetFence',
        ],
        'semaphore': [
            'CreateSemaphore',
            'DestroySemaphore',
            'WaitSemaphore',
            'GetSemaphore',
            'SignalSemaphore',
        ],
        'event': [
            'CreateEvent',
            'DestroyEvent',
            'ResetEvent',
            'SetEvent',
            'GetEvent',
        ],
        'device_memory': [
            'AllocateMemory',
            'FlushMappedMemory',
            'FreeMemory',
            'GetDeviceMemory',
            'InvalidateMappedMemory',
            'MapMemory',
            'UnmapMemory',
        ],
        'image': [
            'BindImage',
            'CreateImage',
            'DestroyImage',
            'GetImage',
        ],
        'image_view': [
            'CreateImageView',
            'DestroyImageView',
        ],
        'sampler': [
            'CreateSampler',
            'DestroySampler',
        ],
        'sampler_ycbcr_conversion': [
            'CreateSamplerYcbcrConversion',
            'DestroySamplerYcbcrConversion',
        ],
        'buffer': [
            'BindBuffer',
            'CreateBuffer',
            'DestroyBuffer',
            'GetBuffer',
        ],
        'buffer_view': [
            'CreateBufferView',
            'DestroyBufferView',
        ],
        'descriptor_pool': [
            'CreateDescriptorPool',
            'DestroyDescriptorPool',
            'ResetDescriptorPool',
        ],
        'descriptor_set': [
            'AllocateDescriptorSet',
            'FreeDescriptorSet',
            'UpdateDescriptorSet',
        ],
        'descriptor_set_layout': [
            'CreateDescriptorSetLayout',
            'DestroyDescriptorSetLayout',
            'GetDescriptorSetLayout',
        ],
        'descriptor_update_template': [
            'CreateDescriptorUpdateTemplate',
            'DestroyDescriptorUpdateTemplate',
        ],
        'render_pass': [
            'CreateRenderPass',
            'DestroyRenderPass',
            'GetRenderArea',
        ],
        'framebuffer': [
            'CreateFramebuffer',
            'DestroyFramebuffer',
        ],
        'query_pool': [
            'CreateQueryPool',
            'DestroyQueryPool',
            'ResetQueryPool',
            'GetQueryPool',
        ],
        'shader_module': [
            'CreateShaderModule',
            'DestroyShaderModule',
        ],
        'pipeline': [
            'CreateComputePipeline',
            'CreateGraphicsPipeline',
            'DestroyPipeline',
        ],
        'pipeline_layout': [
            'CreatePipelineLayout',
            'DestroyPipelineLayout',
        ],
        'pipeline_cache': [
            'CreatePipelineCache',
            'DestroyPipelineCache',
            'GetPipelineCache',
            'MergePipelineCache',
        ],
        'command_pool': [
            'CreateCommandPool',
            'DestroyCommandPool',
            'ResetCommandPool',
            'TrimCommandPool',
        ],
        'command_buffer': [
            'AllocateCommandBuffer',
            'BeginCommandBuffer',
            'EndCommandBuffer',
            'FreeCommandBuffer',
            'ResetCommandBuffer',
            'Cmd',
        ],
    }

    class Group:
        def __init__(self, name, rules=[]):
            self.name = name
            self.rules = rules

            self.type_set = set()
            self.generated = set()

            self.commands = []
            self.skipped_commands = []
            self.structs = []
            self.manual_unions = []
            self.skipped_structs = []

        def match_command(self, cmd):
            for rule in self.rules:
                if cmd.name[2:].startswith(rule):
                    return True
            return False if self.rules else True

    def __init__(self, gen):
        self.gen = gen
        self._init_groups()

    def _init_groups(self):
        self.groups = []
        for key, val in self.RULES.items():
            self.groups.append(self.Group(key, val))

        # reverse before matching
        self.groups.reverse()

        # add commands and their dependencies to type_sets
        for cmd in self.gen.supported_types[VkType.COMMAND]:
            group = None
            for g in self.groups:
                if g.match_command(cmd):
                    group = g
                    break
            assert group
            self._add_type_set_recursive(group.type_set, cmd)

        # make sure each type belongs to just one type_set
        common_type_set = set()
        for i, g1 in enumerate(self.groups):
            for g2 in self.groups[i + 1:]:
                intersection = g1.type_set.intersection(g2.type_set)
                if intersection:
                    g2.type_set.difference_update(intersection)
                    common_type_set.update(intersection)
            g1.type_set.difference_update(common_type_set)

        # the remaining types belong to the last group's type_set
        last_group = self.groups[-1]
        assert last_group.name == 'structs' and not last_group.type_set
        last_group.type_set = common_type_set

        # now we can add types to groups
        for cmd in self.gen.supported_types[VkType.COMMAND]:
            group = None
            for g in self.groups:
                if g.match_command(cmd):
                    group = g
                    break
            assert group
            self._add_group_recursive(group, cmd)

    def _add_type_set_recursive(self, type_set, ty):
        # only commands, structs, and unions
        if ty.category not in [ty.COMMAND, ty.STRUCT, ty.UNION]:
            return
        if ty in type_set:
            return

        deps = [var.ty.base for var in ty.variables] + ty.p_next
        if ty.ret:
            deps.append(ty.ret.ty.base)

        for dep in deps:
            self._add_type_set_recursive(type_set, dep)
        type_set.add(ty)

    def _add_group_recursive(self, group, ty):
        # only commands, structs, and unions
        if ty.category not in [ty.COMMAND, ty.STRUCT, ty.UNION]:
            return

        # redirect to base group
        if ty not in group.type_set:
            group = self.groups[-1]
            assert ty in group.type_set

        if ty in group.generated:
            return
        group.generated.add(ty)

        if self.gen.is_serializable(ty):
            # add dependencies first
            deps = [var.ty.base for var in ty.variables] + ty.p_next
            if ty.ret:
                deps.append(ty.ret.ty.base)
            for dep in deps:
                self._add_group_recursive(group, dep)

            if ty.category == ty.COMMAND:
                group.commands.append(ty)
            else:
                group.structs.append(ty)
        elif ty.category == ty.UNION:
            can_manual = True
            for var in ty.variables:
                if not self.gen.is_serializable(var):
                    can_manual = False
                    break

            if can_manual:
                group.manual_unions.append(ty)
            else:
                group.skipped_structs.append(ty)
        elif ty.category == ty.COMMAND:
            group.skipped_commands.append(ty)
        else:
            group.skipped_structs.append(ty)

    def generate(self, template, group):
        return template.render(
                GEN=self.gen,
                GUARD=group.name.upper(),
                COMMAND_TYPES=group.commands,
                COMMAND_SKIPPED=group.skipped_commands,
                STRUCT_TYPES=group.structs,
                STRUCT_SKIPPED=group.skipped_structs,
                MANUAL_UNION_TYPES=group.manual_unions)

class GenDispatches:
    def __init__(self, gen):
        self.gen = gen

        self.includes = GenStructsAndCommands.RULES.keys()
        self.commands = []
        self.skipped = []
        for ty in self.gen.supported_types[VkType.COMMAND]:
            if self.gen.is_serializable(ty):
                self.commands.append(ty)
            else:
                self.skipped.append(ty)

    def generate(self, template):
        return template.render(
                GEN=self.gen,
                INCLUDES=self.includes,
                COMMAND_TABLE_SIZE=self.gen.api.max_vk_command_type_value + 1,
                COMMAND_TYPES=self.commands,
                COMMAND_SKIPPED=self.skipped)

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--outdir', help='Where to write the files.',
                        required=True)
    parser.add_argument('--banner', help='Path to the banner file.')
    parser.add_argument('--renderer',
                        help='Generate for the renderer.',
                        action='store_true')
    return parser.parse_args()

def get_generators(gen):
    classes = [
        GenCS,
        GenDefines,
        GenInfo,
        GenTypes,
        GenHandles,
        GenStructsAndCommands,
        GenDispatches,
    ]

    generators = {}
    for cls in classes:
        generators[cls] = cls(gen)

    return generators

def get_file_banner(filename):
    banner = None
    if filename:
        with open(filename, 'rb') as f:
            banner = f.read()
    if not banner:
        banner = '/* This file is generated by venus-protocol. */\n\n'.encode()

    return banner

def get_template(template_name):
    return Template(filename=str(VN_TEMPLATE_DIR.joinpath(template_name)),
                    lookup=VN_TEMPLATE_LOOKUP, output_encoding='utf-8')

def generate_base_headers(generators, base_headers, banner, outdir):
    for cls, name in base_headers:
        generator = generators[cls]
        template = get_template(name)
        output = Path(outdir).joinpath('vn_protocol_' + name)
        with open(output, 'wb') as f:
            f.write(banner)
            f.write(generator.generate(template))

def generate_command_headers(generator, variant, banner, outdir):
    template = get_template(variant + '_commands.h')
    for group in generator.groups:
        output = Path(outdir).joinpath('vn_protocol_%s_%s.h' % (variant, group.name))
        with open(output, 'wb') as f:
            f.write(banner)
            f.write(generator.generate(template, group))

def main():
    args = get_args()

    api = VkApi()
    api.parse_xmls(VN_PROTOCOL_XMLS)

    gen = Gen(not args.renderer, api)
    generators = get_generators(gen)

    if gen.is_driver:
        variant = 'driver'
        base_headers = [
            (GenCS,         variant + '_cs.h'),
            (GenDefines,    variant + '_defines.h'),
            (GenInfo,       variant + '_info.h'),
            (GenTypes,      variant + '_types.h'),
            (GenHandles,    variant + '_handles.h'),
        ]
    else:
        variant = 'renderer'
        base_headers = [
            (GenCS,         variant + '_cs.h'),
            (GenDefines,    variant + '_defines.h'),
            (GenInfo,       variant + '_info.h'),
            (GenTypes,      variant + '_types.h'),
            (GenHandles,    variant + '_handles.h'),
            (GenDispatches, variant + '_dispatches.h'),
        ]

    banner = '/* This file is generated by venus-protocol.  See vn_protocol_%s.h. */\n\n'
    banner = (banner % variant).encode()

    generate_base_headers(generators, base_headers, banner, args.outdir)

    generator = generators[GenStructsAndCommands]
    generate_command_headers(generator, variant, banner, args.outdir)

    banner = get_file_banner(args.banner)

    # generate a header that includes all other headers
    template = get_template(variant + '.h')
    output = Path(args.outdir).joinpath('vn_protocol_%s.h' % variant)
    with open(output, 'wb') as f:
        template_filenames = [hdr[1] for hdr in base_headers]
        template_filenames.extend(['%s_%s.h' % (variant, name) for name in
            GenStructsAndCommands.RULES.keys()])
        f.write(banner)
        f.write(template.render(TEMPLATE_FILENAMES=template_filenames))

if __name__ == '__main__':
    main()
