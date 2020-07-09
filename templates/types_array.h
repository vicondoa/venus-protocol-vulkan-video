/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="encode_scalar_array(ty, ty_size)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

    vn_encode_array_size(cs, count);
% if ty_size >= 4:
    vn_encode(cs, size, val, size);
% else:
    vn_encode(cs, (size + 3) & ~3, val, size);
% endif
}
</%def>

<%def name="decode_scalar_array(ty, ty_size)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
    const uint32_t count = vn_decode_array_size(cs, max_count);

    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

% if ty_size >= 4:
    vn_decode(cs, size, val, size);
% else:
    vn_decode(cs, (size + 3) & ~3, val, size);
% endif
}
</%def>

<%def name="encode_typedef_array(ty)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
    vn_encode_${ty.typedef.name}_array(cs, val, count);
}
</%def>

<%def name="decode_typedef_array(ty)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
    vn_decode_${ty.typedef.name}_array(cs, val, max_count);
}
</%def>

<%def name="encode_enum_array(ty)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
    assert(sizeof(*val) == sizeof(int32_t));
    vn_encode_int32_t_array(cs, (const int32_t *)val, count);
}
</%def>

<%def name="decode_enum_array(ty)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
    assert(sizeof(*val) == sizeof(int32_t));
    vn_decode_int32_t_array(cs, (int32_t *)val, max_count);
}
</%def>

<%def name="encode_generic_array(ty, variant='')">\
static inline void
vn_encode_${ty.name}_array${variant}(struct vn_cs *cs, const ${ty.name} *val, uint32_t count)
{
% if '_inout' in variant:
    /* XXX */
% endif
    vn_encode_array_size(cs, count);
    for (uint32_t i = 0; i < count; i++)
        vn_encode_${ty.name}${variant}(cs, &val[i]);
}
</%def>

<%def name="decode_generic_array(ty, variant='')">\
static inline void
vn_decode_${ty.name}_array${variant}(struct vn_cs *cs, ${ty.name} *val, uint32_t max_count)
{
    const uint32_t count = vn_decode_array_size(cs, max_count);
    for (uint32_t i = 0; i < count; i++)
        vn_decode_${ty.name}${variant}(cs, &val[i]);
}
</%def>

<%def name="replace_generic_array_handle(ty)">\
static inline void
vn_replace_${ty.name}_array_handle(${ty.name} *val, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
        vn_replace_${ty.name}_handle(&val[i]);
}
</%def>
