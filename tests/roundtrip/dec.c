/*
 * SPDX-License-Identifier: MIT
 *
 * Host-side (renderer) half of the round-trip harness.
 *
 * Includes the generated renderer protocol and exercises the real
 * vn_decode_*_temp path — the one that runs on guest-controlled bytes.
 */

#include <stdlib.h>
#include <string.h>

#include "vn_protocol_renderer_structs.h"
#include "vn_protocol_renderer_transport.h"
#include "vn_protocol_renderer_command_buffer.h"

#include "harness.h"

static struct vkr_cs_decoder rt_dec;

void
rt_decode_release(void)
{
   vkr_cs_decoder_reset_temp_pool(&rt_dec);
}

/* The decoders are the interesting half: every array length, every enum and
 * every flags word in the buffer is attacker-controlled in the real system.
 *
 * The struct is poisoned before decode so a field the decoder never writes
 * shows up as 0xa5 in the comparison rather than inheriting a zero that might
 * coincidentally match the encoder's input.
 */
#define RT_DECODER(fn, type, prefix)                                          \
   bool fn(const void *data, size_t len, size_t budget, type *out)            \
   {                                                                          \
      vkr_cs_decoder_reset_temp_pool(&rt_dec);                                \
      vkr_cs_decoder_init(&rt_dec, data, len);                                \
      rt_dec.temp_budget = budget;                                            \
      memset(out, 0xa5, sizeof(*out));                                        \
      vn_decode_##prefix##_temp((struct vn_cs_decoder *)&rt_dec, out);        \
      return !vkr_cs_decoder_get_fatal(&rt_dec);                              \
   }

RT_DECODER(rt_decode_h264_profile,
           VkVideoDecodeH264ProfileInfoKHR,
           VkVideoDecodeH264ProfileInfoKHR)
RT_DECODER(rt_decode_h264_add_info,
           VkVideoDecodeH264SessionParametersAddInfoKHR,
           VkVideoDecodeH264SessionParametersAddInfoKHR)
RT_DECODER(rt_decode_h264_sp_create,
           VkVideoDecodeH264SessionParametersCreateInfoKHR,
           VkVideoDecodeH264SessionParametersCreateInfoKHR)
RT_DECODER(rt_decode_h264_picture,
           VkVideoDecodeH264PictureInfoKHR,
           VkVideoDecodeH264PictureInfoKHR)
RT_DECODER(rt_decode_h264_dpb_slot,
           VkVideoDecodeH264DpbSlotInfoKHR,
           VkVideoDecodeH264DpbSlotInfoKHR)
