#!/usr/bin/env python3

# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

from pathlib import Path
import sys

VN_PROTOCOL_DIR = Path(__file__).resolve().parent.parent
sys.path.append(str(VN_PROTOCOL_DIR))

from vkxml import VkApi
from vn_protocol import VN_PROTOCOL_XMLS, VK_XML_EXTENSION_LIST

def get_supported_commands(api):
    commands = []
    for ty in api.venus.types:
        if ty.category == ty.COMMAND and ty not in commands:
            commands.append(ty)
    for feat in api.features:
        for ty in feat.types:
            if ty.category == ty.COMMAND and ty not in commands:
                commands.append(ty)
    for ext in api.extensions:
        if ext.name not in VK_XML_EXTENSION_LIST:
            continue
        for ty in ext.types:
            if ty.category == ty.COMMAND and ty not in commands:
                commands.append(ty)

    return commands

def main():
    api = VkApi()
    api.parse_xmls(VN_PROTOCOL_XMLS)

    vn_command_type_ty = api.type_table['VnCommandType']
    next_id = api.max_vn_command_type_value + 1
    commands = get_supported_commands(api)

    enums = []
    for cmd in commands:
        key = 'VN_COMMAND_TYPE_%s' % cmd.name
        if key in vn_command_type_ty.enums.values:
            val = vn_command_type_ty.enums.values[key]
        else:
            val = next_id
            next_id += 1
        enums.append((key, val))

    print('    <enums name="%s" type="enum">' % vn_command_type_ty.name)
    for key, val in enums:
        print('        <enum value="%s" name="%s"/>' % (val, key))
    print('    </enums>')

if __name__ == '__main__':
    main()
