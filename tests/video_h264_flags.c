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
    /* SPS / VUI / PPS flags: exhaustive over their defined bit ranges */
#define EXHAUSTIVE(Type, MASK, label)                                     \
    do {                                                                  \
        for (uint32_t v = 0; v <= (MASK); v++) {                          \
            Type f; memset(&f, 0, sizeof f);                              \
            CHECK(vn_unpack_##Type(v, &f), label " unpack valid");        \
            CHECK(vn_pack_##Type(&f) == v, label " round-trip");          \
        }                                                                 \
        Type f; memset(&f, 0, sizeof f);                                  \
        CHECK(!vn_unpack_##Type((MASK) + 1u, &f), label " reject next");  \
        CHECK(!vn_unpack_##Type(0xffffffffu, &f), label " reject ones");  \
    } while (0)

    EXHAUSTIVE(StdVideoH264SpsFlags,    VN_H264_SPS_FLAG_VALID_MASK, "sps");
    EXHAUSTIVE(StdVideoH264SpsVuiFlags, VN_H264_VUI_FLAG_VALID_MASK, "vui");
    EXHAUSTIVE(StdVideoH264PpsFlags,    VN_H264_PPS_FLAG_VALID_MASK, "pps");

    printf(fails ? "  %d FAILURES\n" : "  all flag pack/unpack tests passed\n", fails);
    return fails ? 1 : 0;
}
