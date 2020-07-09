/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_encode_command(ty)">\
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
</%def>

<%def name="vn_decode_command_args_temp(ty)">\
static inline void vn_decode_${ty.name}_args_temp(struct vn_cs *cs, struct vn_command_${ty.name} *args)
{
% for var in ty.variables:
    ${GEN.decode_command_arg(ty, var, 'args->')}
% endfor
}
</%def>

<%def name="vn_encode_command_reply(ty)">\
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
</%def>

<%def name="vn_decode_command_reply(ty)">\
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
</%def>

<%def name="vn_replace_command_args_handle(ty)">\
static inline void vn_replace_${ty.name}_args_handle(struct vn_command_${ty.name} *args)
{
% for var in ty.variables:
    ${GEN.replace_command_arg_handle(ty, var, 'args->')}
% endfor
}
</%def>
