/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="array" file="/types_array.h"/>\
<%namespace name="custom" file="/types_custom.h"/>\
\
#ifndef VN_PROTOCOL_DRIVER_TYPES_H
#define VN_PROTOCOL_DRIVER_TYPES_H

#include "vn_protocol_driver_defines.h"

% for ty, size in SCALAR_TYPES:
/* ${ty.name} */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
% endfor
\
% for ty in TYPEDEF_TYPES:
/* typedef ${ty.typedef.name} ${ty.name} */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
% endfor
\
% for ty in ENUM_TYPES:
/* enum ${ty.name} */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
% endfor
\
${custom.vn_custom_types()}
\
/* scalar arrays */

% for ty, size in SCALAR_TYPES:
%   if 'need_array' in ty.attrs:
${array.encode_scalar_array(ty, size)}
${array.decode_scalar_array(ty, size)}
%   endif
% endfor
\
/* typedef arrays */

% for ty in TYPEDEF_TYPES:
%   if 'need_array' in ty.attrs:
${array.encode_typedef_array(ty)}
${array.decode_typedef_array(ty)}
%   endif
% endfor
\
/* enum arrays */

% for ty in ENUM_TYPES:
%   if 'need_array' in ty.attrs:
${array.encode_enum_array(ty)}
${array.decode_enum_array(ty)}
%   endif
% endfor
\
/* TODO remove this and all callers */
static inline void
vn_decode_string(struct vn_cs *cs, const char *val)
{
    vn_cs_set_error(cs);
}

#endif /* VN_PROTOCOL_DRIVER_TYPES_H */
