/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="common" file="/common.h"/>\
\
<%def name="decode_pnext_chain_temp(ty, variant='')">\
static inline void vn_decode_${ty.name}${variant}_temp(struct vn_cs *cs, ${ty.name} *val)
{
    vn_decode_VkStructureType(cs, &val->sType);
    if (val->sType != ${ty.s_type}) {
        vn_cs_set_error(cs);
        return;
    }
    vn_decode_${ty.name}_self${variant}_temp(cs, val);

% if ty.p_next:
    VkBaseOutStructure *pcur = (VkBaseOutStructure *)val;
    do {
        VkBaseOutStructure *pnext;
<%
    next_types, skipped_types = GEN.get_pnext_chain(ty)
%>
        VkStructureType stype;
        vn_decode_VkStructureType(cs, &stype);
        switch (stype) {
%   for next_ty in next_types:
        case ${next_ty.s_type}:
            pnext = vn_cs_alloc_temp(cs, sizeof(${next_ty.name}));
            if (pnext) {
                pnext->sType = stype;
                vn_decode_${next_ty.name}_self${variant}_temp(cs, (${next_ty.name} *)pnext);
            }
            break;
%   endfor
        case VK_STRUCTURE_TYPE_MAX_ENUM:
            /* end-of-chain */
            pnext = NULL;
            break;
%   for skipped_ty in skipped_types:
        case ${skipped_ty.s_type}:
%   endfor
        default:
            /* unexpected struct */
            vn_cs_set_error(cs);
            pnext = NULL;
            break;
        }

        pcur->pNext = pnext;
        pcur = pnext;
    } while (pcur);
% else:
    val->pNext = NULL;
    vn_decode_end_of_chain(cs);
% endif
}
</%def>\
\
<%def name="replace_struct_handle(ty, variant='')">\
static inline void vn_replace_${ty.name}_handle${variant}(${ty.name} *val)
{
% for var in ty.variables:
    ${GEN.replace_struct_member_handle(ty, var, 'val->')}
% endfor
}
</%def>\
\
<%def name="replace_pnext_chain_handle(ty)">\
static inline void vn_replace_${ty.name}_handle(${ty.name} *val)
{
    struct VkBaseOutStructure *pnext = (struct VkBaseOutStructure *)val;
<%
    next_types, skipped_types = GEN.get_pnext_chain(ty)
%>
    do {
        switch (pnext->sType) {
% for next_ty in [ty] + next_types:
        case ${next_ty.s_type}:
            vn_replace_${next_ty.name}_handle_self((${next_ty.name} *)pnext);
            break;
% endfor
% for skipped_ty in skipped_types:
        case ${skipped_ty.s_type}:
% endfor
        default:
            /* ignore unknown/unsupported struct */
            break;
        }
        pnext = pnext->pNext;
    } while (pnext);
}
</%def>\
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
%   if ty.category == ty.STRUCT and not ty.s_type:
/* struct ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_struct(ty)}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_struct(ty, '_temp')}
%     endif
%     if 'need_inout' in ty.attrs:
${common.decode_struct(ty, '_inout_temp')}
%     endif
%     if 'need_decode' in ty.attrs:
${replace_struct_handle(ty)}
%     endif
%   elif ty.category == ty.STRUCT and ty.s_type:
/* struct ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_struct(ty, '_self')}
${common.encode_pnext_chain(ty)}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_struct(ty, '_self_temp')}
${decode_pnext_chain_temp(ty)}
%     endif
%     if 'need_inout' in ty.attrs:
${common.decode_struct(ty, '_self_inout_temp')}
${decode_pnext_chain_temp(ty, '_inout')}
%     endif
%     if 'need_decode' in ty.attrs:
${replace_struct_handle(ty, '_self')}
${replace_pnext_chain_handle(ty)}
%     endif
%   else:
/* union ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_union_tag(ty)}
${common.encode_union(ty, GEN.UNION_DEFAULT_TAGS[ty.name])}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_union(ty, '_temp')}
%     endif
%   endif
% endfor
/*
 * Helpers for manual serialization
 */

% for ty in MANUAL_UNION_TYPES:
/* union ${ty.name} */

%     if 'need_encode' in ty.attrs:
${common.encode_union_tag(ty)}
%     endif
%     if 'need_decode' in ty.attrs:
${common.decode_union(ty, '_temp')}
%     endif
% endfor
\
#endif /* VN_PROTOCOL_RENDERER_STRUCTS_H */
