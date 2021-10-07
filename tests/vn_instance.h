/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_INSTANCE_H
#define VN_INSTANCE_H

#include "vn_cs.h"

#define VN_TRACE_FUNC()

struct vn_instance;

struct vn_instance_submit_command {
   int dummy;
};

static inline struct vn_cs_encoder *
vn_instance_submit_command_init(struct vn_instance *instance,
                                struct vn_instance_submit_command *submit,
                                void *cmd_data,
                                size_t cmd_size,
                                size_t reply_size)
{
   return NULL;
}

void
vn_instance_submit_command(struct vn_instance *instance,
                           struct vn_instance_submit_command *submit)
{
}

static inline struct vn_cs_decoder *
vn_instance_get_command_reply(struct vn_instance *instance,
                              struct vn_instance_submit_command *submit)
{
    return NULL;
}

static inline void
vn_instance_free_command_reply(struct vn_instance *instance,
                               struct vn_instance_submit_command *submit)
{
}

#endif /* VN_INSTANCE_H */
