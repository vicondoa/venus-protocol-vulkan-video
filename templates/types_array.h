/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_encode_scalar_array_body(ty)">\
% if ty.category == ty.DEFINE:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

    vn_encode_array_size(cs, count);
%   if ty_size >= 4:
    vn_encode(cs, size, val, size);
%   else:
    vn_encode(cs, (size + 3) & ~3, val, size);
%   endif
% elif ty.category == ty.BASETYPE:
    vn_encode_${ty.typedef.name}_array(cs, val, count);
% elif ty.category == ty.ENUM:
    vn_encode_int32_t_array(cs, (const int32_t *)val, count);
% endif
</%def>

<%def name="vn_decode_scalar_array_body(ty)">\
% if ty.category == ty.DEFINE:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
    const uint32_t count = vn_decode_array_size(cs, max_count);

    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

%   if ty_size >= 4:
    vn_decode(cs, size, val, size);
%   else:
    vn_decode(cs, (size + 3) & ~3, val, size);
%   endif
% elif ty.category == ty.BASETYPE:
    vn_decode_${ty.typedef.name}_array(cs, val, max_count);
% elif ty.category == ty.ENUM:
    vn_decode_int32_t_array(cs, (int32_t *)val, max_count);
% endif
</%def>

<%def name="vn_encode_loop_array_body(ty, variant='')">\
% if '_partial' in variant:
    /* XXX */
% endif
    vn_encode_array_size(cs, count);
    for (uint32_t i = 0; i < count; i++)
        vn_encode_${ty.name}${variant}(cs, &val[i]);
</%def>

<%def name="vn_decode_loop_array_body(ty, variant='')">\
    const uint32_t count = vn_decode_array_size(cs, max_count);
    for (uint32_t i = 0; i < count; i++)
        vn_decode_${ty.name}${variant}(cs, &val[i]);
</%def>

<%def name="vn_encode_type_array(ty)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${vn_encode_scalar_array_body(ty)}\
% else:
${vn_encode_loop_array_body(ty)}\
% endif
}
</%def>

<%def name="vn_decode_type_array(ty)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${vn_decode_scalar_array_body(ty)}\
% else:
${vn_decode_loop_array_body(ty)}\
% endif
}
</%def>

<%def name="vn_decode_type_array_temp(ty)">\
static inline void
vn_decode_${ty.name}_array_temp(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${vn_decode_scalar_array_body(ty)}\
% else:
${vn_decode_loop_array_body(ty, '_temp')}\
% endif
}
</%def>

<%def name="vn_encode_type_array_partial(ty)">\
static inline void
vn_encode_${ty.name}_array_partial(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${vn_encode_scalar_array_body(ty)}\
% else:
${vn_encode_loop_array_body(ty, '_partial')}\
% endif
}
</%def>

<%def name="vn_decode_type_array_partial_temp(ty)">\
static inline void
vn_decode_${ty.name}_array_partial_temp(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${vn_decode_scalar_array_body(ty)}\
% else:
${vn_decode_loop_array_body(ty, '_partial_temp')}\
% endif
}
</%def>

<%def name="vn_decode_handle_array_lookup(ty)">\
<% assert(not GEN.is_driver) %>\
static inline void
vn_decode_${ty.name}_array_lookup(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
${vn_decode_loop_array_body(ty, '_lookup')}\
}
</%def>

<%def name="vn_replace_type_array_handle(ty)">\
static inline void
vn_replace_${ty.name}_array_handle(${ty.name} *val, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
        vn_replace_${ty.name}_handle(&val[i]);
}
</%def>
