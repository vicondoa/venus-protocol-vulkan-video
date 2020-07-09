/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="array" file="/types_array.h"/>\
<%namespace name="handle" file="/types_handle.h"/>\
\
#ifndef VN_PROTOCOL_RENDERER_HANDLES_H
#define VN_PROTOCOL_RENDERER_HANDLES_H

#include "vn_protocol_renderer_types.h"

% for ty in HANDLE_TYPES:
/* VK_DEFINE_HANDLE(${ty.name}) */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
${handle.vn_decode_handle_lookup(ty)}
${types.vn_decode_type_temp(ty)}
${types.vn_replace_type_handle(ty)}
%   if 'need_array' in ty.attrs:
${array.vn_encode_type_array(ty)}
${array.vn_decode_type_array(ty)}
${array.vn_decode_handle_array_lookup(ty)}
${array.vn_decode_type_array_temp(ty)}
${array.vn_replace_type_array_handle(ty)}
%   endif
% endfor
\
% for ty in ND_HANDLE_TYPES:
/* VK_DEFINE_NON_DISPATCHABLE_HANDLE(${ty.name}) */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
${handle.vn_decode_handle_lookup(ty)}
${types.vn_replace_type_handle(ty)}
%   if 'need_array' in ty.attrs:
${array.vn_encode_type_array(ty)}
${array.vn_decode_type_array(ty)}
${array.vn_decode_handle_array_lookup(ty)}
${array.vn_replace_type_array_handle(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_RENDERER_HANDLES_H */
