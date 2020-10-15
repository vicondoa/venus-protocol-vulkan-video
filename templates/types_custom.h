/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_custom_size_t()">\
/* size_t */

% if GEN.is_driver:
static inline size_t
vn_sizeof_size_t(const size_t *val)
{
    return vn_sizeof_uint64_t(&(uint64_t){ *val });
}

% endif
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

% if GEN.is_driver:
static inline size_t
vn_sizeof_size_t_array(const size_t *val, uint32_t count)
{
    return vn_sizeof_size_t(val) * count;
}

% endif
static inline void
vn_encode_size_t_array(struct vn_cs *cs, const size_t *val, uint32_t count)
{
    if (sizeof(size_t) == sizeof(uint64_t)) {
        vn_encode_uint64_t_array(cs, (const uint64_t *)val, count);
    } else {
        for (uint32_t i = 0; i < count; i++)
            vn_encode_size_t(cs, &val[i]);
    }
}

static inline void
vn_decode_size_t_array(struct vn_cs *cs, size_t *val, uint32_t count)
{
    if (sizeof(size_t) == sizeof(uint64_t)) {
        vn_decode_uint64_t_array(cs, (uint64_t *)val, count);
    } else {
        for (uint32_t i = 0; i < count; i++)
            vn_decode_size_t(cs, &val[i]);
    }
}
</%def>

<%def name="vn_custom_data()">\
/* opaque data */

% if GEN.is_driver:
static inline size_t
vn_sizeof_data_array(const void *val, size_t size)
{
    return (size + 3) & ~3;
}

% endif
static inline void
vn_encode_data_array(struct vn_cs *cs, const void *val, size_t size)
{
    vn_encode(cs, (size + 3) & ~3, val, size);
}

static inline void
vn_decode_data_array(struct vn_cs *cs, void *val, size_t size)
{
    vn_decode(cs, (size + 3) & ~3, val, size);
}
</%def>

<%def name="vn_custom_end_of_chain()">\
/* end of chain */

% if GEN.is_driver:
static inline size_t
vn_sizeof_end_of_chain(void)
{
    return vn_sizeof_VkStructureType(&(VkStructureType){ VK_STRUCTURE_TYPE_MAX_ENUM });
}

% endif
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
</%def>

<%def name="vn_custom_array_size()">\
/* array size (uint64_t) */

% if GEN.is_driver:
static inline size_t
vn_sizeof_array_size(uint64_t size)
{
    return vn_sizeof_uint64_t(&size);
}

% endif
static inline void
vn_encode_array_size(struct vn_cs *cs, uint64_t size)
{
    vn_encode_uint64_t(cs, &size);
}

static inline uint64_t
vn_decode_array_size(struct vn_cs *cs, uint64_t max_size)
{
    uint64_t size;
    vn_decode_uint64_t(cs, &size);
    if (size > max_size) {
        vn_cs_set_error(cs);
        size = 0;
    }
    return size;
}

static inline uint64_t
vn_peek_array_size(struct vn_cs *cs)
{
    uint64_t size;
    vn_cs_in_peek(cs, &size, sizeof(size));
    return size;
}

/* non-array pointer */

% if GEN.is_driver:
static inline size_t
vn_sizeof_simple_pointer(const void *val)
{
    return vn_sizeof_array_size(val ? 1 : 0);
}

% endif
static inline bool
vn_encode_simple_pointer(struct vn_cs *cs, const void *val)
{
    vn_encode_array_size(cs, val ? 1 : 0);
    return val;
}

static inline bool
vn_decode_simple_pointer(struct vn_cs *cs)
{
    return vn_decode_array_size(cs, 1);
}
</%def>
