/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="array" file="/types_array.h"/>\
\
#ifndef VN_PROTOCOL_DRIVER_HANDLES_H
#define VN_PROTOCOL_DRIVER_HANDLES_H

#include "vn_protocol_driver_types.h"

% for ty in HANDLE_TYPES:
/* VK_DEFINE_HANDLE(${ty.name}) */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
%   if 'need_array' in ty.attrs:
${array.encode_generic_array(ty)}
${array.decode_generic_array(ty)}
%   endif
% endfor
\
% for ty in ND_HANDLE_TYPES:
/* VK_DEFINE_NON_DISPATCHABLE_HANDLE(${ty.name}) */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
%   if 'need_array' in ty.attrs:
${array.encode_generic_array(ty)}
${array.decode_generic_array(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_HANDLES_H */
