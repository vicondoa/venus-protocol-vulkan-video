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

struct vkr_object {
    uint64_t handle;
};

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
