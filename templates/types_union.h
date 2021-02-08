/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_sizeof_union_tag(ty)">\
static inline size_t
vn_sizeof_${ty.name}_tag(const ${ty.name} *val, uint32_t tag)
{
    size_t size = vn_sizeof_uint32_t(&tag);
    switch (tag) {
% for (i, var) in enumerate(ty.variables):
    case ${i}:
        ${GEN.sizeof_struct_member(ty, var, 'val->', False, 'size')}
        break;
% endfor
    default:
        assert(false);
        break;
    }
    return size;
}
</%def>

<%def name="vn_encode_union_tag(ty)">\
static inline void
vn_encode_${ty.name}_tag(struct vn_cs_encoder *enc, const ${ty.name} *val, uint32_t tag)
{
    vn_encode_uint32_t(enc, &tag);
    switch (tag) {
% for (i, var) in enumerate(ty.variables):
    case ${i}:
        ${GEN.encode_struct_member(ty, var, 'val->', False)}
        break;
% endfor
    default:
        assert(false);
        break;
    }
}
</%def>

<%def name="vn_sizeof_union_body(ty, variant='')">\
<% tag = GEN.UNION_DEFAULT_TAGS[ty.name] %>\
    return vn_sizeof_${ty.name}_tag(val, ${tag});
</%def>

<%def name="vn_encode_union_body(ty)">\
<% tag = GEN.UNION_DEFAULT_TAGS[ty.name] %>\
    vn_encode_${ty.name}_tag(enc, val, ${tag}); /* union with default tag */
</%def>

<%def name="vn_decode_union_body(ty, variant='')">\
    uint32_t tag;
    vn_decode_uint32_t(dec, &tag);
    switch (tag) {
% for (i, var) in enumerate(ty.variables):
    case ${i}:
        ${GEN.decode_struct_member(ty, var, 'val->', False, '_temp' in variant)}
        break;
% endfor
    default:
        vn_cs_set_error(dec);
        break;
    }
</%def>
