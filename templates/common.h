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
${define_enum(ty.bitmask)}
typedef VkFlags ${ty.name};
</%def>

<%def name="define_capset(wire_format_ver, vn_xml_ver, vk_xml_ver, ext_table)">\
static inline uint32_t
vn_capset_wire_format_version(void)
{
    return ${wire_format_ver};
}

static inline uint32_t
vn_capset_vn_xml_version(void)
{
    return ${vn_xml_ver};
}

static inline uint32_t
vn_capset_vk_xml_version(void)
{
    return ${vk_xml_ver};
}
<%
c_table_size = (len(ext_table) + 31) // 32
c_table = []
for i in range(c_table_size):
  start = i * 32
  stop = min((i + 1) * 32, len(ext_table))
  val = 0
  names = []
  for j in range(start, stop):
    if ext_table[j]:
      val |= 1 << (j % 32)
      names.append("%d. %s" % (j, ext_table[j]))
  c_table.append((val, names))
%>
static inline const uint32_t *
vn_capset_vk_xml_extension_table(uint32_t *size)
{
    static const uint32_t vk_xml_extension_table[${c_table_size}] = {
% for val, names in c_table:
  % if names:
        /*
    % for name in names:
         * ${name}
    % endfor
         */
  % endif
        ${"0x%08x" % val},
% endfor
    };

    *size = ${c_table_size};
    return vk_xml_extension_table;
}
</%def>
