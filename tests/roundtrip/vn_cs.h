/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 *
 * Executable driver-side command stream for the round-trip harness.
 *
 * The stubs in tests/vn_cs.h make the upstream test a *compile* test: every
 * primitive is an empty body, so encode and decode both succeed without
 * touching a single byte. That is enough to prove the generated code builds,
 * and nothing else.
 *
 * This file implements the same API for real. The bodies are deliberately
 * transcribed from Mesa's src/virtio/vulkan/vn_cs.h rather than reinvented:
 *
 *   vn_cs_encoder_write():   memcpy(cur, val, val_size); cur += size;
 *   vn_cs_encoder_reserve(): fail when size > end - cur
 *
 * Two intentional differences, both of which make the harness *stricter* than
 * the real encoder:
 *
 *   1. Padding bytes (the size - val_size slack that keeps every field
 *      4-byte aligned) are filled with a poison pattern instead of being left
 *      as whatever the ring buffer happened to hold. If a decoder ever reads a
 *      pad byte it sees 0xa5, not a plausible zero, so a round-trip comparison
 *      fails loudly instead of passing by luck.
 *   2. The buffer is exact-sized and heap-allocated by the caller, so
 *      AddressSanitizer treats a one-byte overrun as a hard error. The real
 *      encoder writes into a large shared ring where an overrun is silent.
 */

#ifndef VN_CS_H
#define VN_CS_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

typedef uint64_t vn_object_id;

/* Poison written into inter-field padding. */
#define VN_CS_PAD_POISON 0xa5

struct vn_cs_encoder {
   uint8_t *start;
   uint8_t *cur;
   uint8_t *end;
   bool fatal;
};

struct vn_cs_decoder {
   const uint8_t *cur;
   const uint8_t *end;
   bool fatal;
};

static inline void
vn_cs_encoder_init(struct vn_cs_encoder *enc, void *data, size_t size)
{
   enc->start = data;
   enc->cur = data;
   enc->end = (uint8_t *)data + size;
   enc->fatal = false;
}

static inline bool
vn_cs_renderer_protocol_has_api_version(uint32_t api_version)
{
   (void)api_version;
   return true;
}

static inline bool
vn_cs_renderer_protocol_has_extension(uint32_t ext_number)
{
   (void)ext_number;
   return true;
}

static inline size_t
vn_cs_encoder_get_len(const struct vn_cs_encoder *enc)
{
   return (size_t)(enc->cur - enc->start);
}

static inline bool
vn_cs_encoder_reserve(struct vn_cs_encoder *enc, size_t size)
{
   if (size > (size_t)(enc->end - enc->cur)) {
      enc->fatal = true;
      return false;
   }
   return true;
}

static inline void
vn_cs_encoder_write(struct vn_cs_encoder *enc,
                    size_t size,
                    const void *val,
                    size_t val_size)
{
   assert(val_size <= size);

   if (size > (size_t)(enc->end - enc->cur)) {
      enc->fatal = true;
      return;
   }

   memcpy(enc->cur, val, val_size);
   if (size > val_size)
      memset(enc->cur + val_size, VN_CS_PAD_POISON, size - val_size);
   enc->cur += size;
}

static inline void
vn_cs_decoder_init(struct vn_cs_decoder *dec, const void *data, size_t size)
{
   dec->cur = data;
   dec->end = (const uint8_t *)data + size;
   dec->fatal = false;
}

static inline void
vn_cs_decoder_set_fatal(struct vn_cs_decoder *dec)
{
   dec->fatal = true;
}

static inline bool
vn_cs_decoder_peek_internal(struct vn_cs_decoder *dec,
                            size_t size,
                            void *val,
                            size_t val_size)
{
   assert(val_size <= size);

   if (size > (size_t)(dec->end - dec->cur)) {
      vn_cs_decoder_set_fatal(dec);
      memset(val, 0, val_size);
      return false;
   }

   memcpy(val, dec->cur, val_size);
   return true;
}

static inline void
vn_cs_decoder_read(struct vn_cs_decoder *dec,
                   size_t size,
                   void *val,
                   size_t val_size)
{
   if (vn_cs_decoder_peek_internal(dec, size, val, val_size))
      dec->cur += size;
}

static inline void
vn_cs_decoder_peek(struct vn_cs_decoder *dec,
                   size_t size,
                   void *val,
                   size_t val_size)
{
   vn_cs_decoder_peek_internal(dec, size, val, val_size);
}

static inline vn_object_id
vn_cs_handle_load_id(const void **handle, VkObjectType type)
{
   (void)type;
   return (vn_object_id)(uintptr_t)*handle;
}

static inline void
vn_cs_handle_store_id(void **handle, vn_object_id id, VkObjectType type)
{
   (void)type;
   *handle = (void *)(uintptr_t)id;
}

#endif /* VN_CS_H */
