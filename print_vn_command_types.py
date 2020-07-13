# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

from pathlib import Path

from vkxml import VkApi
from vn_protocol import VK_XML_EXTENSION_LIST

VN_PROTOCOL_DIR = Path(__file__).parent.resolve()
VK_XML = VN_PROTOCOL_DIR.joinpath('xml/vk.xml')
VN_XML = VN_PROTOCOL_DIR.joinpath('xml/vn.xml')

def get_supported_commands(api):
    commands = []
    for cmd in api.venus.commands:
        if cmd not in commands:
            commands.append(cmd)
    for feat in api.vulkan:
        for cmd in feat.commands:
            if cmd not in commands:
                commands.append(cmd)
    for ext in api.extensions:
        if ext.name not in VK_XML_EXTENSION_LIST:
            continue
        for cmd in ext.commands:
            if cmd not in commands:
                commands.append(cmd)

    return commands

def main():
    api = VkApi()
    api.parse_xml(VK_XML)
    api.parse_xml(VN_XML)
    api.validate()

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
