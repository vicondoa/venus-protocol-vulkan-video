/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_custom_types()">\
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

/* array size */

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

/* opaque data */

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
