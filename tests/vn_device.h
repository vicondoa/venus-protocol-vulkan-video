/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_DEVICE_H
#define VN_DEVICE_H

#include "vn_cs.h"

struct vn_instance;
struct vn_renderer_bo;

struct vn_instance_submit_command {
   struct vn_cs_encoder command;
   size_t reply_size;

   struct vn_renderer_bo *reply_bo;
   struct vn_cs_decoder reply;
};

static inline bool
vn_renderer_bo_unref(struct vn_renderer_bo *bo)
{
	return true;
}

void
vn_instance_submit_command(struct vn_instance *instance,
                           struct vn_instance_submit_command *submit)
{
}

#endif /* VN_DEVICE_H */
