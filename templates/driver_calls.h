/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="call_command(ty)">\
static inline ${ty.c_func_ret()} vn_call_${ty.name}(struct vn_instance *vn_instance, ${ty.c_func_params()})
{
    const size_t cmd_size = vn_sizeof_${ty.name}(${ty.c_func_args()});
    const size_t reply_size = vn_sizeof_${ty.name}_reply(${ty.c_func_args()});
    const VkCommandFlagsEXT cmd_flags = VK_COMMAND_GENERATE_REPLY_BIT_EXT;
    bool submitted = false;
    struct vn_renderer_bo *reply_bo;
    void *reply_ptr;
    uint64_t reply_sync_val;

    /* encode and submit */
    struct vn_cs_encoder *enc = vn_instance_lock_cs(vn_instance);
    reply_bo = vn_instance_get_cs_reply_bo_locked(vn_instance, reply_size, &reply_ptr);
    if (likely(reply_bo && vn_cs_encoder_reserve(enc, cmd_size))) {
        vn_encode_${ty.name}(enc, cmd_flags, ${ty.c_func_args()});
        submitted = vn_instance_submit_cs_locked(vn_instance, reply_bo, &reply_sync_val);
    }
    vn_instance_unlock_cs(vn_instance);

    /* decode reply */
%   if ty.ret:
    ${ty.ret.to_c()} = VK_ERROR_OUT_OF_HOST_MEMORY;
%   endif
    if (likely(submitted)) {
        struct vn_cs_decoder dec;
        vn_cs_decoder_init(&dec, reply_ptr, reply_size);

        vn_instance_wait_cs_reply(vn_instance, reply_sync_val);
%   if ty.ret:
        ${ty.ret.name} = vn_decode_${ty.name}_reply(&dec, ${ty.c_func_args()});
%   else:
        vn_decode_${ty.name}_reply(&dec, ${ty.c_func_args()});
%   endif
        vn_instance_free_cs_reply_bo(vn_instance, reply_bo);
    } else if (reply_bo) {
        vn_instance_free_cs_reply_bo(vn_instance, reply_bo);
    }
%   if ty.ret:

    return ${ty.ret.name};
%   endif
}
</%def>\
\
<%def name="async_command(ty)">\
static inline void vn_async_${ty.name}(struct vn_instance *vn_instance, ${ty.c_func_params()})
{
    const size_t cmd_size = vn_sizeof_${ty.name}(${ty.c_func_args()});
    const VkCommandFlagsEXT cmd_flags = 0;

    struct vn_cs_encoder *enc = vn_instance_lock_cs(vn_instance);
    if (vn_cs_encoder_reserve(enc, cmd_size))
        vn_encode_${ty.name}(enc, cmd_flags, ${ty.c_func_args()});
% if ty.name in ['vkCreateGraphicsPipelines', 'vkCreateComputePipelines']:

    bool throttle = false;
    uint64_t throttle_sync_val;
    vn_instance->cs_throttle_pipeline_count += createInfoCount;
    if (vn_instance->cs_throttle_pipeline_count >
        vn_instance->cs_throttle_pipeline_threshold) {
        /* TODO refactor vn_instance_submit_cs_locked */
        assert(vn_instance->cs_reply.bo);
        throttle = vn_instance_submit_cs_locked(vn_instance,
                vn_instance->cs_reply.bo, &throttle_sync_val);
    }

% endif
    if (vn_cs_encoder_get_len(enc) > vn_instance->cs_implicit_flush_threshold)
        vn_instance_submit_cs_locked(vn_instance, NULL, NULL);
    vn_instance_unlock_cs(vn_instance);
% if ty.name in ['vkCreateGraphicsPipelines', 'vkCreateComputePipelines']:

    if (throttle)
        vn_instance_wait_cs_reply(vn_instance, throttle_sync_val);
% endif
}
</%def>\
\
#ifndef VN_PROTOCOL_DRIVER_CALLS_H
#define VN_PROTOCOL_DRIVER_CALLS_H

#include "vn_protocol_driver_commands.h"
#include "vn_device.h"

% for ty in COMMAND_TYPES:
${call_command(ty)}
${async_command(ty)}
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_CALLS_H */
