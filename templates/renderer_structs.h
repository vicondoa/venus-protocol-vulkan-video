/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="types" file="/types.h"/>\
<%namespace name="array" file="/types_array.h"/>\
<%namespace name="chain" file="/types_chain.h"/>\
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
%     if ty.category == ty.UNION:
${union.vn_encode_union_tag(ty)}
%     elif ty.s_type:
${chain.vn_encode_chain_self(ty)}
%     endif
${types.vn_encode_type(ty)}
${array.vn_encode_type_array(ty)}
%   endif
\
%   if 'need_decode' in ty.attrs:
%     if ty.s_type:
${chain.vn_decode_chain_self(ty, '_temp')}
%     endif
${types.vn_decode_type_temp(ty)}
${array.vn_decode_type_array_temp(ty)}
%   endif
\
%   if 'need_inout' in ty.attrs and ty.category == ty.STRUCT:
%     if ty.s_type:
${chain.vn_decode_chain_self(ty, '_inout_temp')}
%     endif
${types.vn_decode_type_inout_temp(ty)}
${array.vn_decode_type_array_inout_temp(ty)}
%   endif
\
%   if 'need_decode' in ty.attrs and ty.category == ty.STRUCT:
%     if ty.s_type:
${chain.vn_replace_chain_handle_self(ty)}
%     endif
${types.vn_replace_type_handle(ty)}
${array.vn_replace_type_array_handle(ty)}
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
