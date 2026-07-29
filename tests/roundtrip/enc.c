/*
 * SPDX-License-Identifier: MIT
 *
 * Guest-side (driver) half of the round-trip harness.
 *
 * Includes the generated driver protocol and exercises the real
 * vn_sizeof_* / vn_encode_* pairs against an exact-sized buffer.
 */

#include <stdlib.h>
#include <string.h>

#include "vn_protocol_driver_structs.h"
#include "vn_protocol_driver_transport.h"
#include "vn_protocol_driver_command_buffer.h"

#include "harness.h"

void
rt_encoded_free(struct rt_encoded *e)
{
   free(e->data);
   e->data = NULL;
}

/* One body per payload. The size is taken from the generated sizeof helper
 * and the buffer allocated to exactly that, so:
 *
 *   - an encoder that writes more than sizeof predicted overruns a heap
 *     allocation and ASan aborts;
 *   - an encoder that writes less shows up as predicted != written.
 *
 * Both are silent in the upstream compile-only harness.
 */
#define RT_ENCODER(fn, type, prefix)                                          \
   struct rt_encoded fn(const type *val)                                      \
   {                                                                          \
      struct rt_encoded e = { 0 };                                            \
      e.predicted = vn_sizeof_##prefix(val);                                  \
      e.data = malloc(e.predicted ? e.predicted : 1);                         \
      struct vn_cs_encoder enc;                                               \
      vn_cs_encoder_init(&enc, e.data, e.predicted);                          \
      vn_encode_##prefix(&enc, val);                                          \
      e.written = vn_cs_encoder_get_len(&enc);                                \
      e.encoder_fatal = enc.fatal;                                            \
      return e;                                                               \
   }

RT_ENCODER(rt_encode_h264_profile,
           VkVideoDecodeH264ProfileInfoKHR,
           VkVideoDecodeH264ProfileInfoKHR)
RT_ENCODER(rt_encode_h264_add_info,
           VkVideoDecodeH264SessionParametersAddInfoKHR,
           VkVideoDecodeH264SessionParametersAddInfoKHR)
RT_ENCODER(rt_encode_h264_sp_create,
           VkVideoDecodeH264SessionParametersCreateInfoKHR,
           VkVideoDecodeH264SessionParametersCreateInfoKHR)
RT_ENCODER(rt_encode_h264_picture,
           VkVideoDecodeH264PictureInfoKHR,
           VkVideoDecodeH264PictureInfoKHR)
RT_ENCODER(rt_encode_h264_dpb_slot,
           VkVideoDecodeH264DpbSlotInfoKHR,
           VkVideoDecodeH264DpbSlotInfoKHR)
