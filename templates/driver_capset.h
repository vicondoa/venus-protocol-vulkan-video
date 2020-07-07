/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

<%namespace file="/common.h" import="define_capset"/>\
\
#ifndef VN_PROTOCOL_DRIVER_CAPSET_H
#define VN_PROTOCOL_DRIVER_CAPSET_H

#include "vn_protocol_driver_defines.h"

${define_capset(WIRE_FORMAT_VERSION, VN_XML_VERSION, VK_XML_VERSION, VK_XML_EXTENSION_TABLE)}
\
#endif /* VN_PROTOCOL_DRIVER_CAPSET_H */
