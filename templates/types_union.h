/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_encode_union_tag(ty)">\
static inline void
vn_encode_${ty.name}_tag(struct vn_cs *cs, const ${ty.name} *val, uint32_t tag)
{
    vn_encode_uint32_t(cs, &tag);
    switch (tag) {
% for (i, var) in enumerate(ty.variables):
    case ${i}:
        ${GEN.encode_struct_member(ty, var, 'val->')}
        break;
% endfor
    default:
        assert(false);
        break;
    }
}
</%def>

<%def name="vn_size_union_body(ty, variant='')">\
<% tag = GEN.UNION_DEFAULT_TAGS[ty.name] %>\
    return sizeof(uint32_t) + 16; /* TODO GEN.size_struct_member(ty, ty.variables[tag], 'val->') */
</%def>

<%def name="vn_encode_union_body(ty)">\
<% tag = GEN.UNION_DEFAULT_TAGS[ty.name] %>\
    vn_encode_${ty.name}_tag(cs, val, ${tag}); /* union with default tag */
</%def>

<%def name="vn_decode_union_body(ty, variant='')">\
    uint32_t tag;
    vn_decode_uint32_t(cs, &tag);
    switch (tag) {
% for (i, var) in enumerate(ty.variables):
    case ${i}:
        ${GEN.decode_struct_member(ty, var, 'val->', '_temp' in variant)}
        break;
% endfor
    default:
        vn_cs_set_error(cs);
        break;
    }
</%def>
