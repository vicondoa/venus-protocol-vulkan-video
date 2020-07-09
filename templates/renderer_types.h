/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="array" file="/types_array.h"/>\
<%namespace name="custom" file="/types_custom.h"/>\
\
#ifndef VN_PROTOCOL_RENDERER_TYPES_H
#define VN_PROTOCOL_RENDERER_TYPES_H

#include "vn_protocol_renderer_defines.h"

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
${array.vn_encode_type_array(ty)}
${array.vn_decode_type_array(ty)}
%   endif
% endfor
\
/* typedef arrays */

% for ty in TYPEDEF_TYPES:
%   if 'need_array' in ty.attrs:
${array.vn_encode_type_array(ty)}
${array.vn_decode_type_array(ty)}
%   endif
% endfor
\
/* enum arrays */

% for ty in ENUM_TYPES:
%   if 'need_array' in ty.attrs:
${array.vn_encode_type_array(ty)}
${array.vn_decode_type_array(ty)}
%   endif
% endfor
\
static inline void
vn_decode_string_temp(struct vn_cs *cs, char **val)
{
    const size_t size = vn_decode_array_size(cs, UINT64_MAX);
    char *str = vn_cs_alloc_temp(cs, size);
    if (str) {
        vn_decode(cs, (size + 3) & ~3, str, size);
        str[size - 1] = '\0';
    }

    *val = str;
}

static inline void
vn_decode_string_array_temp(struct vn_cs *cs, char ***val, uint32_t max_count)
{
    const uint32_t count = vn_decode_array_size(cs, max_count);
    char **strs = vn_cs_alloc_temp(cs, sizeof(*strs) * max_count);
    if (strs) {
        for (uint32_t i = 0; i < count; i++)
            vn_decode_string_temp(cs, &strs[i]);
        for (uint32_t i = count; i < max_count; i++)
            strs[i] = NULL;
    }

    *val = strs;
}

#endif /* VN_PROTOCOL_RENDERER_TYPES_H */
