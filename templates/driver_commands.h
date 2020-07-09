/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="command" file="/types_command.h"/>\
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
${command.vn_encode_command(ty)}
${command.vn_decode_command_reply(ty)}
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_COMMANDS_H */
