/*
 * SPDX-License-Identifier: MIT
 *
 * Interface between the three translation units of the round-trip harness.
 *
 * The generated driver and renderer headers cannot be included in the same
 * translation unit: both define vn_encode_* and vn_decode_* for the same
 * types with different bodies. So the harness is split:
 *
 *   enc.c   includes vn_protocol_driver_*.h   (guest side, encode)
 *   dec.c   includes vn_protocol_renderer_*.h (host side, decode)
 *   main.c  includes neither; it only builds payloads and compares results
 *
 * Vulkan and vk_video struct layouts are identical in all three because they
 * come from the same vulkan.h and vk_video headers.
 */

#ifndef VN_ROUNDTRIP_HARNESS_H
#define VN_ROUNDTRIP_HARNESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

/* Result of an encode: how many bytes the sizeof helper predicted, and how
 * many the encoder actually wrote. A mismatch is the classic Venus bug that
 * turns into a renderer heap overflow, so the harness checks it on every
 * payload rather than only on the ones a human thought to test. */
struct rt_encoded {
   uint8_t *data;
   size_t predicted; /* vn_sizeof_*()  */
   size_t written;   /* vn_cs_encoder_get_len() */
   bool encoder_fatal;
};

void rt_encoded_free(struct rt_encoded *e);

/* Encoders. Each allocates an exact-sized buffer of vn_sizeof_X(val) bytes so
 * ASan flags a single byte of overrun. */
struct rt_encoded rt_encode_h264_profile(const VkVideoDecodeH264ProfileInfoKHR *val);
struct rt_encoded rt_encode_h264_add_info(const VkVideoDecodeH264SessionParametersAddInfoKHR *val);
struct rt_encoded rt_encode_h264_sp_create(const VkVideoDecodeH264SessionParametersCreateInfoKHR *val);
struct rt_encoded rt_encode_h264_picture(const VkVideoDecodeH264PictureInfoKHR *val);
struct rt_encoded rt_encode_h264_dpb_slot(const VkVideoDecodeH264DpbSlotInfoKHR *val);

/* Decoders. `budget` caps total temp allocation (0 = unlimited); it exists to
 * prove the decoder fails closed instead of honouring a huge guest count.
 * Returns false when the decoder marked the stream fatal.
 *
 * On success the caller owns nothing: all nested arrays live in the decoder's
 * temp pool, which rt_decode_release() frees. */
bool rt_decode_h264_profile(const void *data, size_t len, size_t budget,
                            VkVideoDecodeH264ProfileInfoKHR *out);
bool rt_decode_h264_add_info(const void *data, size_t len, size_t budget,
                             VkVideoDecodeH264SessionParametersAddInfoKHR *out);
bool rt_decode_h264_sp_create(const void *data, size_t len, size_t budget,
                              VkVideoDecodeH264SessionParametersCreateInfoKHR *out);
bool rt_decode_h264_picture(const void *data, size_t len, size_t budget,
                            VkVideoDecodeH264PictureInfoKHR *out);
bool rt_decode_h264_dpb_slot(const void *data, size_t len, size_t budget,
                             VkVideoDecodeH264DpbSlotInfoKHR *out);

/* Frees the temp pool used by the most recent successful decode. */
void rt_decode_release(void);

/* Bytes handed out by the decoder temp pool during the last decode. Used to
 * prove an array cap fires BEFORE the allocation rather than after it: with
 * no budget set, an uncapped over-large count would really allocate. */
size_t rt_last_temp_used(void);

struct rt_encoded rt_encode_video_profile_list(const VkVideoProfileListInfoKHR *val);
bool rt_decode_video_profile_list(const void *data, size_t len, size_t budget,
                                  VkVideoProfileListInfoKHR *out);

#endif /* VN_ROUNDTRIP_HARNESS_H */
