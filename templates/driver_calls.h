/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="call_command(ty)">\
static inline ${ty.c_func_ret()} vn_call_${ty.name}(struct vn_instance *vn_instance, ${ty.c_func_params()})
{
    const size_t cmd_size = vn_sizeof_${ty.name}(${ty.c_func_args()});
    const size_t reply_size = vn_sizeof_${ty.name}_reply(${ty.c_func_args()});
    const VnCommandFlags cmd_flags = VN_COMMAND_GENERATE_REPLY_BIT;

    struct vn_cs *cs = vn_instance_lock_cs(vn_instance);

    size_t reply_offset;
    void *reply_ptr;
    struct vn_renderer_bo *reply_bo = vn_instance_alloc_cs_reply_locked(
            vn_instance, reply_size, &reply_offset, &reply_ptr);
    if (!reply_bo) {
       vn_instance_unlock_cs(vn_instance);
%   if ty.ret:
      return VK_ERROR_OUT_OF_HOST_MEMORY;
%   else:
      return;
%   endif
    }

    /* TODO too many commands... */
    vn_cs_out_begin_reply_stream(cs);
    if (vn_cs_reserve_out(cs, cmd_size))
        vn_encode_${ty.name}(cs, cmd_flags, ${ty.c_func_args()});
    vn_cs_out_end_reply_stream(cs, reply_bo->res_id, reply_offset, reply_size);

   if (vn_cs_has_error(cs)) {
      vn_cs_reset(cs);
      vn_cs_set_error(cs);
      vn_instance_unlock_cs(vn_instance);
      vn_instance_free_cs_reply(vn_instance, reply_bo);

%   if ty.ret:
      return VK_ERROR_OUT_OF_HOST_MEMORY;
%   else:
      return;
%   endif
   }

    vn_cs_end_out(cs);

    /* TODO replace wait_cpu by vn_renderer_sync to wait without lock */
    vn_renderer_submit(vn_instance->renderer, cs, &reply_bo, 1, true);

    /* TODO use a local cs to decode without lock */
    vn_cs_set_in_data(cs, reply_ptr, reply_size);

%   if ty.ret:
    const ${ty.ret.to_c()} = vn_decode_${ty.name}_reply(cs, ${ty.c_func_args()});
%   else:
    vn_decode_${ty.name}_reply(cs, ${ty.c_func_args()});
%   endif

    vn_cs_reset(cs);

    vn_instance_unlock_cs(vn_instance);
    vn_instance_free_cs_reply(vn_instance, reply_bo);
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
    const VnCommandFlags cmd_flags = 0;

    struct vn_cs *cs = vn_instance_lock_cs(vn_instance);
    if (vn_cs_reserve_out(cs, cmd_size))
        vn_encode_${ty.name}(cs, cmd_flags, ${ty.c_func_args()});
    vn_instance_unlock_cs(vn_instance);
}
</%def>\
\
#ifndef VN_PROTOCOL_DRIVER_CALLS_H
#define VN_PROTOCOL_DRIVER_CALLS_H

#include "vn_device.h"
#include "vn_protocol_driver_commands.h"
#include "vn_renderer.h"

% for ty in COMMAND_TYPES:
${call_command(ty)}
${async_command(ty)}
% endfor
\
static inline void
vn_async_flush(struct vn_instance *instance)
{
    struct vn_cs *cs = vn_instance_lock_cs(instance);

    if (!vn_cs_has_out(cs)) {
        vn_instance_unlock_cs(instance);
        return;
    }

    if (vn_cs_has_error(cs)) {
        vn_cs_reset(cs);
        vn_cs_set_error(cs);
        vn_instance_unlock_cs(instance);
        return;
    }

    vn_cs_end_out(cs);
    vn_renderer_submit(instance->renderer, cs, NULL, 0, false);
    vn_cs_reset(cs);

    vn_instance_unlock_cs(instance);
}

#endif /* VN_PROTOCOL_DRIVER_CALLS_H */
