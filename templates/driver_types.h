/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="common" file="/common.h"/>\
\
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
#ifndef VN_PROTOCOL_DRIVER_TYPES_H
#define VN_PROTOCOL_DRIVER_TYPES_H

#include "vn_protocol_driver_defines.h"

% for ty, size in SCALAR_TYPES:
/* ${ty.name} */

${common.encode_scalar(ty, size)}
${common.decode_scalar(ty, size)}
%   if 'need_array' in ty.attrs:
${common.encode_scalar_array(ty, size)}
${common.decode_scalar_array(ty, size)}
%   endif
% endfor
\
% for ty in TYPEDEF_TYPES:
/* typedef ${ty.typedef.name} ${ty.name} */

${common.encode_typedef(ty)}
${common.decode_typedef(ty)}
%   if 'need_array' in ty.attrs:
${common.encode_typedef_array(ty)}
${common.decode_typedef_array(ty)}
%   endif
% endfor
\
% for ty in ENUM_TYPES:
/* enum ${ty.name} */

${common.encode_enum(ty)}
${common.decode_enum(ty)}
% endfor
\
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
${common.encode_decode_special()}
\
/* TODO remove this and all callers */
static inline void
vn_decode_string(struct vn_cs *cs, const char *val)
{
    vn_cs_set_error(cs);
}

#endif /* VN_PROTOCOL_DRIVER_TYPES_H */
