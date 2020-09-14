/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="define_info"/>\
\
#ifndef VN_PROTOCOL_DRIVER_INFO_H
#define VN_PROTOCOL_DRIVER_INFO_H

#include "vn_protocol_driver_defines.h"

${define_info(WIRE_FORMAT_VERSION, VN_XML_VERSION, VK_XML_VERSION, EXTENSIONS)}
\
#endif /* VN_PROTOCOL_DRIVER_INFO_H */
