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

vn_template_common_files = files(
  'templates/common.h',
  'templates/types.h',
  'templates/types_array.h',
  'templates/types_chain.h',
  'templates/types_custom.h',
  'templates/types_handle.h',
  'templates/types_scalar.h',
  'templates/types_struct.h',
  'templates/types_union.h',
)

driver_basenames = [
  'driver.h',
  'driver_calls.h',
  'driver_capset.h',
  'driver_commands.h',
  'driver_cs.h',
  'driver_defines.h',
  'driver_handles.h',
  'driver_structs.h',
  'driver_types.h',
]

renderer_basenames = [
  'renderer.h',
  'renderer_capset.h',
  'renderer_commands.h',
  'renderer_cs.h',
  'renderer_defines.h',
  'renderer_dispatches.h',
  'renderer_handles.h',
  'renderer_structs.h',
  'renderer_types.h',
]

vn_template_driver_files = []
vn_protocol_driver_outputs = []
foreach basename : driver_basenames
  vn_template_driver_files += files('templates/' + basename)
  vn_protocol_driver_outputs += 'vn_protocol_' + basename
endforeach

vn_template_renderer_files = []
vn_protocol_renderer_outputs = []
foreach basename : renderer_basenames
  vn_template_renderer_files += files('templates/' + basename)
  vn_protocol_renderer_outputs += 'vn_protocol_' + basename
endforeach

vn_protocol_driver = custom_target(
  'vn_protocol_driver',
  input: ['vn_protocol.py'],
  output: vn_protocol_driver_outputs,
  command: [
    py, '@INPUT0@', '--outdir', meson.current_build_dir(),
  ],
  depend_files: [
    vn_script_files,
    vn_xml_files,
    vn_template_common_files,
    vn_template_driver_files,
  ],
  build_by_default: true,
)

vn_protocol_renderer = custom_target(
  'vn_protocol_renderer',
  input: ['vn_protocol.py'],
  output: vn_protocol_renderer_outputs,
  command: [
    py, '@INPUT0@', '--outdir', meson.current_build_dir(),
    '--renderer',
  ],
  depend_files: [
    vn_script_files,
    vn_xml_files,
    vn_template_common_files,
    vn_template_renderer_files,
  ],
  build_by_default: true,
)

subdir('tests')
