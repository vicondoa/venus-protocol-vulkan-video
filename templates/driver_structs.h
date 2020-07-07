/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="common" file="/common.h"/>\
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
%   if ty.category == ty.STRUCT and not ty.s_type:
/* struct ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_struct(ty)}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_struct(ty)}
%     endif
%     if 'need_inout' in ty.attrs:
${common.encode_struct(ty, '_inout')}
%     endif
%   elif ty.category == ty.STRUCT and ty.s_type:
/* struct ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_struct(ty, '_self')}
${common.encode_pnext_chain(ty)}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_struct(ty, '_self')}
${common.decode_pnext_chain(ty)}
%     endif
%     if 'need_inout' in ty.attrs:
${common.encode_struct(ty, '_self_inout')}
${common.encode_pnext_chain(ty, '_inout')}
%     endif
%   else:
/* union ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_union_tag(ty)}
${common.encode_union(ty, GEN.UNION_DEFAULT_TAGS[ty.name])}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_union(ty)}
%     endif
%   endif
% endfor
/*
 * Helpers for manual serialization
 */

% for ty in MANUAL_UNION_TYPES:
/* union ${ty.name} */

%   if 'need_encode' in ty.attrs:
${common.encode_union_tag(ty)}
%   endif
%   if 'need_decode' in ty.attrs:
${common.decode_union(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_STRUCTS_H */
