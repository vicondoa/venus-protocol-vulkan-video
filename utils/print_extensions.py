# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

from pathlib import Path

from vkxml import VkApi

VN_PROTOCOL_DIR = Path(__file__).parent.resolve()
VK_XML = VN_PROTOCOL_DIR.joinpath('xml/vk.xml')
VN_XML = VN_PROTOCOL_DIR.joinpath('xml/vn.xml')

def main():
    api = VkApi()
    api.parse_xml(VK_XML)
    api.parse_xml(VN_XML)
    api.validate()

    wsi = [
        'VK_KHR_display',
        'VK_KHR_surface',
        'VK_KHR_swapchain',
    ]

    exts = {}
    for ext in api.extensions:
        if ext.supported != 'vulkan':
            continue

        is_wsi = ext.name in wsi
        if not is_wsi and ext.requires:
            for req in ext.requires:
                if req in wsi:
                    wsi.append(ext.name)
                    is_wsi = True
                    break

        if is_wsi and ext.name.startswith('VK_KHR_'):
            key = 'WSI'
        else:
            key = ext.promoted

        if key not in exts:
            exts[key] = []
        exts[key].append(ext)

    for key in exts:
        exts[key].sort(key=lambda ext: ext.name)

        print(key)
        for ext in exts[key]:
            line = '    Extension(\'%s\', ' % ext.name
            ver = str(ext.version)
            pad = 59 - (len(line) + len(ver))
            if pad > 0:
                line += ' ' * pad
            line += ver
            line += ', False),'
            if ext.platform:
                line += ' # %s' % ext.platform

            print(line)

if __name__ == '__main__':
    main()
