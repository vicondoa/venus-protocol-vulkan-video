# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

project(
  'venus-protocol',
  'c',
  version: '0.1',
  license: 'MIT',
  meson_version: '>=0.51.0',
  default_options: ['buildtype=debugoptimized',
                    'warning_level=2',
                    'werror=true',
                    'c_std=c11'],
)

py = import('python').find_installation('python3', modules: ['mako'])

vn_script_files = files(
  'vkxml.py',
  'vn_protocol.py',
)

vn_xml_files = files(
  'xmls/vk.xml',
  'xmls/VK_EXT_command_serialization.xml',
  'xmls/VK_MESA_venus_protocol.xml',
)

vn_template_banner = vcs_tag(
  command: ['git', 'rev-parse', '--short=8', 'HEAD'],
  input: 'templates/banner.in',
  output: 'banner',
)

vn_template_common_files = files(
  'templates/common.h',
  'templates/types.h',
  'templates/types_chain.h',
  'templates/types_command.h',
  'templates/types_custom.h',
  'templates/types_handle.h',
  'templates/types_scalar.h',
  'templates/types_struct.h',
  'templates/types_union.h',
)

driver_base_headers = [
  'driver.h',
  'driver_calls.h',
  'driver_commands.h',
  'driver_cs.h',
  'driver_defines.h',
  'driver_handles.h',
  'driver_info.h',
  'driver_structs.h',
  'driver_types.h',
]

renderer_base_headers = [
  'renderer.h',
  'renderer_commands.h',
  'renderer_cs.h',
  'renderer_defines.h',
  'renderer_dispatches.h',
  'renderer_handles.h',
  'renderer_info.h',
  'renderer_structs.h',
  'renderer_types.h',
]

vn_template_driver_files = []
vn_protocol_driver_outputs = []
foreach basename : driver_base_headers
  vn_template_driver_files += files('templates/' + basename)
  vn_protocol_driver_outputs += 'vn_protocol_' + basename
endforeach

vn_template_renderer_files = []
vn_protocol_renderer_outputs = []
foreach basename : renderer_base_headers
  vn_template_renderer_files += files('templates/' + basename)
  vn_protocol_renderer_outputs += 'vn_protocol_' + basename
endforeach

vn_protocol_driver = custom_target(
  'vn_protocol_driver',
  input: ['vn_protocol.py'],
  output: vn_protocol_driver_outputs,
  command: [
    py, '@INPUT0@', '--outdir', meson.current_build_dir(),
    '--banner', vn_template_banner.full_path(),
  ],
  depends: vn_template_banner,
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
    '--banner', vn_template_banner.full_path(), '--renderer',
  ],
  depends: vn_template_banner,
  depend_files: [
    vn_script_files,
    vn_xml_files,
    vn_template_common_files,
    vn_template_renderer_files,
  ],
  build_by_default: true,
)

subdir('tests')
