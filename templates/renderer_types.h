/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="custom" file="/types_custom.h"/>\
<%namespace name="scalar" file="/types_scalar.h"/>\
\
#ifndef VN_PROTOCOL_RENDERER_TYPES_H
#define VN_PROTOCOL_RENDERER_TYPES_H

#include "vn_protocol_renderer_defines.h"

/* Explicit bit packing for the StdVideo H.264 bitfield flag structs. The
 * generated serializers below expand VN_H264_DEFINE_FLAG_SERIALIZERS, which is
 * defined here along with the shift constants that form the wire contract. */
#include "vn_protocol_video_h264_flags.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

% for ty in EARLY_SCALAR_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

${types.vn_sizeof_type(ty)}
${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
%   if 'need_array' in ty.attrs:
${scalar.vn_encode_scalar_array(ty)}
${scalar.vn_decode_scalar_array(ty)}
%   endif
% endfor
\
${custom.vn_custom_size_t()}
${custom.vn_custom_blob()}
${custom.vn_custom_string()}
${custom.vn_custom_array_size()}
\
% for ty in SCALAR_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

${types.vn_sizeof_type(ty)}
${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
%   if 'need_array' in ty.attrs:
${scalar.vn_encode_scalar_array(ty)}
${scalar.vn_decode_scalar_array(ty)}
%   endif
% endfor
\
<%doc>
  Emitted after SCALAR_TYPES: the StdVideo helpers call vn_{encode,decode}_uint16_t
  and friends, which are defined in that loop.
</%doc>
${custom.vn_custom_video_h264_flags()}
\
#pragma GCC diagnostic pop

#endif /* VN_PROTOCOL_RENDERER_TYPES_H */
