/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="decode_command_args_temp(ty)">\
static inline void vn_decode_${ty.name}_args_temp(struct vn_cs *cs, struct vn_command_${ty.name} *args)
{
% for var in ty.variables:
    ${GEN.decode_command_arg(ty, var, 'args->')}
% endfor
}
</%def>\
\
<%def name="replace_command_args_handle(ty)">\
static inline void vn_replace_${ty.name}_args_handle(struct vn_command_${ty.name} *args)
{
% for var in ty.variables:
    ${GEN.replace_command_arg_handle(ty, var, 'args->')}
% endfor
}
</%def>\
\
<%def name="encode_command_reply(ty)">\
static inline void vn_encode_${ty.name}_reply(struct vn_cs *cs, const struct vn_command_${ty.name} *args)
{
    vn_encode_VnCommandType(cs, &(VnCommandType){${ty.attrs['c_type']}});
% if ty.ret and ty.ret.name == 'VkResult':
    vn_encode_VkResult(cs, &args->ret);
% else:
    vn_encode_VkResult(cs, &(VkResult){VK_SUCCESS});
% endif
% if ty.ret and ty.ret.name != 'VkResult':
    ${GEN.encode_command_ret(ty, 'ret', 'args->')}
% endif
% for var in ty.variables:
    ${GEN.encode_command_reply(ty, var, 'args->')}
% endfor
}
</%def>\
\
#ifndef VN_PROTOCOL_RENDERER_COMMANDS_H
#define VN_PROTOCOL_RENDERER_COMMANDS_H

#include "vn_protocol_renderer_structs.h"

/*
 * These commands are not included
 *
% for ty in COMMAND_SKIPPED:
 *   ${ty.name}
% endfor
 */

% for ty in COMMAND_TYPES:
${decode_command_args_temp(ty)}
${replace_command_args_handle(ty)}
${encode_command_reply(ty)}
% endfor
\
#endif /* VN_PROTOCOL_RENDERER_COMMANDS_H */
