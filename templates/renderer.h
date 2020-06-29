/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_PROTOCOL_RENDERER_H
#define VN_PROTOCOL_RENDERER_H

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wunused-parameter"

% for filename in TEMPLATE_FILENAMES:
#include "vn_protocol_${filename}"
% endfor

#pragma GCC diagnostic pop

#endif /* VN_PROTOCOL_RENDERER_H */
