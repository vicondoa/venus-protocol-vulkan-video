/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_encode_scalar_array(ty)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
% if ty.category == ty.DEFAULT:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

%   if ty_size >= 4:
    vn_encode(cs, size, val, size);
%   else:
    vn_encode(cs, (size + 3) & ~3, val, size);
%   endif
% elif ty.category == ty.BASETYPE:
    vn_encode_${ty.typedef.name}_array(cs, val, count);
% elif ty.category == ty.ENUM:
    vn_encode_int32_t_array(cs, (const int32_t *)val, count);
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_decode_scalar_array(ty)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint32_t count)
{
% if ty.category == ty.DEFAULT:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

%   if ty_size >= 4:
    vn_decode(cs, size, val, size);
%   else:
    vn_decode(cs, (size + 3) & ~3, val, size);
%   endif
% elif ty.category == ty.BASETYPE:
    vn_decode_${ty.typedef.name}_array(cs, val, count);
% elif ty.category == ty.ENUM:
    vn_decode_int32_t_array(cs, (int32_t *)val, count);
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_size_scalar_body(ty)">\
% if ty.category == ty.DEFAULT:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
    assert(sizeof(*val) == ${ty_size});
    return ${ty_size if ty_size >= 4 else 4};
% elif ty.category == ty.BASETYPE:
    return vn_size_${ty.typedef.name}(val);
% elif ty.category == ty.ENUM:
    assert(sizeof(*val) == sizeof(int32_t));
    return sizeof(int32_t);
% endif
</%def>

<%def name="vn_encode_scalar_body(ty)">\
% if ty.category == ty.DEFAULT:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
%   if ty_size >= 4:
    vn_encode(cs, ${ty_size}, val, sizeof(*val));
%   else:
    vn_encode(cs, 4, val, sizeof(*val));
%   endif
% elif ty.category == ty.BASETYPE:
    vn_encode_${ty.typedef.name}(cs, val);
% elif ty.category == ty.ENUM:
    vn_encode_int32_t(cs, (const int32_t *)val);
% endif
</%def>

<%def name="vn_decode_scalar_body(ty)">\
% if ty.category == ty.DEFAULT:
<% ty_size = GEN.PRIMITIVE_TYPES[ty.name] %>\
%   if ty_size >= 4:
    vn_decode(cs, ${ty_size}, val, sizeof(*val));
%   else:
    vn_decode(cs, 4, val, sizeof(*val));
%   endif
% elif ty.category == ty.BASETYPE:
    vn_decode_${ty.typedef.name}(cs, val);
% elif ty.category == ty.ENUM:
    vn_decode_int32_t(cs, (int32_t *)val);
% endif
</%def>
