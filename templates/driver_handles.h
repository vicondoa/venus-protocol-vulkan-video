/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="encode_generic_array, decode_generic_array"/>\
\
<%def name="handle_is_dev(ty)">\
${"true" if ty.name == 'VkDevice' else "false"}\
</%def>\
\
<%def name="encode_handle(ty)">\
static inline void
vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
    const bool is_dev = ${handle_is_dev(ty)};
    const uint64_t id = vn_cs_handle_load_id((const void *)val, is_dev);
    vn_encode_uint64_t(cs, &id);
}
</%def>\
\
<%def name="decode_handle(ty)">\
static inline void
vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
    const bool is_dev = ${handle_is_dev(ty)};
    uint64_t id;
    vn_decode_uint64_t(cs, &id);
    vn_cs_handle_store_id((void *)val, id, is_dev);
}
</%def>\
\
#ifndef VN_PROTOCOL_DRIVER_HANDLES_H
#define VN_PROTOCOL_DRIVER_HANDLES_H

#include "vn_protocol_driver_types.h"

% for ty in HANDLE_TYPES:
/* VK_DEFINE_HANDLE(${ty.name}) */

${encode_handle(ty)}
${decode_handle(ty)}
%   if 'need_array' in ty.attrs:
${encode_generic_array(ty)}
${decode_generic_array(ty)}
%   endif
% endfor
\
% for ty in ND_HANDLE_TYPES:
/* VK_DEFINE_NON_DISPATCHABLE_HANDLE(${ty.name}) */

${encode_handle(ty)}
${decode_handle(ty)}
%   if 'need_array' in ty.attrs:
${encode_generic_array(ty)}
${decode_generic_array(ty)}
%   endif
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_HANDLES_H */
