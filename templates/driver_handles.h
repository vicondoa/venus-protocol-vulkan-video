/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="handle_is_dev(ty)">\
${"true" if ty.name == 'VkDevice' else "false"}\
</%def>\
\
<%def name="encode_handle_object(ty)">\
static inline void
vn_encode_${ty.name}_object(struct vn_cs *cs, const ${ty.name} *val)
{
    const uint64_t id = vn_cs_object_get_id((const void *)val, ${handle_is_dev(ty)});
    vn_encode_uint64_t(cs, &id);
}
</%def>\
\
<%def name="decode_handle_object(ty)">\
static inline void
vn_decode_${ty.name}_object(struct vn_cs *cs, ${ty.name} *val)
{
    uint64_t id;
    vn_decode_uint64_t(cs, &id);

    vn_cs_object_set_id((void *)val, id, ${handle_is_dev(ty)});
}
</%def>\
\
#ifndef VN_PROTOCOL_DRIVER_HANDLES_H
#define VN_PROTOCOL_DRIVER_HANDLES_H

#include "vn_protocol_driver_types.h"

% for ty in HANDLE_TYPES:
/* VK_DEFINE_HANDLE(${ty.name}) */

${encode_handle_object(ty)}
${decode_handle_object(ty)}
% endfor
\
% for ty in ND_HANDLE_TYPES:
/* VK_DEFINE_NON_DISPATCHABLE_HANDLE(${ty.name}) */

${encode_handle_object(ty)}
${decode_handle_object(ty)}
% endfor
\
#endif /* VN_PROTOCOL_DRIVER_HANDLES_H */
