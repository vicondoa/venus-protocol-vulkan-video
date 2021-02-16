/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="define_typedef, define_extended_enum, define_enum, define_bitmask, define_struct, define_command"/>\
\
#ifndef VN_PROTOCOL_RENDERER_DEFINES_H
#define VN_PROTOCOL_RENDERER_DEFINES_H

#include <stdlib.h>
#include <string.h>

#include "vulkan.h"

#include "vn_protocol_renderer_cs.h"

% for ty in TYPEDEF_TYPES:
${define_typedef(ty)}
% endfor
\
% for ty in ENUM_EXTENDS:
${define_extended_enum(ty)}
% endfor
\
% for ty in ENUM_TYPES:
${define_enum(ty)}
% endfor
\
% for ty in BITMASK_TYPES:
${define_bitmask(ty)}
% endfor
\
% for ty in STRUCT_TYPES:
${define_struct(ty)}
% endfor
\
% for ty in COMMAND_TYPES:
${define_command(ty)}
% endfor
\
struct vn_dispatch_context {
    void *data;
    void (*debug_log)(struct vn_dispatch_context *ctx, const char *msg);

    struct vn_cs_encoder *encoder;
    struct vn_cs_decoder *decoder;

% for ty in COMMAND_TYPES:
    void (*dispatch_${ty.name})(struct vn_dispatch_context *ctx, struct vn_command_${ty.name} *args);
% endfor
};

#endif /* VN_PROTOCOL_RENDERER_DEFINES_H */
