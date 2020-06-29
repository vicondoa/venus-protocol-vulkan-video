/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="define_typedef, define_enum, define_bitmask"/>\
\
<%def name="define_command(ty)">\
struct vn_command_${ty.name} {
    ${ty.c_func_params(';\n    ')};
% if ty.ret:

    ${ty.ret.name} ret;
% endif
};
</%def>\
\
#ifndef VN_PROTOCOL_RENDERER_DEFINES_H
#define VN_PROTOCOL_RENDERER_DEFINES_H

#include <string.h>

#include "vulkan.h"

#include "vn_protocol_renderer_cs.h"

% for ty in TYPEDEF_TYPES:
${define_typedef(ty)}
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
% for ty in COMMAND_TYPES:
${define_command(ty)}
% endfor
\
struct vn_dispatch_context {
    void *data;
    void (*debug_log)(struct vn_dispatch_context *ctx, const char *msg);

    struct vn_cs *cs;

% for ty in COMMAND_TYPES:
    void (*dispatch_${ty.name})(struct vn_dispatch_context *ctx, struct vn_command_${ty.name} *args);
% endfor
};

#endif /* VN_PROTOCOL_RENDERER_DEFINES_H */
