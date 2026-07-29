/*
 * Copyright 2026 the Venus Vulkan Video lab contributors
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Explicit bit packing for the StdVideo H.264 *Flags bitfield structs.
 *
 * WHY THIS IS NOT GENERATED
 *
 * The generator's scalar helpers take pointers:
 *
 *     static inline void vn_encode_uint32_t(struct vn_cs_encoder *enc,
 *                                           const uint32_t *val);
 *
 * and taking the address of a bitfield is illegal C, so the flags cannot be
 * described as ordinary members in XML. Describing a *Flags struct as one
 * opaque uint32_t and copying it would compile, but C leaves bitfield
 * allocation order and padding implementation-defined -- guest and host are not
 * guaranteed to agree, and a disagreement would corrupt decode parameters
 * silently rather than failing.
 *
 * So each flags struct crosses the wire as a single uint32_t whose layout is
 * fixed HERE by explicit shifts over named fields. Each side then lets its own
 * compiler lay out the bitfield however it likes; only these shift constants
 * are shared.
 *
 * THE SHIFT ASSIGNMENTS ARE WIRE CONTRACT.
 *
 * They are append-only, exactly like the VkCommandTypeEXT ids. Renumbering or
 * reordering a bit silently changes the meaning of a decode parameter for every
 * peer built against a different revision. Add new bits at the next free
 * position; never reuse or reorder.
 */

#ifndef VN_PROTOCOL_VIDEO_H264_FLAGS_H
#define VN_PROTOCOL_VIDEO_H264_FLAGS_H

#include <stdint.h>

#include "vk_video/vulkan_video_codec_h264std.h"
#include "vk_video/vulkan_video_codec_h264std_decode.h"

/* StdVideoDecodeH264PictureInfoFlags — bits 0..5, append-only. */
#define VN_H264_PIC_FLAG_field_pic_flag            0u
#define VN_H264_PIC_FLAG_is_intra                  1u
#define VN_H264_PIC_FLAG_IdrPicFlag                2u
#define VN_H264_PIC_FLAG_bottom_field_flag         3u
#define VN_H264_PIC_FLAG_is_reference              4u
#define VN_H264_PIC_FLAG_complementary_field_pair  5u
#define VN_H264_PIC_FLAG_VALID_MASK                0x3fu

/* StdVideoDecodeH264ReferenceInfoFlags — bits 0..3, append-only. */
#define VN_H264_REF_FLAG_top_field_flag               0u
#define VN_H264_REF_FLAG_bottom_field_flag            1u
#define VN_H264_REF_FLAG_used_for_long_term_reference 2u
#define VN_H264_REF_FLAG_is_non_existing              3u
#define VN_H264_REF_FLAG_VALID_MASK                   0x0fu

static inline uint32_t
vn_pack_StdVideoDecodeH264PictureInfoFlags(
   const StdVideoDecodeH264PictureInfoFlags *f)
{
   return ((uint32_t)(f->field_pic_flag           & 1u) << VN_H264_PIC_FLAG_field_pic_flag) |
          ((uint32_t)(f->is_intra                 & 1u) << VN_H264_PIC_FLAG_is_intra) |
          ((uint32_t)(f->IdrPicFlag               & 1u) << VN_H264_PIC_FLAG_IdrPicFlag) |
          ((uint32_t)(f->bottom_field_flag        & 1u) << VN_H264_PIC_FLAG_bottom_field_flag) |
          ((uint32_t)(f->is_reference             & 1u) << VN_H264_PIC_FLAG_is_reference) |
          ((uint32_t)(f->complementary_field_pair & 1u) << VN_H264_PIC_FLAG_complementary_field_pair);
}

/*
 * Returns false if the guest set any bit outside the defined range.
 *
 * Unknown bits are REJECTED rather than masked away: a peer setting them is
 * either a newer revision we cannot correctly interpret, or a malformed
 * stream. Silently clearing them would let a decode proceed with parameters
 * that do not mean what the sender intended.
 */
static inline bool
vn_unpack_StdVideoDecodeH264PictureInfoFlags(
   uint32_t bits, StdVideoDecodeH264PictureInfoFlags *f)
{
   if (bits & ~VN_H264_PIC_FLAG_VALID_MASK)
      return false;

   f->field_pic_flag           = (bits >> VN_H264_PIC_FLAG_field_pic_flag) & 1u;
   f->is_intra                 = (bits >> VN_H264_PIC_FLAG_is_intra) & 1u;
   f->IdrPicFlag               = (bits >> VN_H264_PIC_FLAG_IdrPicFlag) & 1u;
   f->bottom_field_flag        = (bits >> VN_H264_PIC_FLAG_bottom_field_flag) & 1u;
   f->is_reference             = (bits >> VN_H264_PIC_FLAG_is_reference) & 1u;
   f->complementary_field_pair = (bits >> VN_H264_PIC_FLAG_complementary_field_pair) & 1u;
   return true;
}

static inline uint32_t
vn_pack_StdVideoDecodeH264ReferenceInfoFlags(
   const StdVideoDecodeH264ReferenceInfoFlags *f)
{
   return ((uint32_t)(f->top_field_flag               & 1u) << VN_H264_REF_FLAG_top_field_flag) |
          ((uint32_t)(f->bottom_field_flag            & 1u) << VN_H264_REF_FLAG_bottom_field_flag) |
          ((uint32_t)(f->used_for_long_term_reference & 1u) << VN_H264_REF_FLAG_used_for_long_term_reference) |
          ((uint32_t)(f->is_non_existing              & 1u) << VN_H264_REF_FLAG_is_non_existing);
}

static inline bool
vn_unpack_StdVideoDecodeH264ReferenceInfoFlags(
   uint32_t bits, StdVideoDecodeH264ReferenceInfoFlags *f)
{
   if (bits & ~VN_H264_REF_FLAG_VALID_MASK)
      return false;

   f->top_field_flag               = (bits >> VN_H264_REF_FLAG_top_field_flag) & 1u;
   f->bottom_field_flag            = (bits >> VN_H264_REF_FLAG_bottom_field_flag) & 1u;
   f->used_for_long_term_reference = (bits >> VN_H264_REF_FLAG_used_for_long_term_reference) & 1u;
   f->is_non_existing              = (bits >> VN_H264_REF_FLAG_is_non_existing) & 1u;
   return true;
}

#endif /* VN_PROTOCOL_VIDEO_H264_FLAGS_H */
