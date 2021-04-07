/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="union" file="/types_union.h"/>\
\
#ifndef VN_PROTOCOL_DRIVER_STRUCTS_H
#define VN_PROTOCOL_DRIVER_STRUCTS_H

#include "vn_protocol_driver_handles.h"

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

${types.vn_sizeof_type_helpers(ty)}\
${types.vn_sizeof_type(ty)}
\
%   if 'need_encode' in ty.attrs:
${types.vn_encode_type_helpers(ty)}\
${types.vn_encode_type(ty)}
%   endif
\
%   if 'need_decode' in ty.attrs:
${types.vn_decode_type_helpers(ty)}\
${types.vn_decode_type(ty)}
%   endif
\
%   if 'need_partial' in ty.attrs and ty.category == ty.STRUCT:
${types.vn_sizeof_type_helpers(ty, '_partial')}\
${types.vn_sizeof_type_partial(ty)}
\
${types.vn_encode_type_helpers(ty, '_partial')}\
${types.vn_encode_type_partial(ty)}
%   endif
% endfor
/*
 * Helpers for manual serialization
 */

% for ty in MANUAL_UNION_TYPES:
/* ${types.vn_type_descriptive_name(ty)} */

${union.vn_sizeof_union_tag(ty)}
%   if 'need_encode' in ty.attrs:
${union.vn_encode_union_tag(ty)}
%   endif
%   if 'need_decode' in ty.attrs:
${types.vn_decode_type(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_STRUCTS_H */
