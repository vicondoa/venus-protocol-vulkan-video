/*
 * Copyright 2026 the Venus Vulkan Video lab contributors
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Exhaustive round-trip and rejection tests for the H.264 flags wire packing.
 * The bit assignments are wire contract, so this asserts every combination
 * survives a round trip and that undefined bits are rejected rather than
 * silently masked.
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "vn_protocol_video_h264_flags.h"

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("  FAIL: %s\n", m); fails++; } } while (0)

int main(void) {
    /* round-trip every combination of the 6 picture-info bits */
    for (uint32_t v = 0; v <= VN_H264_PIC_FLAG_VALID_MASK; v++) {
        StdVideoDecodeH264PictureInfoFlags f;
        memset(&f, 0, sizeof f);
        CHECK(vn_unpack_StdVideoDecodeH264PictureInfoFlags(v, &f), "unpack valid");
        CHECK(vn_pack_StdVideoDecodeH264PictureInfoFlags(&f) == v, "pic round-trip");
    }
    /* and all 4 reference-info bits */
    for (uint32_t v = 0; v <= VN_H264_REF_FLAG_VALID_MASK; v++) {
        StdVideoDecodeH264ReferenceInfoFlags f;
        memset(&f, 0, sizeof f);
        CHECK(vn_unpack_StdVideoDecodeH264ReferenceInfoFlags(v, &f), "unpack valid");
        CHECK(vn_pack_StdVideoDecodeH264ReferenceInfoFlags(&f) == v, "ref round-trip");
    }
    /* unknown bits must be REJECTED, not masked */
    StdVideoDecodeH264PictureInfoFlags pf; memset(&pf, 0, sizeof pf);
    CHECK(!vn_unpack_StdVideoDecodeH264PictureInfoFlags(1u << 6, &pf), "reject pic bit 6");
    CHECK(!vn_unpack_StdVideoDecodeH264PictureInfoFlags(0xffffffffu, &pf), "reject pic all-ones");
    StdVideoDecodeH264ReferenceInfoFlags rf; memset(&rf, 0, sizeof rf);
    CHECK(!vn_unpack_StdVideoDecodeH264ReferenceInfoFlags(1u << 4, &rf), "reject ref bit 4");
    CHECK(!vn_unpack_StdVideoDecodeH264ReferenceInfoFlags(0xffffffffu, &rf), "reject ref all-ones");

    printf(fails ? "  %d FAILURES\n" : "  all flag pack/unpack tests passed\n", fails);
    return fails ? 1 : 0;
}
