/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%def name="vn_size_struct_body(ty, variant='')">\
    size_t size = 0;
%   for var in ty.variables:
    size += 256; /* TODO GEN.size_struct_member(ty, var, 'val->') */
%   endfor
    return size;
</%def>

<%def name="vn_encode_struct_body(ty, variant='')">\
<% skip_vars = 2 if '_self' in variant and ty.s_type else 0 %>\
% if skip_vars:
    /* skip val->{${','.join([var.name for var in ty.variables[:skip_vars]])}} */
% endif
% for var in ty.variables[skip_vars:]:
%   if '_inout' in variant:
    ${GEN.encode_struct_inout(ty, var, 'val->')}
%   else:
    ${GEN.encode_struct_member(ty, var, 'val->')}
%   endif
% endfor
</%def>

<%def name="vn_decode_struct_body(ty, variant='')">\
<% skip_vars = 2 if '_self' in variant and ty.s_type else 0 %>\
% if skip_vars:
    /* skip val->{${','.join([var.name for var in ty.variables[:skip_vars]])}} */
% endif
% for var in ty.variables[skip_vars:]:
%   if '_inout' in variant:
    ${GEN.decode_struct_inout(ty, var, 'val->', '_temp' in variant)}
%   else:
    ${GEN.decode_struct_member(ty, var, 'val->', '_temp' in variant)}
%   endif
% endfor
</%def>

<%def name="vn_replace_struct_handle_body(ty, variant='')">\
% for var in ty.variables:
    ${GEN.replace_struct_member_handle(ty, var, 'val->')}
% endfor
</%def>
