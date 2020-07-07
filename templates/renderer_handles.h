/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="encode_generic_array, decode_generic_array, replace_generic_array_handle"/>\
\
<%def name="handle_is_in_place(ty)">\
sizeof(${ty.name}) >= sizeof(vn_cs_object_id)\
</%def>\
\
<%def name="encode_handle(ty)">\
static inline void
vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
    const bool in_place = ${handle_is_in_place(ty)};
    const uint64_t id = vn_cs_handle_load_id((const void *)val, in_place);
    vn_encode_uint64_t(cs, &id);
}
</%def>\
\
<%def name="decode_handle(ty)">\
static inline void
vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
    const bool in_place = ${handle_is_in_place(ty)};
    uint64_t id;
    vn_decode_uint64_t(cs, &id);
    vn_cs_handle_store_id((void *)val, id, in_place);
}
</%def>\
\
<%def name="decode_handle_lookup(ty)">\
static inline void
vn_decode_${ty.name}_lookup(struct vn_cs *cs, ${ty.name} *val)
{
    uint64_t id;
    vn_decode_uint64_t(cs, &id);
    *val = (${ty.name})vn_cs_lookup_object(cs, id);
}
</%def>\
\
<%def name="decode_handle_temp(ty)">\
static inline void
vn_decode_${ty.name}_temp(struct vn_cs *cs, ${ty.name} *val)
{
    const bool in_place = ${handle_is_in_place(ty)};
    if (!in_place) {
        *val = vn_cs_alloc_temp(cs, sizeof(vn_cs_object_id));
        if (!val)
            return;
    }
    vn_decode_${ty.name}(cs, val);
}
</%def>\
\
<%def name="replace_handle(ty)">\
static inline void
vn_replace_${ty.name}_handle(${ty.name} *val)
{
    *val = (${ty.name})vn_cs_get_object_handle(val);
}
</%def>\
\
#ifndef VN_PROTOCOL_RENDERER_HANDLES_H
#define VN_PROTOCOL_RENDERER_HANDLES_H

#include "vn_protocol_renderer_types.h"

% for ty in HANDLE_TYPES:
/* VK_DEFINE_HANDLE(${ty.name}) */

${encode_handle(ty)}
${decode_handle(ty)}
${decode_handle_lookup(ty)}
${decode_handle_temp(ty)}
${replace_handle(ty)}
%   if 'need_array' in ty.attrs:
${encode_generic_array(ty)}
${decode_generic_array(ty)}
${decode_generic_array(ty, '_lookup')}
${decode_generic_array(ty, '_temp')}
${replace_generic_array_handle(ty)}
%   endif
% endfor
\
% for ty in ND_HANDLE_TYPES:
/* VK_DEFINE_NON_DISPATCHABLE_HANDLE(${ty.name}) */

${encode_handle(ty)}
${decode_handle(ty)}
${decode_handle_lookup(ty)}
${replace_handle(ty)}
%   if 'need_array' in ty.attrs:
${encode_generic_array(ty)}
${decode_generic_array(ty)}
${decode_generic_array(ty, '_lookup')}
${replace_generic_array_handle(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_RENDERER_HANDLES_H */
