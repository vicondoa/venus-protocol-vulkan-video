/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_CS_H
#define VN_CS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t vn_object_id;

struct vn_cs;
struct vn_cs_decoder;

static inline void
vn_cs_reset(struct vn_cs *cs)
{
}

static inline void
vn_cs_set_error(struct vn_cs *cs)
{
}

static inline bool
vn_cs_has_out(const struct vn_cs *cs)
{
    return true;
}

static inline bool
vn_cs_reserve_out(struct vn_cs *cs, size_t size)
{
    return true;
}

static inline void
vn_cs_out(struct vn_cs *cs, size_t size, const void *val, size_t val_size)
{
}

static inline void
vn_cs_out_begin_reply_stream(struct vn_cs *cs)
{
}

static inline void
vn_cs_out_end_reply_stream(struct vn_cs *cs,
                           uint32_t res_id,
                           size_t offset,
                           size_t size)
{
}

static inline void
vn_cs_end_out(struct vn_cs *cs)
{
}

static inline void
vn_cs_decoder_set_fatal(struct vn_cs_decoder *dec)
{
}

static inline void
vn_cs_decoder_read(struct vn_cs_decoder *dec, size_t size, void *val, size_t val_size)
{
}

static inline void
vn_cs_decoder_peek(struct vn_cs_decoder *dec, void *val, size_t val_size)
{
}

static inline vn_object_id
vn_cs_object_load_id(const void *obj_handle)
{
    return 0;
}

static inline void
vn_cs_object_store_id(void *obj_handle, vn_object_id id)
{
}

static inline vn_object_id
vn_cs_device_load_id(const VkDevice *dev_handle)
{
    return 0;
}

static inline void
vn_cs_device_store_id(VkDevice *dev_handle, vn_object_id id)
{
}

#endif /* VN_CS_H */
