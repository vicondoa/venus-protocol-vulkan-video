/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="common" file="/common.h"/>\
\
#ifndef VN_PROTOCOL_RENDERER_TYPES_H
#define VN_PROTOCOL_RENDERER_TYPES_H

#include "vn_protocol_renderer_defines.h"

% for ty, size in SCALAR_TYPES:
/* ${ty.name} */

${common.encode_scalar(ty, size)}
${common.decode_scalar(ty, size)}
%   if 'need_array' in ty.attrs:
${common.encode_scalar_array(ty, size)}
${common.decode_scalar_array(ty, size)}
%   endif
% endfor
\
% for ty in TYPEDEF_TYPES:
/* typedef ${ty.typedef.name} ${ty.name} */

${common.encode_typedef(ty)}
${common.decode_typedef(ty)}
%   if 'need_array' in ty.attrs:
${common.encode_typedef_array(ty)}
${common.decode_typedef_array(ty)}
%   endif
% endfor
\
% for ty in ENUM_TYPES:
/* enum ${ty.name} */

${common.encode_enum(ty)}
${common.decode_enum(ty)}
% endfor
\
${common.encode_decode_special()}
\
static inline void
vn_decode_string_temp(struct vn_cs *cs, char **val)
{
    uint64_t count;
    vn_decode_uint64_t(cs, &count);

    char *str = vn_cs_alloc_temp(cs, count);
    if (str) {
        vn_decode(cs, (count + 3) & ~3, str, count);
        str[count - 1] = '\0';
    }

    *val = str;
}

#endif /* VN_PROTOCOL_RENDERER_TYPES_H */
