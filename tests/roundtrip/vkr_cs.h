/*
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: MIT
 *
 * Executable renderer-side command stream for the round-trip harness.
 *
 * Transcribed from virglrenderer's src/venus/vkr_cs.h so the decoder under
 * test behaves exactly as it does in the renderer:
 *
 *   vkr_cs_decoder_peek_internal():
 *       bounds-check size against end - cur; on failure log, set fatal,
 *       zero the destination and return false.
 *   vkr_cs_decoder_read(): peek, then advance by size.
 *   vkr_cs_decoder_alloc_temp_array(): __builtin_mul_overflow, fail closed.
 *
 * Three intentional differences, all of which make the harness stricter:
 *
 *   1. Temp allocations are individual malloc()s tracked in a list rather
 *      than bump-allocated from a large pool. Under AddressSanitizer each
 *      allocation gets its own redzone, so an off-by-one write into a
 *      decoded array is a hard error instead of silently landing in the
 *      next object's storage.
 *   2. Every temp allocation is poisoned before use, so a decoder that
 *      forgets to write an element leaves 0xa5 rather than a zero that
 *      might coincidentally match the encoder's input.
 *   3. `fatal` is a plain member checked by the harness after every decode.
 *      The real renderer aborts the context; here we want to assert on it.
 */

#ifndef VKR_CS_H
#define VKR_CS_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

struct vkr_cs_encoder;

typedef uint64_t vkr_object_id;

struct vkr_object {
   union {
      uint64_t u64;
   } handle;
};

#define VKR_CS_TEMP_POISON 0xa5
#define VKR_CS_MAX_TEMPS 4096

struct vkr_cs_decoder {
   const uint8_t *cur;
   const uint8_t *end;
   bool fatal;

   void *temps[VKR_CS_MAX_TEMPS];
   size_t temp_count;

   /* Set by the harness to model a renderer-side allocation cap. Zero means
    * unlimited. Used to prove the decoder fails closed rather than
    * attempting a huge allocation on a guest-controlled count. */
   size_t temp_budget;
   size_t temp_used;
};

static inline void
vkr_cs_decoder_init(struct vkr_cs_decoder *dec, const void *data, size_t size)
{
   memset(dec, 0, sizeof(*dec));
   dec->cur = data;
   dec->end = (const uint8_t *)data + size;
}

static inline void
vkr_cs_decoder_set_fatal(const struct vkr_cs_decoder *dec)
{
   /* Upstream logs and flags the context; the flag is mutated through a
    * const pointer there too. */
   ((struct vkr_cs_decoder *)dec)->fatal = true;
}

static inline bool
vkr_cs_decoder_get_fatal(const struct vkr_cs_decoder *dec)
{
   return dec->fatal;
}

static inline bool
vkr_cs_decoder_peek_internal(const struct vkr_cs_decoder *dec,
                             size_t size,
                             void *val,
                             size_t val_size)
{
   assert(val_size <= size);

   if (size > (size_t)(dec->end - dec->cur)) {
      vkr_cs_decoder_set_fatal(dec);
      memset(val, 0, val_size);
      return false;
   }

   if (dec->cur != val)
      memcpy(val, dec->cur, val_size);
   return true;
}

static inline void
vkr_cs_decoder_read(struct vkr_cs_decoder *dec,
                    size_t size,
                    void *val,
                    size_t val_size)
{
   if (vkr_cs_decoder_peek_internal(dec, size, val, val_size))
      dec->cur += size;
}

static inline void
vkr_cs_decoder_peek(const struct vkr_cs_decoder *dec,
                    size_t size,
                    void *val,
                    size_t val_size)
{
   vkr_cs_decoder_peek_internal(dec, size, val, val_size);
}

static inline struct vkr_object *
vkr_cs_decoder_lookup_object(const struct vkr_cs_decoder *dec,
                             vkr_object_id id,
                             VkObjectType type)
{
   (void)dec;
   (void)id;
   (void)type;
   return NULL;
}

static inline void *
vkr_cs_decoder_alloc_temp(struct vkr_cs_decoder *dec, size_t size)
{
   if (!size)
      return NULL;

   if (dec->temp_count >= VKR_CS_MAX_TEMPS) {
      vkr_cs_decoder_set_fatal(dec);
      return NULL;
   }

   if (dec->temp_budget && dec->temp_used + size > dec->temp_budget) {
      vkr_cs_decoder_set_fatal(dec);
      return NULL;
   }

   void *p = malloc(size);
   if (!p) {
      vkr_cs_decoder_set_fatal(dec);
      return NULL;
   }

   memset(p, VKR_CS_TEMP_POISON, size);
   dec->temps[dec->temp_count++] = p;
   dec->temp_used += size;
   return p;
}

static inline void *
vkr_cs_decoder_alloc_temp_array(struct vkr_cs_decoder *dec,
                                size_t size,
                                size_t count)
{
   size_t alloc_size;
   if (__builtin_mul_overflow(size, count, &alloc_size)) {
      vkr_cs_decoder_set_fatal(dec);
      return NULL;
   }
   return vkr_cs_decoder_alloc_temp(dec, alloc_size);
}

static inline void
vkr_cs_decoder_reset_temp_pool(struct vkr_cs_decoder *dec)
{
   for (size_t i = 0; i < dec->temp_count; i++)
      free(dec->temps[i]);
   dec->temp_count = 0;
   dec->temp_used = 0;
}

static inline void *
vkr_cs_decoder_get_blob_storage(struct vkr_cs_decoder *dec, size_t size)
{
   return vkr_cs_decoder_alloc_temp(dec, size);
}

static inline void *
vkr_cs_encoder_get_blob_storage(struct vkr_cs_encoder *enc,
                                size_t offset,
                                size_t size)
{
   (void)enc;
   (void)offset;
   (void)size;
   return NULL;
}

static inline bool
vkr_cs_encoder_acquire(struct vkr_cs_encoder *enc)
{
   (void)enc;
   return true;
}

static inline void
vkr_cs_encoder_release(struct vkr_cs_encoder *enc)
{
   (void)enc;
}

static inline void
vkr_cs_encoder_write(struct vkr_cs_encoder *enc,
                     size_t size,
                     const void *val,
                     size_t val_size)
{
   (void)enc;
   (void)size;
   (void)val;
   (void)val_size;
}

static inline bool
vkr_cs_handle_indirect_id(VkObjectType type)
{
   (void)type;
   return true;
}

static inline vkr_object_id
vkr_cs_handle_load_id(const void **handle, VkObjectType type)
{
   (void)type;
   return (vkr_object_id)(uintptr_t)*handle;
}

static inline void
vkr_cs_handle_store_id(void **handle, vkr_object_id id, VkObjectType type)
{
   (void)type;
   *handle = (void *)(uintptr_t)id;
}

#endif /* VKR_CS_H */
