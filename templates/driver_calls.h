/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="submit_command(ty)">\
static inline void vn_submit_${ty.name}(struct vn_instance *vn_instance, VkCommandFlagsEXT cmd_flags, ${ty.c_func_params()}, struct vn_instance_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_${ty.name}(${ty.c_func_args()});
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }

    submit->command = VN_CS_ENCODER_INITIALIZER(cmd_data, cmd_size);
    if (cmd_size)
        vn_encode_${ty.name}(&submit->command, cmd_flags, ${ty.c_func_args()});
    submit->reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_${ty.name}_reply(${ty.c_func_args()}) : 0;
    vn_instance_submit_command(vn_instance, submit);

    if (cmd_data != local_cmd_data)
        free(cmd_data);
}
</%def>\
\
<%def name="call_command(ty)">\
static inline ${ty.c_func_ret()} vn_call_${ty.name}(struct vn_instance *vn_instance, ${ty.c_func_params()})
{
    struct vn_instance_submit_command submit;
    vn_submit_${ty.name}(vn_instance, VK_COMMAND_GENERATE_REPLY_BIT_EXT, ${ty.c_func_args()}, &submit);
%   if ty.ret:
    if (submit.reply_bo) {
        const ${ty.ret.to_c()} = vn_decode_${ty.name}_reply(&submit.reply, ${ty.c_func_args()});
        vn_renderer_bo_unref(submit.reply_bo);
        return ${ty.ret.name};
    } else {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
%   else:
    if (submit.reply_bo) {
        vn_decode_${ty.name}_reply(&submit.reply, ${ty.c_func_args()});
        vn_renderer_bo_unref(submit.reply_bo);
    }
%   endif
}
</%def>\
\
<%def name="async_command(ty)">\
static inline void vn_async_${ty.name}(struct vn_instance *vn_instance, ${ty.c_func_params()})
{
    struct vn_instance_submit_command submit;
    vn_submit_${ty.name}(vn_instance, 0, ${ty.c_func_args()}, &submit);
}
</%def>\
\
#ifndef VN_PROTOCOL_DRIVER_CALLS_H
#define VN_PROTOCOL_DRIVER_CALLS_H

#include "vn_protocol_driver_commands.h"
#include "vn_device.h"

#define VN_SUBMIT_LOCAL_CMD_SIZE 256

% for ty in COMMAND_TYPES:
${submit_command(ty)}
% endfor
% for ty in COMMAND_TYPES:
${call_command(ty)}
${async_command(ty)}
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_CALLS_H */
