/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="define_info"/>\
\
#ifndef VN_PROTOCOL_RENDERER_INFO_H
#define VN_PROTOCOL_RENDERER_INFO_H

#include "vn_protocol_renderer_defines.h"

${define_info(WIRE_FORMAT_VERSION, VN_XML_VERSION, VK_XML_VERSION, EXTENSIONS)}
\
#endif /* VN_PROTOCOL_RENDERER_INFO_H */
