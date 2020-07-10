/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace name="chain" file="/types_chain.h"/>
<%namespace name="handle" file="/types_handle.h"/>
<%namespace name="scalar" file="/types_scalar.h"/>
<%namespace name="struct" file="/types_struct.h"/>
<%namespace name="union" file="/types_union.h"/>

<%def name="vn_type_descriptive_name(ty)">\
% if ty.category == ty.DEFINE:
${ty.name}\
% elif ty.category == ty.BASETYPE:
typedef ${ty.typedef.name} ${ty.name}\
% elif ty.category == ty.ENUM:
enum ${ty.name}\
% elif ty.category == ty.HANDLE:
VK_DEFINE_HANDLE(${ty.name})\
% elif ty.category == ty.ND_HANDLE:
VK_DEFINE_NON_DISPATCHABLE_HANDLE(${ty.name})\
% elif ty.category == ty.UNION:
union ${ty.name}\
% elif ty.category == ty.STRUCT:
struct ${ty.name}${" chain" if ty.s_type else ""}\
% else:
<% assert(False) %>
% endif
</%def>

<%def name="vn_size_type(ty)">\
static inline size_t
vn_size_${ty.name}(const ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_size_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE]:
${handle.vn_size_handle_body(ty)}\
% elif ty.category == ty.UNION:
${union.vn_size_union_body(ty)}\
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_size_struct_body(ty)}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_size_chain_body(ty)}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_encode_type(ty)">\
static inline void
vn_encode_${ty.name}(struct vn_cs *cs, const ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_encode_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE]:
${handle.vn_encode_handle_body(ty)}\
% elif ty.category == ty.UNION:
${union.vn_encode_union_body(ty)}\
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_encode_struct_body(ty)}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_encode_chain_body(ty)}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_decode_type(ty)">\
static inline void
vn_decode_${ty.name}(struct vn_cs *cs, ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_decode_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE]:
${handle.vn_decode_handle_body(ty)}\
% elif ty.category == ty.UNION:
${union.vn_decode_union_body(ty)}\
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_decode_struct_body(ty)}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_decode_chain_body(ty)}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_decode_type_temp(ty)">\
static inline void
vn_decode_${ty.name}_temp(struct vn_cs *cs, ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_decode_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE] and not GEN.is_driver:
${handle.vn_decode_handle_body(ty, '_temp')}\
% elif ty.category == ty.UNION:
${union.vn_decode_union_body(ty, '_temp')}\
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_decode_struct_body(ty, '_temp')}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_decode_chain_temp_body(ty)}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_size_type_inout(ty)">\
static inline size_t
vn_size_${ty.name}_inout(const ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_size_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE]:
${handle.vn_size_handle_body(ty)}\
% elif ty.category == ty.UNION:
${union.vn_size_union_body(ty, '_inout')}\
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_size_struct_body(ty, '_inout')}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_size_chain_body(ty, '_inout')}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_encode_type_inout(ty)">\
static inline void
vn_encode_${ty.name}_inout(struct vn_cs *cs, const ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_encode_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE]:
${handle.vn_encode_handle_body(ty)}\
% elif ty.category == ty.UNION:
    assert(false); /* no user? */
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_encode_struct_body(ty, '_inout')}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_encode_chain_body(ty, '_inout')}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_decode_type_inout_temp(ty)">\
static inline void
vn_decode_${ty.name}_inout_temp(struct vn_cs *cs, ${ty.name} *val)
{
% if ty.category in [ty.DEFINE, ty.BASETYPE, ty.ENUM]:
${scalar.vn_decode_scalar_body(ty)}\
% elif ty.category in [ty.HANDLE, ty.ND_HANDLE]:
${handle.vn_decode_handle_body(ty, '_temp')}\
% elif ty.category == ty.UNION:
    assert(false); /* no user? */
% elif ty.category == ty.STRUCT and not ty.s_type:
${struct.vn_decode_struct_body(ty, '_inout_temp')}\
% elif ty.category == ty.STRUCT and ty.s_type:
${chain.vn_decode_chain_temp_body(ty, '_inout')}\
% else:
<% assert(False) %>
% endif
}
</%def>

<%def name="vn_replace_type_handle(ty)">\
static inline void
vn_replace_${ty.name}_handle(${ty.name} *val)
{
% if ty.category in [ty.HANDLE, ty.ND_HANDLE] and not GEN.is_driver:
${handle.vn_replace_handle_handle_body(ty)}\
% elif ty.category == ty.STRUCT and not ty.s_type and not GEN.is_driver:
${struct.vn_replace_struct_handle_body(ty)}\
% elif ty.category == ty.STRUCT and ty.s_type and not GEN.is_driver:
${chain.vn_replace_chain_handle_body(ty)}\
% else:
<% assert(False) %>
% endif
}
</%def>
