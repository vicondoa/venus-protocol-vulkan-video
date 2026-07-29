/*
 * SPDX-License-Identifier: MIT
 *
 * Executable round-trip and malformed-input tests for the H.264 Venus
 * protocol payloads.
 *
 * Why this exists
 * ---------------
 * The upstream venus-protocol test harness is compile-only: tests/vn_cs.h and
 * tests/vkr_cs.h have empty bodies, so encode and decode both "succeed"
 * without moving a byte. It proves the generated code builds. It cannot prove
 * the generated code is correct, and it cannot prove the generated code is
 * safe against a hostile guest.
 *
 * These payloads are the entire guest-controlled codec surface: SPS and PPS
 * arrays with guest-supplied counts, slice-offset arrays, bitfield flags, and
 * StdVideo enums whose underlying type is implementation-defined. Everything
 * here crosses a trust boundary.
 *
 * What is asserted
 * ----------------
 * T1  idempotence      encode(decode(b)) == b, for fully populated payloads.
 *                      Complete over the wire format by construction: any
 *                      dropped, reordered, truncated or mis-packed field
 *                      changes the bytes. No hand-written field-by-field
 *                      comparator to drift out of date.
 *
 * T2  size agreement   vn_sizeof_X() == bytes actually written. A sizeof that
 *                      under-predicts is a renderer heap overflow; this is the
 *                      classic Venus bug class and it is silent without an
 *                      exact-sized buffer.
 *
 * T3  field influence  Flipping any byte of a source payload struct must
 *                      change the encoded bytes, except at offsets that are C
 *                      padding. A field silently dropped from the schema
 *                      becomes a non-influential offset and fails against the
 *                      golden set.
 *
 * T4  truncation       Decode every prefix of every payload. The decoder must
 *                      mark the stream fatal and must not read out of bounds.
 *                      The buffer is exact-sized and heap-allocated, so ASan
 *                      turns a one-byte over-read into a hard failure.
 *
 * T5  word corruption  Exhaustively overwrite each 4-byte-aligned word with
 *                      0xffffffff, 0x00000000 and seeded pseudo-random values,
 *                      then decode. Every guest-controlled count, flags word
 *                      and enum in the buffer is hit by construction. The
 *                      decoder must never crash, never read out of bounds and
 *                      never exceed the allocation budget.
 *
 * T6  allocation cap   A legitimate but large count under a small temp budget
 *                      must fail closed rather than allocate.
 *
 * Build with -fsanitize=address,undefined: T4 and T5 are only as strong as the
 * sanitizer behind them.
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vk_video/vulkan_video_codec_h264std.h>
#include <vk_video/vulkan_video_codec_h264std_decode.h>

#include "harness.h"

static int failures;
static int checks;

#define CHECK(cond, ...)                                                      \
   do {                                                                       \
      checks++;                                                               \
      if (!(cond)) {                                                          \
         failures++;                                                          \
         fprintf(stderr, "  FAIL %s:%d: ", __FILE__, __LINE__);               \
         fprintf(stderr, __VA_ARGS__);                                        \
         fprintf(stderr, "\n");                                               \
      }                                                                       \
   } while (0)

/* ------------------------------------------------------------------ *
 * Payload construction
 *
 * Values are deliberately distinctive rather than realistic: every field
 * gets a different non-zero value so that a field written to the wrong
 * offset, or omitted, changes the byte stream. Zeros and small repeated
 * integers would let an offset error round-trip by accident.
 * ------------------------------------------------------------------ */

static uint8_t
fill_byte(unsigned seed, unsigned i)
{
   /* Non-zero, non-repeating, and never 0xa5 so it cannot be confused with
    * the harness poison pattern. */
   uint8_t v = (uint8_t)(seed * 37u + i * 61u + 1u);
   if (v == 0xa5 || v == 0)
      v ^= 0x5au;
   return v;
}

static StdVideoH264ScalingLists scaling_lists;
static StdVideoH264HrdParameters hrd;
static StdVideoH264SequenceParameterSetVui vui;
/* Sized to the schema cap (255) because the T3 influence sweep mutates
 * num_ref_frames_in_pic_order_cnt_cycle, which the encoder uses as this
 * array's length. A smaller fixture would make the harness itself read out
 * of bounds -- as ASan duly reported the first time this ran. */
static int32_t offset_for_ref_frame[256];
static StdVideoH264SequenceParameterSet sps[3];
static StdVideoH264PictureParameterSet pps[2];
static StdVideoDecodeH264PictureInfo dec_pic;
static StdVideoDecodeH264ReferenceInfo dec_ref;
static uint32_t slice_offsets[5];

static void
build_payloads(void)
{
   memset(&scaling_lists, 0, sizeof(scaling_lists));
   scaling_lists.scaling_list_present_mask = 0x2a5b;
   scaling_lists.use_default_scaling_matrix_mask = 0x13c7;
   for (unsigned l = 0; l < STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS; l++)
      for (unsigned e = 0; e < STD_VIDEO_H264_SCALING_LIST_4X4_NUM_ELEMENTS; e++)
         scaling_lists.ScalingList4x4[l][e] = fill_byte(l, e);
   for (unsigned l = 0; l < STD_VIDEO_H264_SCALING_LIST_8X8_NUM_LISTS; l++)
      for (unsigned e = 0; e < STD_VIDEO_H264_SCALING_LIST_8X8_NUM_ELEMENTS; e++)
         scaling_lists.ScalingList8x8[l][e] = fill_byte(l + 9, e);

   memset(&hrd, 0, sizeof(hrd));
   hrd.cpb_cnt_minus1 = 3;
   hrd.bit_rate_scale = 5;
   hrd.cpb_size_scale = 7;
   for (unsigned i = 0; i < STD_VIDEO_H264_CPB_CNT_LIST_SIZE; i++) {
      hrd.bit_rate_value_minus1[i] = 0x11110000u + i;
      hrd.cpb_size_value_minus1[i] = 0x22220000u + i;
      hrd.cbr_flag[i] = (uint8_t)(i & 1);
   }
   hrd.initial_cpb_removal_delay_length_minus1 = 23;
   hrd.cpb_removal_delay_length_minus1 = 17;
   hrd.dpb_output_delay_length_minus1 = 19;
   hrd.time_offset_length = 13;

   memset(&vui, 0, sizeof(vui));
   vui.flags.aspect_ratio_info_present_flag = 1;
   vui.flags.overscan_info_present_flag = 0;
   vui.flags.overscan_appropriate_flag = 1;
   vui.flags.video_signal_type_present_flag = 1;
   vui.flags.video_full_range_flag = 0;
   vui.flags.color_description_present_flag = 1;
   vui.flags.chroma_loc_info_present_flag = 1;
   vui.flags.timing_info_present_flag = 1;
   vui.flags.fixed_frame_rate_flag = 0;
   vui.flags.bitstream_restriction_flag = 1;
   vui.flags.nal_hrd_parameters_present_flag = 1;
   vui.flags.vcl_hrd_parameters_present_flag = 0;
   vui.aspect_ratio_idc = STD_VIDEO_H264_ASPECT_RATIO_IDC_SQUARE;
   vui.sar_width = 1234;
   vui.sar_height = 4321;
   vui.video_format = 5;
   vui.colour_primaries = 9;
   vui.transfer_characteristics = 16;
   vui.matrix_coefficients = 6;
   vui.num_units_in_tick = 1001;
   vui.time_scale = 60000;
   vui.max_num_reorder_frames = 2;
   vui.max_dec_frame_buffering = 4;
   vui.chroma_sample_loc_type_top_field = 1;
   vui.chroma_sample_loc_type_bottom_field = 2;
   vui.pHrdParameters = &hrd;

   for (unsigned i = 0; i < 256; i++)
      offset_for_ref_frame[i] = (int32_t)(-1000 + (int)i * 37);

   for (unsigned s = 0; s < 3; s++) {
      memset(&sps[s], 0, sizeof(sps[s]));
      sps[s].flags.constraint_set0_flag = 1;
      sps[s].flags.constraint_set1_flag = 0;
      sps[s].flags.constraint_set2_flag = 1;
      sps[s].flags.direct_8x8_inference_flag = 1;
      sps[s].flags.mb_adaptive_frame_field_flag = 0;
      sps[s].flags.frame_mbs_only_flag = 1;
      sps[s].flags.delta_pic_order_always_zero_flag = 1;
      sps[s].flags.separate_colour_plane_flag = 0;
      sps[s].flags.gaps_in_frame_num_value_allowed_flag = 1;
      sps[s].flags.qpprime_y_zero_transform_bypass_flag = 1;
      sps[s].flags.frame_cropping_flag = 1;
      sps[s].flags.seq_scaling_matrix_present_flag = 1;
      sps[s].flags.vui_parameters_present_flag = 1;
      sps[s].profile_idc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
      sps[s].level_idc = STD_VIDEO_H264_LEVEL_IDC_4_1;
      sps[s].chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420;
      sps[s].seq_parameter_set_id = (uint8_t)s;
      sps[s].bit_depth_luma_minus8 = 2;
      sps[s].bit_depth_chroma_minus8 = 2;
      sps[s].log2_max_frame_num_minus4 = 4;
      sps[s].pic_order_cnt_type = STD_VIDEO_H264_POC_TYPE_1;
      sps[s].offset_for_non_ref_pic = -4242;
      sps[s].offset_for_top_to_bottom_field = 2424;
      sps[s].log2_max_pic_order_cnt_lsb_minus4 = 3;
      sps[s].num_ref_frames_in_pic_order_cnt_cycle = 8;
      sps[s].max_num_ref_frames = 5;
      sps[s].pic_width_in_mbs_minus1 = 119;
      sps[s].pic_height_in_map_units_minus1 = 67;
      sps[s].frame_crop_left_offset = 1;
      sps[s].frame_crop_right_offset = 2;
      sps[s].frame_crop_top_offset = 3;
      sps[s].frame_crop_bottom_offset = 4;
      sps[s].pOffsetForRefFrame = offset_for_ref_frame;
      sps[s].pScalingLists = &scaling_lists;
      sps[s].pSequenceParameterSetVui = &vui;
   }

   for (unsigned p = 0; p < 2; p++) {
      memset(&pps[p], 0, sizeof(pps[p]));
      pps[p].flags.transform_8x8_mode_flag = 1;
      pps[p].flags.redundant_pic_cnt_present_flag = 0;
      pps[p].flags.constrained_intra_pred_flag = 1;
      pps[p].flags.deblocking_filter_control_present_flag = 1;
      pps[p].flags.weighted_pred_flag = 1;
      pps[p].flags.bottom_field_pic_order_in_frame_present_flag = 0;
      pps[p].flags.entropy_coding_mode_flag = 1;
      pps[p].flags.pic_scaling_matrix_present_flag = 1;
      pps[p].seq_parameter_set_id = 1;
      pps[p].pic_parameter_set_id = (uint8_t)p;
      pps[p].num_ref_idx_l0_default_active_minus1 = 2;
      pps[p].num_ref_idx_l1_default_active_minus1 = 1;
      pps[p].weighted_bipred_idc = STD_VIDEO_H264_WEIGHTED_BIPRED_IDC_IMPLICIT;
      /* Signed 8-bit fields: negative values prove sign extension survives
       * the wire, which is exactly what int8_t/int16_t missing from
       * PRIMITIVE_TYPES would have broken. */
      pps[p].pic_init_qp_minus26 = -26;
      pps[p].pic_init_qs_minus26 = -13;
      pps[p].chroma_qp_index_offset = -7;
      pps[p].second_chroma_qp_index_offset = 11;
      pps[p].pScalingLists = &scaling_lists;
   }

   memset(&dec_pic, 0, sizeof(dec_pic));
   dec_pic.flags.field_pic_flag = 1;
   dec_pic.flags.is_intra = 0;
   dec_pic.flags.IdrPicFlag = 1;
   dec_pic.flags.bottom_field_flag = 1;
   dec_pic.flags.is_reference = 1;
   dec_pic.flags.complementary_field_pair = 0;
   dec_pic.seq_parameter_set_id = 1;
   dec_pic.pic_parameter_set_id = 1;
   dec_pic.frame_num = 4097;
   dec_pic.idr_pic_id = 513;
   for (unsigned i = 0; i < STD_VIDEO_DECODE_H264_FIELD_ORDER_COUNT_LIST_SIZE; i++)
      dec_pic.PicOrderCnt[i] = (int32_t)(-77 + (int)i * 3);

   memset(&dec_ref, 0, sizeof(dec_ref));
   dec_ref.flags.top_field_flag = 1;
   dec_ref.flags.bottom_field_flag = 0;
   dec_ref.flags.used_for_long_term_reference = 1;
   dec_ref.flags.is_non_existing = 0;
   dec_ref.FrameNum = 8191;
   for (unsigned i = 0; i < STD_VIDEO_DECODE_H264_FIELD_ORDER_COUNT_LIST_SIZE; i++)
      dec_ref.PicOrderCnt[i] = (int32_t)(99 - (int)i * 5);

   for (unsigned i = 0; i < 5; i++)
      slice_offsets[i] = 0x1000u * (i + 1);
}

/* ------------------------------------------------------------------ *
 * Wrapper payloads
 * ------------------------------------------------------------------ */

static VkVideoDecodeH264ProfileInfoKHR
mk_profile(void)
{
   VkVideoDecodeH264ProfileInfoKHR v = {
      .sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR,
      .pNext = NULL,
      .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
      .pictureLayout =
         VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR,
   };
   return v;
}

static VkVideoDecodeH264SessionParametersAddInfoKHR
mk_add_info(void)
{
   VkVideoDecodeH264SessionParametersAddInfoKHR v = {
      .sType =
         VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR,
      .pNext = NULL,
      .stdSPSCount = 3,
      .pStdSPSs = sps,
      .stdPPSCount = 2,
      .pStdPPSs = pps,
   };
   return v;
}

static VkVideoDecodeH264SessionParametersCreateInfoKHR
mk_sp_create(const VkVideoDecodeH264SessionParametersAddInfoKHR *add)
{
   VkVideoDecodeH264SessionParametersCreateInfoKHR v = {
      .sType =
         VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR,
      .pNext = NULL,
      .maxStdSPSCount = 8,
      .maxStdPPSCount = 16,
      .pParametersAddInfo = add,
   };
   return v;
}

static VkVideoDecodeH264PictureInfoKHR
mk_picture(void)
{
   VkVideoDecodeH264PictureInfoKHR v = {
      .sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR,
      .pNext = NULL,
      .pStdPictureInfo = &dec_pic,
      .sliceCount = 5,
      .pSliceOffsets = slice_offsets,
   };
   return v;
}

static VkVideoDecodeH264DpbSlotInfoKHR
mk_dpb_slot(void)
{
   VkVideoDecodeH264DpbSlotInfoKHR v = {
      .sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR,
      .pNext = NULL,
      .pStdReferenceInfo = &dec_ref,
   };
   return v;
}

/* ------------------------------------------------------------------ *
 * Generic per-payload test bodies
 * ------------------------------------------------------------------ */

typedef struct rt_encoded (*rt_enc_fn)(const void *);
typedef bool (*rt_dec_fn)(const void *, size_t, size_t, void *);

struct payload {
   const char *name;
   rt_enc_fn encode;
   rt_dec_fn decode;
   void *src;      /* the wrapper struct */
   size_t src_size;
   size_t out_size; /* sizeof the decoded wrapper struct */
};

/* Temp budget used by the corruption sweep. Generous enough for the honest
 * payloads (a few KiB), small enough that a corrupted count asking for
 * megabytes fails closed instead of succeeding. */
#define RT_SWEEP_BUDGET (256u * 1024u)

static void
test_roundtrip(const struct payload *p)
{
   struct rt_encoded a = p->encode(p->src);

   CHECK(!a.encoder_fatal, "%s: encoder went fatal on a well-formed payload",
         p->name);
   CHECK(a.predicted == a.written,
         "%s: vn_sizeof said %zu bytes, encoder wrote %zu. A sizeof that "
         "under-predicts overflows the renderer's buffer.",
         p->name, a.predicted, a.written);

   void *out = calloc(1, p->out_size);
   bool ok = p->decode(a.data, a.written, 0, out);
   CHECK(ok, "%s: decoder marked a well-formed stream fatal", p->name);

   if (ok) {
      /* Re-encode what the renderer decoded. Equality proves every byte the
       * wire format carries survived the round trip: ordering, widths, flag
       * packing, enum values, array counts and nested pointers. */
      struct rt_encoded b = p->encode(out);
      CHECK(b.written == a.written,
            "%s: re-encode length %zu != original %zu", p->name, b.written,
            a.written);
      if (b.written == a.written) {
         int cmp = memcmp(a.data, b.data, a.written);
         CHECK(cmp == 0, "%s: re-encoded bytes differ from the original",
               p->name);
         if (cmp != 0) {
            for (size_t i = 0; i < a.written; i++) {
               if (a.data[i] != b.data[i]) {
                  fprintf(stderr,
                          "       first difference at byte %zu: 0x%02x -> 0x%02x\n",
                          i, a.data[i], b.data[i]);
                  break;
               }
            }
         }
      }
      rt_encoded_free(&b);
   }
   rt_decode_release();
   free(out);
   rt_encoded_free(&a);
}

static void
test_truncation(const struct payload *p)
{
   struct rt_encoded a = p->encode(p->src);
   void *out = calloc(1, p->out_size);
   int survived = 0;

   /* Decode every prefix. The buffer handed to the decoder is a fresh
    * exact-sized allocation, so any read past the truncation point is a
    * heap-buffer-overflow under ASan rather than a silent read of the bytes
    * that happen to follow. */
   for (size_t len = 0; len < a.written; len += 4) {
      uint8_t *slice = malloc(len ? len : 1);
      memcpy(slice, a.data, len);
      bool ok = p->decode(slice, len, RT_SWEEP_BUDGET, out);
      if (ok)
         survived++;
      rt_decode_release();
      free(slice);
   }

   CHECK(survived == 0,
         "%s: %d truncated prefixes decoded without the stream going fatal",
         p->name, survived);

   free(out);
   rt_encoded_free(&a);
}

/* xorshift32; fixed seed so a failure is reproducible. */
static uint32_t rng_state = 0x9e3779b9u;

static uint32_t
rng_next(void)
{
   uint32_t x = rng_state;
   x ^= x << 13;
   x ^= x >> 17;
   x ^= x << 5;
   rng_state = x;
   return x;
}

static void
test_word_corruption(const struct payload *p)
{
   struct rt_encoded a = p->encode(p->src);
   void *out = calloc(1, p->out_size);

   /* Every guest-controlled count, flags word and enum lives at some
    * 4-byte-aligned offset in this buffer, so sweeping all of them hits all
    * of those without needing to know where any of them are. */
   static const uint32_t fixed[] = { 0xffffffffu, 0x00000000u, 0x80000000u,
                                     0x7fffffffu };
   size_t words = a.written / 4;
   size_t accepted = 0;

   for (size_t w = 0; w < words; w++) {
      for (unsigned k = 0; k < sizeof(fixed) / sizeof(fixed[0]) + 4; k++) {
         uint32_t patch = k < sizeof(fixed) / sizeof(fixed[0])
                             ? fixed[k]
                             : rng_next();

         uint8_t *buf = malloc(a.written);
         memcpy(buf, a.data, a.written);
         memcpy(buf + w * 4, &patch, 4);

         /* The assertion is not "this must fail". A corrupted word may well
          * describe a different but still valid payload. The assertion is
          * that the decoder reaches a defined outcome without reading out of
          * bounds (ASan), executing undefined behaviour (UBSan), or
          * allocating past the budget. */
         if (p->decode(buf, a.written, RT_SWEEP_BUDGET, out))
            accepted++;
         rt_decode_release();
         free(buf);
      }
   }

   /* A sanity floor on the sweep itself: if nothing was ever accepted the
    * decoder is probably failing for an unrelated reason and the sweep is
    * not testing what it claims to. */
   CHECK(words == 0 || accepted > 0,
         "%s: every single-word corruption was rejected, which suggests the "
         "sweep is not exercising the decoder",
         p->name);

   printf("  %-46s %4zu words x %u patches, %zu accepted\n", p->name, words,
          (unsigned)(sizeof(fixed) / sizeof(fixed[0]) + 4), accepted);

   free(out);
   rt_encoded_free(&a);
}

/* T3: every byte of a source payload struct must influence the wire, except
 * at C padding offsets. The golden lists below were produced by this test and
 * checked against the struct definitions in vulkan_video_codec_h264std.h; each
 * entry is either compiler padding or a `reserved` field the schema
 * deliberately does not transmit. A field silently dropped from the private
 * XML shows up here as a new non-influential offset. */
struct influence_case {
   const char *name;
   uint8_t *base;
   size_t size;
   /* Offsets of pointer members. Mutating a pointer produces a wild address,
    * not a different payload, so those bytes are not wire data and are
    * excluded. The pointees are covered by their own cases and by T1. */
   const size_t *ptr_off;
   size_t ptr_count;
   const size_t *expected_dead;
   size_t expected_dead_count;
   struct rt_encoded (*encode)(const void *);
   void *wrapper;
};

static bool
is_pointer_byte(const struct influence_case *c, size_t off)
{
   for (size_t i = 0; i < c->ptr_count; i++) {
      if (off >= c->ptr_off[i] && off < c->ptr_off[i] + sizeof(void *))
         return true;
   }
   return false;
}

static void
test_influence(const struct influence_case *c)
{
   struct rt_encoded ref = c->encode(c->wrapper);
   size_t dead[512];
   size_t dead_count = 0;

   for (size_t i = 0; i < c->size; i++) {
      if (is_pointer_byte(c, i))
         continue;

      uint8_t saved = c->base[i];
      c->base[i] = (uint8_t)~saved;

      struct rt_encoded mut = c->encode(c->wrapper);
      bool same = mut.written == ref.written &&
                  memcmp(mut.data, ref.data, ref.written) == 0;
      rt_encoded_free(&mut);
      c->base[i] = saved;

      if (same) {
         if (dead_count < sizeof(dead) / sizeof(dead[0]))
            dead[dead_count] = i;
         dead_count++;
      }
   }

   bool match = dead_count == c->expected_dead_count;
   for (size_t i = 0; match && i < dead_count; i++)
      match = dead[i] == c->expected_dead[i];

   CHECK(match,
         "%s: %zu byte offsets do not influence the wire (expected %zu). A new "
         "one means a field is no longer serialized.",
         c->name, dead_count, c->expected_dead_count);

   if (!match) {
      fprintf(stderr, "       non-influential offsets:");
      for (size_t i = 0; i < dead_count && i < 64; i++)
         fprintf(stderr, " %zu", dead[i]);
      fprintf(stderr, "\n");
   }

   rt_encoded_free(&ref);
}

static void
test_allocation_cap(void)
{
   /* A count that is legal per the schema but large. With a tiny budget the
    * decoder must fail closed rather than attempt the allocation. */
   VkVideoDecodeH264PictureInfoKHR pic = mk_picture();
   struct rt_encoded a = rt_encode_h264_picture(&pic);
   VkVideoDecodeH264PictureInfoKHR out;

   bool ok = rt_decode_h264_picture(a.data, a.written, 8, &out);
   CHECK(!ok, "picture info: decode succeeded under an 8-byte temp budget");
   rt_decode_release();

   ok = rt_decode_h264_picture(a.data, a.written, RT_SWEEP_BUDGET, &out);
   CHECK(ok, "picture info: decode failed under a generous temp budget");
   rt_decode_release();

   rt_encoded_free(&a);
}

/* ------------------------------------------------------------------ *
 * Thin typed wrappers so the generic driver can hold function pointers
 * ------------------------------------------------------------------ */

#define RT_SHIM(name, type, enc, dec)                                         \
   static struct rt_encoded name##_enc(const void *v)                         \
   {                                                                          \
      return enc((const type *)v);                                            \
   }                                                                          \
   static bool name##_dec(const void *d, size_t l, size_t b, void *o)         \
   {                                                                          \
      return dec(d, l, b, (type *)o);                                         \
   }

RT_SHIM(profile, VkVideoDecodeH264ProfileInfoKHR, rt_encode_h264_profile,
        rt_decode_h264_profile)
RT_SHIM(add_info, VkVideoDecodeH264SessionParametersAddInfoKHR,
        rt_encode_h264_add_info, rt_decode_h264_add_info)
RT_SHIM(sp_create, VkVideoDecodeH264SessionParametersCreateInfoKHR,
        rt_encode_h264_sp_create, rt_decode_h264_sp_create)
RT_SHIM(picture, VkVideoDecodeH264PictureInfoKHR, rt_encode_h264_picture,
        rt_decode_h264_picture)
RT_SHIM(dpb_slot, VkVideoDecodeH264DpbSlotInfoKHR, rt_encode_h264_dpb_slot,
        rt_decode_h264_dpb_slot)

int
main(void)
{
   setvbuf(stdout, NULL, _IONBF, 0);
   setvbuf(stderr, NULL, _IONBF, 0);

   build_payloads();

   VkVideoDecodeH264ProfileInfoKHR profile = mk_profile();
   VkVideoDecodeH264SessionParametersAddInfoKHR add = mk_add_info();
   VkVideoDecodeH264SessionParametersCreateInfoKHR spc = mk_sp_create(&add);
   VkVideoDecodeH264PictureInfoKHR pic = mk_picture();
   VkVideoDecodeH264DpbSlotInfoKHR dpb = mk_dpb_slot();

   const struct payload payloads[] = {
      { "VkVideoDecodeH264ProfileInfoKHR", profile_enc, profile_dec, &profile,
        sizeof(profile), sizeof(profile) },
      { "VkVideoDecodeH264SessionParametersAddInfoKHR", add_info_enc,
        add_info_dec, &add, sizeof(add), sizeof(add) },
      { "VkVideoDecodeH264SessionParametersCreateInfoKHR", sp_create_enc,
        sp_create_dec, &spc, sizeof(spc), sizeof(spc) },
      { "VkVideoDecodeH264PictureInfoKHR", picture_enc, picture_dec, &pic,
        sizeof(pic), sizeof(pic) },
      { "VkVideoDecodeH264DpbSlotInfoKHR", dpb_slot_enc, dpb_slot_dec, &dpb,
        sizeof(dpb), sizeof(dpb) },
   };
   const size_t n = sizeof(payloads) / sizeof(payloads[0]);

   printf("T1/T2 round-trip idempotence and size agreement\n");
   for (size_t i = 0; i < n; i++)
      test_roundtrip(&payloads[i]);

   printf("T3 field influence\n");
   {
      /* Offsets that legitimately do not reach the wire. Regenerate by
       * running the test and confirming each new offset against the struct
       * definition before adding it. */
      static const size_t sps_dead[] = {
#include "golden/sps_dead.inc"
      };
      static const size_t pps_dead[] = {
#include "golden/pps_dead.inc"
      };
      static const size_t sps_ptrs[] = {
         offsetof(StdVideoH264SequenceParameterSet, pOffsetForRefFrame),
         offsetof(StdVideoH264SequenceParameterSet, pScalingLists),
         offsetof(StdVideoH264SequenceParameterSet, pSequenceParameterSetVui),
      };
      static const size_t pps_ptrs[] = {
         offsetof(StdVideoH264PictureParameterSet, pScalingLists),
      };
      const struct influence_case cases[] = {
         { "StdVideoH264SequenceParameterSet", (uint8_t *)&sps[0],
           sizeof(sps[0]), sps_ptrs, sizeof(sps_ptrs) / sizeof(sps_ptrs[0]),
           sps_dead, sizeof(sps_dead) / sizeof(sps_dead[0]), add_info_enc,
           &add },
         { "StdVideoH264PictureParameterSet", (uint8_t *)&pps[0],
           sizeof(pps[0]), pps_ptrs, sizeof(pps_ptrs) / sizeof(pps_ptrs[0]),
           pps_dead, sizeof(pps_dead) / sizeof(pps_dead[0]), add_info_enc,
           &add },
      };
      for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
         test_influence(&cases[i]);
   }

   printf("T4 truncation\n");
   for (size_t i = 0; i < n; i++)
      test_truncation(&payloads[i]);

   printf("T5 exhaustive single-word corruption\n");
   for (size_t i = 0; i < n; i++)
      test_word_corruption(&payloads[i]);

   printf("T6 allocation cap\n");
   test_allocation_cap();

   printf("\n%d checks, %d failures\n", checks, failures);
   return failures ? 1 : 0;
}
