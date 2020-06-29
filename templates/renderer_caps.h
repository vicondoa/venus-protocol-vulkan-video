/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="define_caps"/>\
\
#ifndef VN_PROTOCOL_RENDERER_CAPS_H
#define VN_PROTOCOL_RENDERER_CAPS_H

#include "vn_protocol_renderer_defines.h"

${define_caps(WIRE_FORMAT_VERSION, VN_XML_VERSION, VK_XML_VERSION, VK_XML_EXTENSION_TABLE)}
\
#endif /* VN_PROTOCOL_RENDERER_CAPS_H */
