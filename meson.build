# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

project(
  'venus-protocol',
  'c',
  version: '0.1',
  license: 'MIT',
  meson_version: '>=0.46.0',
  default_options: ['buildtype=debugoptimized',
                    'warning_level=2',
                    'werror=true',
                    'c_std=c11'],
)

py = import('python').find_installation('python3')

vn_script_files = files(
  'vkxml.py',
  'vn_protocol.py',
)

vn_xml_files = files(
  'xml/vk.xml',
  'xml/vn.xml',
)

vn_template_files = files(
  'templates/common.h',
  'templates/driver.h',
  'templates/driver_calls.h',
  'templates/driver_commands.h',
  'templates/driver_cs.h',
  'templates/driver_defines.h',
  'templates/driver_handles.h',
  'templates/driver_structs.h',
  'templates/driver_types.h',
  'templates/renderer.h',
  'templates/renderer_commands.h',
  'templates/renderer_cs.h',
  'templates/renderer_defines.h',
  'templates/renderer_dispatches.h',
  'templates/renderer_handles.h',
  'templates/renderer_structs.h',
  'templates/renderer_types.h',
)

vn_protocol_files = [
  vn_script_files,
  vn_xml_files,
  vn_template_files,
]

vn_protocol_driver = custom_target(
  'vn_protocol_driver',
  input: ['vn_protocol.py'],
  output: [
    'vn_protocol_driver.h',
    'vn_protocol_driver_calls.h',
    'vn_protocol_driver_commands.h',
    'vn_protocol_driver_cs.h',
    'vn_protocol_driver_defines.h',
    'vn_protocol_driver_handles.h',
    'vn_protocol_driver_structs.h',
    'vn_protocol_driver_types.h',
  ],
  command: [
    py, '@INPUT0@', '--outdir', meson.current_build_dir(),
  ],
  depend_files: [vn_protocol_files],
  build_by_default: true,
)

vn_protocol_renderer = custom_target(
  'vn_protocol_renderer',
  input: ['vn_protocol.py'],
  output: [
    'vn_protocol_renderer.h',
    'vn_protocol_renderer_commands.h',
    'vn_protocol_renderer_cs.h',
    'vn_protocol_renderer_defines.h',
    'vn_protocol_renderer_dispatches.h',
    'vn_protocol_renderer_handles.h',
    'vn_protocol_renderer_structs.h',
    'vn_protocol_renderer_types.h',
  ],
  command: [
    py, '@INPUT0@', '--outdir', meson.current_build_dir(),
    '--renderer',
  ],
  depend_files: [vn_protocol_files],
  build_by_default: true,
)

subdir('tests')
