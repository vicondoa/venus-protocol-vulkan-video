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

<%def name="encode_scalar(ty, ty_size)">\
static inline void
vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
    assert(sizeof(*val) == ${ty_size});
% if ty_size >= 4:
    vn_encode(cs, sizeof(*val), val, sizeof(*val));
% else:
    vn_encode(cs, 4, val, sizeof(*val));
% endif
}
</%def>

<%def name="decode_scalar(ty, ty_size)">\
static inline void
vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
    assert(sizeof(*val) == ${ty_size});
% if ty_size >= 4:
    vn_decode(cs, sizeof(*val), val, sizeof(*val));
% else:
    vn_decode(cs, 4, val, sizeof(*val));
% endif
}
</%def>

<%def name="encode_scalar_array(ty, ty_size)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint64_t count)
{
    assert(sizeof(*val) == ${ty_size});
    const size_t size = sizeof(*val) * count;
    assert(size >= count);

    vn_encode_uint64_t(cs, &count);
% if ty_size >= 4:
    vn_encode(cs, size, val, size);
% else:
    vn_encode(cs, (size + 3) & ~3, val, size);
% endif
}
</%def>

<%def name="decode_scalar_array(ty, ty_size)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint64_t max_count)
{
    uint64_t count;
    vn_decode_uint64_t(cs, &count);
    if (count > max_count) {
        vn_cs_set_error(cs);
        count = max_count;
    }

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

<%def name="encode_typedef(ty)">\
static inline void
vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
    vn_encode_${ty.typedef.name}(cs, val);
}
</%def>

<%def name="decode_typedef(ty)">\
static inline void
vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
    vn_decode_${ty.typedef.name}(cs, val);
}
</%def>

<%def name="encode_typedef_array(ty)">\
static inline void
vn_encode_${ty.name}_array(struct vn_cs *cs, const ${ty.name} *val, uint64_t count)
{
    vn_encode_${ty.typedef.name}_array(cs, val, count);
}
</%def>

<%def name="decode_typedef_array(ty)">\
static inline void
vn_decode_${ty.name}_array(struct vn_cs *cs, ${ty.name} *val, uint64_t max_count)
{
    vn_decode_${ty.typedef.name}_array(cs, val, max_count);
}
</%def>

<%def name="encode_enum(ty)">\
static inline void
vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
    assert(sizeof(*val) == sizeof(int32_t));
    vn_encode_int32_t(cs, (const int32_t *)val);
}
</%def>

<%def name="decode_enum(ty)">\
static inline void
vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
    assert(sizeof(*val) == sizeof(int32_t));
    vn_decode_int32_t(cs, (int32_t *)val);
}
</%def>

<%def name="encode_decode_special()">\
/* size_t */

static inline void
vn_encode_size_t(struct vn_cs *cs, const size_t *val)
{
    const uint64_t tmp = *val;
    vn_encode_uint64_t(cs, &tmp);
}

static inline void
vn_decode_size_t(struct vn_cs *cs, size_t *val)
{
    uint64_t tmp;
    vn_decode_uint64_t(cs, &tmp);
    *val = tmp;
}

/* pointer */

static inline bool
vn_encode_pointer(struct vn_cs *cs, const void *val)
{
    const uint64_t tmp = (uintptr_t)val;
    vn_encode_uint64_t(cs, &tmp);
    return tmp > 0;
}

static inline bool
vn_decode_pointer(struct vn_cs *cs)
{
    uint64_t tmp;
    vn_decode_uint64_t(cs, &tmp);
    return tmp > 0;
}

/* pNext chain */

static inline void
vn_encode_end_of_chain(struct vn_cs *cs)
{
    vn_encode_VkStructureType(cs, &(VkStructureType){ VK_STRUCTURE_TYPE_MAX_ENUM });
}

static inline void
vn_decode_end_of_chain(struct vn_cs *cs)
{
    VkStructureType tmp;
    vn_decode_VkStructureType(cs, &tmp);
    if (tmp != VK_STRUCTURE_TYPE_MAX_ENUM)
        vn_cs_set_error(cs);
}

/* blob */

static inline void
vn_encode_blob(struct vn_cs *cs, const void *val, size_t size)
{
    vn_encode_uint8_t_array(cs, (const uint8_t *)val, size);
}

static inline void
vn_decode_blob(struct vn_cs *cs, void *val, size_t max_size)
{
    vn_decode_uint8_t_array(cs, (uint8_t *)val, max_size);
}

/* string */

static inline void
vn_encode_string(struct vn_cs *cs, const char *val)
{
    const size_t len = strlen(val);

    assert(sizeof(*val) == sizeof(uint8_t));
    vn_encode_uint8_t_array(cs, (const uint8_t *)val, len + 1);
}

static inline void
vn_encode_char_array(struct vn_cs *cs, const char *val, uint64_t count)
{
    const size_t len = strlen(val);
    assert(len < count);

    assert(sizeof(*val) == sizeof(uint8_t));
    vn_encode_uint8_t_array(cs, (const uint8_t *)val, len + 1);
}

static inline void
vn_decode_char_array(struct vn_cs *cs, char *val, uint64_t max_count)
{
    assert(sizeof(*val) == sizeof(uint8_t));
    vn_decode_uint8_t_array(cs, (uint8_t *)val, max_count);
    val[max_count - 1] = '\0';
}
</%def>

<%def name="encode_struct(ty, variant='')">\
<% skip_vars = 2 if '_self' in variant and ty.s_type else 0 %>\
static inline void vn_encode_${ty.name}${variant}(struct vn_cs *cs, const ${ty.name} *val)
{
% if skip_vars:
    /* skip val->{${','.join([var.name for var in ty.variables[:skip_vars]])}} */
% endif
% for var in ty.variables[skip_vars:]:
%   if '_inout' in variant:
    ${GEN.encode_struct_inout(ty, var, 'val->')}
%   else:
    ${GEN.encode_struct_member(ty, var, 'val->')}
%   endif
% endfor
}
</%def>

<%def name="decode_struct(ty, variant='')">\
<% skip_vars = 2 if '_self' in variant and ty.s_type else 0 %>\
static inline void vn_decode_${ty.name}${variant}(struct vn_cs *cs, ${ty.name} *val)
{
% if skip_vars:
    /* skip val->{${','.join([var.name for var in ty.variables[:skip_vars]])}} */
% endif
% for var in ty.variables[skip_vars:]:
%   if '_inout' in variant:
    ${GEN.decode_struct_inout(ty, var, 'val->', '_temp' in variant)}
%   else:
    ${GEN.decode_struct_member(ty, var, 'val->', '_temp' in variant)}
%   endif
% endfor
}
</%def>

<%def name="encode_pnext_chain(ty, variant='')">\
static inline void vn_encode_${ty.name}${variant}(struct vn_cs *cs, const ${ty.name} *val)
{
    const struct VkBaseInStructure *pnext = (const struct VkBaseInStructure *)val;
<%
    next_types, skipped_types = GEN.get_pnext_chain(ty)
%>
    do {
        switch (pnext->sType) {
% for next_ty in [ty] + next_types:
        case ${next_ty.s_type}:
            vn_encode_VkStructureType(cs, &pnext->sType);
            vn_encode_${next_ty.name}_self${variant}(cs, (const ${next_ty.name} *)pnext);
            break;
% endfor
% for skipped_ty in skipped_types:
        case ${skipped_ty.s_type}:
% endfor
        default:
            /* ignore unknown/unsupported struct */
            break;
        }
        pnext = pnext->pNext;
    } while (pnext);

    vn_encode_end_of_chain(cs);
}
</%def>

<%def name="decode_pnext_chain(ty)">\
static inline void vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
    struct VkBaseOutStructure *pnext = (struct VkBaseOutStructure *)val;
<%
    next_types, skipped_types = GEN.get_pnext_chain(ty)
%>
    do {
        VkStructureType stype;
        vn_decode_VkStructureType(cs, &stype);

        while (pnext->sType != stype) {
            pnext = pnext->pNext;
            assert(pnext);
        }

        switch (stype) {
% for next_ty in [ty] + next_types:
        case ${next_ty.s_type}:
            vn_decode_${next_ty.name}_self(cs, (${next_ty.name} *)pnext);
            break;
% endfor
% for skipped_ty in skipped_types:
        case ${skipped_ty.s_type}:
% endfor
        default:
            /* unexpected struct */
            vn_cs_set_error(cs);
            break;
        }
        pnext = pnext->pNext;
    } while (pnext);

    vn_decode_end_of_chain(cs);
}
</%def>

<%def name="encode_union(ty, default_tag)">\
static inline void vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
    /* encoding val->${ty.variables[default_tag].name} suffices */
    const uint32_t tag = ${default_tag};
    vn_encode_uint32_t(cs, &tag);
    ${GEN.encode_struct_member(ty, ty.variables[default_tag], 'val->')}
}
</%def>

<%def name="encode_union_tag(ty)">\
static inline void vn_encode_${ty.name}_tag(struct vn_cs *cs, const ${ty.name} *val, uint32_t tag)
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

<%def name="decode_union(ty, variant='')">\
static inline void vn_decode_${ty.name}${variant}(struct vn_cs *cs, ${ty.name} *val)
{
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
}
</%def>
