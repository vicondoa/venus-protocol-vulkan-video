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

    mtx_lock(&vn_instance->mutex);

    /* TODO suballocate from reply stream and avoid locking */
    assert(reply_size <= vn_instance->reply_size);

    /* TODO too many commands... */
    vn_cs_out_begin_reply_stream(&vn_instance->cs);
    if (vn_cs_reserve_out(&vn_instance->cs, cmd_size))
        vn_encode_${ty.name}(&vn_instance->cs, cmd_flags, ${ty.c_func_args()});
    vn_cs_out_end_reply_stream(&vn_instance->cs, vn_instance->reply_bo->res_id, 0, reply_size);

   if (vn_cs_has_error(&vn_instance->cs)) {
      vn_cs_reset(&vn_instance->cs);
      vn_cs_set_error(&vn_instance->cs);
      mtx_unlock(&vn_instance->mutex);
%   if ty.ret:
      return VK_ERROR_OUT_OF_HOST_MEMORY;
%   else:
      return;
%   endif
   }

    vn_cs_end_out(&vn_instance->cs);

    /* TODO suballocate reply_bo to avoid round trip with lock held... */
    vn_renderer_submit(vn_instance->renderer, &vn_instance->cs, &vn_instance->reply_bo, 1, true);

    vn_cs_set_in_data(&vn_instance->cs, vn_instance->reply_ptr, vn_instance->reply_size);

%   if ty.ret:
    const ${ty.ret.to_c()} = vn_decode_${ty.name}_reply(&vn_instance->cs, ${ty.c_func_args()});
%   else:
    vn_decode_${ty.name}_reply(&vn_instance->cs, ${ty.c_func_args()});
%   endif

    vn_cs_reset(&vn_instance->cs);

    mtx_unlock(&vn_instance->mutex);
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

    mtx_lock(&vn_instance->mutex);
    if (vn_cs_reserve_out(&vn_instance->cs, cmd_size))
        vn_encode_${ty.name}(&vn_instance->cs, cmd_flags, ${ty.c_func_args()});
    mtx_unlock(&vn_instance->mutex);
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
   mtx_lock(&instance->mutex);

   if (!vn_cs_has_out(&instance->cs)) {
      mtx_unlock(&instance->mutex);
      return;
   }

   if (vn_cs_has_error(&instance->cs)) {
      vn_cs_reset(&instance->cs);
      vn_cs_set_error(&instance->cs);
      mtx_unlock(&instance->mutex);
      return;
   }

   vn_cs_end_out(&instance->cs);
   vn_renderer_submit(instance->renderer, &instance->cs, NULL, 0, false);
   vn_cs_reset(&instance->cs);

   mtx_unlock(&instance->mutex);
}

#endif /* VN_PROTOCOL_DRIVER_CALLS_H */
