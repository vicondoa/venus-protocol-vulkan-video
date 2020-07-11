/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="custom" file="/types_custom.h"/>\
<%namespace name="scalar" file="/types_scalar.h"/>\
\
#ifndef VN_PROTOCOL_DRIVER_TYPES_H
#define VN_PROTOCOL_DRIVER_TYPES_H

#include "vn_protocol_driver_defines.h"

% for ty in EARLY_SCALAR_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
% endfor
\
${custom.vn_custom_types()}
\
% for ty in SCALAR_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

%   if ty not in EARLY_SCALAR_TYPES:
${types.vn_encode_type(ty)}
${types.vn_decode_type(ty)}
%   endif
%   if 'need_array' in ty.attrs:
${scalar.vn_encode_scalar_array(ty)}
${scalar.vn_decode_scalar_array(ty)}
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
