/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="struct" file="/types_struct.h"/>

<%def name="vn_encode_chain_self(ty, variant='')">\
static inline void
vn_encode_${ty.name}_self${variant}(struct vn_cs *cs, const ${ty.name} *val)
{
${struct.vn_encode_struct_body(ty, '_self' + variant)}\
}
</%def>

<%def name="vn_decode_chain_self(ty, variant='')">\
static inline void
vn_decode_${ty.name}_self${variant}(struct vn_cs *cs, ${ty.name} *val)
{
${struct.vn_decode_struct_body(ty, '_self' + variant)}\
}
</%def>

<%def name="vn_replace_chain_handle_self(ty)">\
static inline void
vn_replace_${ty.name}_handle_self(${ty.name} *val)
{
${struct.vn_replace_struct_handle_body(ty, '_self')}\
}
</%def>

<%def name="vn_size_chain_body(ty, variant='')">\
    return 1024; /* TODO walk the chain */
</%def>

<%def name="vn_encode_chain_body(ty, variant='')">\
    const struct VkBaseInStructure *pnext = (const struct VkBaseInStructure *)val;
<%
    next_types, skipped_types = GEN.get_chain(ty)
%>
    do {
        switch (pnext->sType) {
% for next_ty in [ty] + next_types:
        case ${next_ty.s_type}:
            vn_encode_VkStructureType(cs, &pnext->sType);
            vn_encode_${next_ty.name}_self${variant}(cs, (const ${next_ty.name} *)pnext);
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

    vn_encode_end_of_chain(cs);
</%def>

<%def name="vn_decode_chain_body(ty)">\
    struct VkBaseOutStructure *pnext = (struct VkBaseOutStructure *)val;
<%
    next_types, skipped_types = GEN.get_chain(ty)
%>
    do {
        VkStructureType stype;
        vn_decode_VkStructureType(cs, &stype);

        while (pnext->sType != stype) {
            pnext = pnext->pNext;
            assert(pnext);
        }

        switch (stype) {
% for next_ty in [ty] + next_types:
        case ${next_ty.s_type}:
            vn_decode_${next_ty.name}_self(cs, (${next_ty.name} *)pnext);
            break;
% endfor
% for skipped_ty in skipped_types:
        case ${skipped_ty.s_type}:
% endfor
        default:
            /* unexpected struct */
            vn_cs_set_error(cs);
            break;
        }
        pnext = pnext->pNext;
    } while (pnext);

    vn_decode_end_of_chain(cs);
</%def>

<%def name="vn_decode_chain_temp_body(ty, variant='')">\
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
    next_types, skipped_types = GEN.get_chain(ty)
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
</%def>

<%def name="vn_replace_chain_handle_body(ty)">\
    struct VkBaseOutStructure *pnext = (struct VkBaseOutStructure *)val;
<%
    next_types, skipped_types = GEN.get_chain(ty)
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
</%def>
