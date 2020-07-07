/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_CS_H
#define VN_CS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t vn_cs_object_id;

struct vn_cs;

static inline void
vn_cs_reset(struct vn_cs *cs)
{
}

static inline void
vn_cs_set_error(struct vn_cs *cs)
{
}

static inline void
vn_cs_in(struct vn_cs *cs, size_t size, void *val, size_t val_size)
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

static inline vn_cs_object_id
vn_cs_handle_load_id(const void *vk_handle, bool is_dev)
{
    return 0;
}

static inline void
vn_cs_handle_store_id(void *vk_handle, vn_cs_object_id id, bool is_dev)
{
}

#endif /* VN_CS_H */
