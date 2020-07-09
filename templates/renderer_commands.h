/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="command" file="/types_command.h"/>\
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
${command.vn_decode_command_args_temp(ty)}
${command.vn_replace_command_args_handle(ty)}
${command.vn_encode_command_reply(ty)}
% endfor
\
#endif /* VN_PROTOCOL_RENDERER_COMMANDS_H */
