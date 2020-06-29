/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="encode_command(ty)">\
static inline void vn_encode_${ty.name}(struct vn_cs *cs, VnCommandFlags cmd_flags, ${ty.c_func_params()})
{
    const VnCommandType cmd_type = ${ty.attrs['c_type']};

    vn_encode_VnCommandType(cs, &cmd_type);
    vn_encode_VkFlags(cs, &cmd_flags);

% for var in ty.variables:
    ${GEN.encode_command_arg(ty, var, '')}
% endfor

    /* TODO */
    if (cmd_flags & VN_COMMAND_GENERATE_REPLY_BIT) {
        size_t reply_size = 0;
        vn_cs_reserve_out(cs, reply_size);
    }
}
</%def>\
\
<%def name="decode_command_reply(ty)">\
static inline ${ty.c_func_ret()} vn_decode_${ty.name}_reply(struct vn_cs *cs, ${ty.c_func_params()})
{
    VnCommandType command_type;
    vn_decode_VnCommandType(cs, &command_type);
    assert(command_type == ${ty.attrs['c_type']});

% if ty.ret and ty.ret.name == 'VkResult':
    VkResult ret;
    vn_decode_VkResult(cs, &ret);
% else:
    VkResult command_result;
    vn_decode_VkResult(cs, &command_result);
    if (command_result != VK_SUCCESS)
        vn_cs_set_error(cs);
%   if ty.ret:

    ${ty.ret.name} ret;
    ${GEN.decode_command_ret(ty, 'ret', '')}
%   endif
% endif
% for var in ty.variables:
    ${GEN.decode_command_reply(ty, var, '')}
% endfor
% if ty.ret:

    return ret;
% endif
}
</%def>\
\
#ifndef VN_PROTOCOL_DRIVER_COMMANDS_H
#define VN_PROTOCOL_DRIVER_COMMANDS_H

#include "vn_protocol_driver_structs.h"

/*
 * These commands are not included
 *
% for ty in COMMAND_SKIPPED:
 *   ${ty.name}
% endfor
 */

% for ty in COMMAND_TYPES:
${encode_command(ty)}
${decode_command_reply(ty)}
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_COMMANDS_H */
