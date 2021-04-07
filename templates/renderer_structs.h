/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="union" file="/types_union.h"/>\
\
#ifndef VN_PROTOCOL_RENDERER_STRUCTS_H
#define VN_PROTOCOL_RENDERER_STRUCTS_H

#include "vn_protocol_renderer_handles.h"

/*
 * These structs/unions are not included
 *
% for ty in STRUCT_SKIPPED:
 *   ${ty.name}
% endfor
% for ty in MANUAL_UNION_TYPES:
 *   ${ty.name}
% endfor
 */

% for ty in STRUCT_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

%   if 'need_encode' in ty.attrs:
${types.vn_encode_type_helpers(ty)}\
${types.vn_encode_type(ty)}
%   endif
\
%   if 'need_decode' in ty.attrs:
${types.vn_decode_type_helpers(ty, '_temp')}\
${types.vn_decode_type_temp(ty)}
%   endif
\
%   if 'need_partial' in ty.attrs and ty.category == ty.STRUCT:
${types.vn_decode_type_helpers(ty, '_partial_temp')}\
${types.vn_decode_type_partial_temp(ty)}
%   endif
\
%   if 'need_decode' in ty.attrs and ty.category == ty.STRUCT:
${types.vn_replace_type_handle_helpers(ty)}\
${types.vn_replace_type_handle(ty)}
%   endif
% endfor
/*
 * Helpers for manual serialization
 */

% for ty in MANUAL_UNION_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

%   if 'need_encode' in ty.attrs:
${union.vn_encode_union_tag(ty)}
%   endif
%   if 'need_decode' in ty.attrs:
${types.vn_decode_type_temp(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_RENDERER_STRUCTS_H */
