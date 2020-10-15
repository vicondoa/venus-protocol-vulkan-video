/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="define_typedef(ty)">\
typedef ${ty.typedef.name} ${ty.name};
</%def>

<%def name="define_enum(ty)">\
typedef enum ${ty.name} {
% for (key, val) in ty.enums.values.items():
    ${key} = ${val},
% endfor
} ${ty.name};
</%def>

<%def name="define_bitmask(ty)">\
typedef VkFlags ${ty.name};
</%def>

<%def name="define_struct(ty)">\
typedef struct ${ty.name} {
    ${ty.c_func_params(';\n    ')};
} ${ty.name};
</%def>

<%def name="define_command(ty)">\
struct vn_command_${ty.name} {
    ${ty.c_func_params(';\n    ')};
% if ty.ret:

    ${ty.ret.to_c()};
% endif
};
</%def>

<%def name="define_info(wire_format_ver, vk_xml_ver, exts)">\
static inline uint32_t
vn_info_wire_format_version(void)
{
    return ${wire_format_ver};
}

static inline uint32_t
vn_info_vk_xml_version(void)
{
    return ${vk_xml_ver};
}

static inline int
vn_info_extension_compare(const void *a, const void *b)
{
   return strcmp(a, *(const char **)b);
}

static inline uint32_t
vn_info_extension_spec_version(const char *name)
{
    static uint32_t ext_count = ${len(exts)};
    static const char *ext_names[${len(exts)}] = {
% for ext in exts:
        "${ext.name}",
% endfor
    };
    static const uint32_t ext_versions[${len(exts)}] = {
% for ext in exts:
        ${ext.version},
% endfor
    };
    const char **found;

    found = bsearch(name, ext_names, ext_count, sizeof(ext_names[0]),
          vn_info_extension_compare);

    return found ? ext_versions[found - ext_names] : 0;
}
</%def>
