/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="is_handle_VkDevice(ty)">\
<% assert(GEN.is_driver) %>\
${"true" if ty.name == 'VkDevice' else "false"}\
</%def>

<%def name="is_handle_in_place(ty)">\
<% assert(not GEN.is_driver) %>\
sizeof(*val) >= sizeof(vn_object_id)\
</%def>

<%def name="vn_decode_handle_lookup(ty)">\
<% assert(not GEN.is_driver) %>\
static inline void
vn_decode_${ty.name}_lookup(struct vn_cs_decoder *dec, ${ty.name} *val)
{
    uint64_t id;
    vn_decode_uint64_t(dec, &id);
% if ty.dispatchable:
    *val = (${ty.name})vn_cs_lookup_object(dec, id);
% else:
    *val = (${ty.name})(uintptr_t)vn_cs_lookup_object(dec, id);
% endif
}
</%def>

<%def name="vn_sizeof_handle_body(ty)">\
    return sizeof(uint64_t);
</%def>

<%def name="vn_encode_handle_body(ty)">\
% if GEN.is_driver and ty.name == 'VkDevice':
    const uint64_t id = vn_cs_device_load_id(val);
% elif GEN.is_driver:
    const uint64_t id = vn_cs_object_load_id((const void *)val);
% else:
    const bool in_place = ${is_handle_in_place(ty)};
    const uint64_t id = vn_cs_handle_load_id((const void *)val, in_place);
% endif
    vn_encode_uint64_t(cs, &id);
</%def>

<%def name="vn_decode_handle_body(ty, variant='')">\
    uint64_t id;
    vn_decode_uint64_t(dec, &id);
% if GEN.is_driver and ty.name == 'VkDevice':
    vn_cs_device_store_id(val, id);
% elif GEN.is_driver:
    vn_cs_object_store_id((void *)val, id);
% else:
    const bool in_place = ${is_handle_in_place(ty)};
%   if '_temp' in variant:
    if (!in_place) {
        *val = vn_cs_alloc_temp(dec, sizeof(vn_object_id));
        if (!val)
            return;
    }
%   endif
    vn_cs_handle_store_id((void *)val, id, in_place);
% endif
</%def>

<%def name="vn_replace_handle_handle_body(ty)">\
<% assert(not GEN.is_driver) %>\
% if ty.dispatchable:
    *val = (${ty.name})(uintptr_t)vn_cs_get_object_handle(val);
% else:
    *val = (${ty.name})vn_cs_get_object_handle(val);
% endif
</%def>
