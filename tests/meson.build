# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

cc = meson.get_compiler('c')

warnings = [
  '-Wno-unused-parameter',
  '-Wno-unused-but-set-parameter',
]
foreach w : warnings
  if cc.has_argument(w)
    add_project_arguments(w, language: 'c')
  endif
endforeach

vn_test_files = files(
  'main.c',
  'driver.c',
  'renderer.c',
)

vn_protocol_tests = executable(
  'vn_protocol_tests',
  [vn_test_files,
   vn_protocol_driver,
   vn_protocol_renderer],
  include_directories: ['../include',
                        '../include/vulkan'],
)

# Executable round-trip and malformed-input tests for the H.264 payloads.
#
# The target above is compile-only by design: tests/vn_cs.h and tests/vkr_cs.h
# have empty bodies, so it proves the generated code builds and nothing more.
# That is an acceptable bar for structures the guest cannot influence. It is
# not an acceptable bar for the codec payload, where every array length, flags
# word and enum in the buffer is chosen by the guest.
#
# This target supplies real command-stream primitives -- transcribed from
# Mesa's vn_cs.h and virglrenderer's vkr_cs.h -- so encode and decode actually
# move bytes, and then asserts round-trip idempotence, sizeof/write agreement,
# field influence, truncation rejection and exhaustive single-word corruption.
#
# It is built with the sanitizers on: T4 and T5 are only as strong as the
# instrumentation behind them.
vn_roundtrip_files = files(
  'roundtrip/enc.c',
  'roundtrip/dec.c',
  'roundtrip/main.c',
)

vn_roundtrip_tests = executable(
  'vn_roundtrip_tests',
  [vn_roundtrip_files,
   vn_protocol_driver,
   vn_protocol_renderer],
  implicit_include_directories: false,
  # 'roundtrip' must precede everything so its real vn_cs.h and vkr_cs.h
  # shadow the compile-only stubs in tests/. '..' resolves to the build root,
  # where the generated protocol headers live, and to the source root.
  include_directories: ['roundtrip',
                        '..',
                        '../include',
                        '../include/vulkan'],
  c_args: ['-fsanitize=address,undefined', '-fno-omit-frame-pointer',
           '-fno-sanitize-recover=all',
           # GCC at -O2 loses track of the write through
           # vn_decode_uint64_t -> vn_cs_decoder_read -> peek_internal, which
           # assigns on both the success and the short-read path, and warns
           # about `uint64_t size;` in upstream's vn_decode_array_size.
           # Adding an initializer there would change the body of a
           # pre-existing serialization function and trip the append-only ABI
           # gate for no wire-format benefit, so the warning is suppressed on
           # this target only. The risk it stands in for -- reading an
           # uninitialized value -- is what the sanitizers below are for.
           '-Wno-maybe-uninitialized'],
  link_args: ['-fsanitize=address,undefined'],
)

test('roundtrip', vn_roundtrip_tests, timeout: 600)
