/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="dispatch_command(ty)">\
static inline void vn_dispatch_${ty.name}(struct vn_dispatch_context *ctx, VkCommandFlagsEXT flags)
{
    struct vn_command_${ty.name} args;

    if (!ctx->dispatch_${ty.name}) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    vn_decode_${ty.name}_args_temp(ctx->decoder, &args);

    if (!vn_cs_decoder_get_fatal(ctx->decoder))
        ctx->dispatch_${ty.name}(ctx, &args);

% if ty.ret and ty.ret.ty.name == 'VkResult':
    if (!vn_cs_decoder_get_fatal(ctx->decoder) && args.${ty.ret.name} < VK_SUCCESS) {
        switch (args.${ty.ret.name}) {
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            break;
        default:
            vn_dispatch_debug_log(ctx, "${ty.name} returned %d", args.${ty.ret.name});
            break;
        }
    }
% endif

    if (!vn_cs_decoder_get_fatal(ctx->decoder) && (flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT))
       vn_encode_${ty.name}_reply(ctx->encoder, &args);

    vn_cs_decoder_reset_temp_pool(ctx->decoder);
}
</%def>\
\
#ifndef VN_PROTOCOL_RENDERER_DISPATCHES_H
#define VN_PROTOCOL_RENDERER_DISPATCHES_H

#include <stdarg.h>
#include <stdio.h>

#include "vn_protocol_renderer_commands.h"

static inline const char *vn_dispatch_command_name(VkCommandTypeEXT type)
{
    switch (type) {
% for ty in COMMAND_TYPES:
    case ${ty.attrs['c_type']}: return "${ty.name}";
% endfor
% for ty in COMMAND_SKIPPED:
    case ${ty.attrs['c_type']}: return "${ty.name}";
% endfor
    default: return "unknown";
    }
}

static inline void vn_dispatch_debug_log(struct vn_dispatch_context *ctx, const char *format, ...)
{
    char msg[256];
    va_list va;

    if (!ctx->debug_log)
        return;

    va_start(va, format);
    vsnprintf(msg, sizeof(msg), format, va);
    ctx->debug_log(ctx, msg);
    va_end(va);
}

% for ty in COMMAND_TYPES:
${dispatch_command(ty)}
% endfor
\
static void (*const vn_dispatch_table[${COMMAND_TABLE_SIZE}])(struct vn_dispatch_context *ctx, VkCommandFlagsEXT flags) = {
% for ty in COMMAND_TYPES:
    [${ty.attrs['c_type']}] = vn_dispatch_${ty.name},
% endfor
};

static inline void vn_dispatch_command(struct vn_dispatch_context *ctx)
{
    VkCommandTypeEXT cmd_type;
    VkCommandFlagsEXT cmd_flags;

    vn_decode_VkCommandTypeEXT(ctx->decoder, &cmd_type);
    vn_decode_VkFlags(ctx->decoder, &cmd_flags);

    if (cmd_type < ${COMMAND_TABLE_SIZE} && vn_dispatch_table[cmd_type])
        vn_dispatch_table[cmd_type](ctx, cmd_flags);
    else
        vn_cs_decoder_set_fatal(ctx->decoder);

    if (vn_cs_decoder_get_fatal(ctx->decoder))
        vn_dispatch_debug_log(ctx, "%s resulted in CS error", vn_dispatch_command_name(cmd_type));
}

#endif /* VN_PROTOCOL_RENDERER_DISPATCHES_H */
