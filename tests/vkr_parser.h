/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VKR_PARSER_H
#define VKR_PARSER_H

#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"

typedef uint64_t vkr_object_id;

struct vkr_parser_object {
    uint64_t handle;
};

struct vkr_parser;

static inline void
vkr_parser_set_error(struct vkr_parser *parser)
{
}

static inline bool
vkr_parser_has_error(const struct vkr_parser *parser)
{
    return false;
}

static inline struct vkr_parser_object *
vkr_parser_lookup_object(struct vkr_parser *parser, vkr_object_id id)
{
    return NULL;
}

static inline void
vkr_parser_reset_temp_pool(struct vkr_parser *parser)
{
}

static inline void *
vkr_parser_alloc_temp(struct vkr_parser *parser, size_t size)
{
    return NULL;
}

static inline void
vkr_parser_peek(struct vkr_parser *parser,
                void *val,
                size_t val_size)
{
}

static inline void
vkr_parser_read(struct vkr_parser *parser,
                size_t size,
                void *val,
                size_t val_size)
{
}

static inline void
vkr_parser_reply(struct vkr_parser *parser,
                 size_t size,
                 const void *val,
                 size_t val_size)
{
}

static inline void
vkr_parser_handle_store_id(void *vk_handle, vkr_object_id id, bool in_place)
{
}

static inline vkr_object_id
vkr_parser_handle_load_id(const void *vk_handle, bool in_place)
{
    return 0;
}

#endif /* VKR_PARSER_H */
