/*
 * Accessor for the read-only `ncrom` flash partition: a container with the
 * zlib-compressed (and pre-ProcessBinary'd) NC1020 ROM plus the default NOR
 * image (see tools/gen_rom_blob.py). The partition is memory-mapped, so the
 * compressed bytes stay in flash — only the decompressed ROM uses PSRAM.
 */
#ifndef ROM_STORE_H
#define ROM_STORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mmap'd compressed ROM (zlib). *z_len = compressed length, *raw_len =
 * decompressed length (24MB). Returns NULL if the partition is absent/invalid
 * (→ caller falls back to SD). Args may be NULL if only presence is wanted. */
const uint8_t *rom_store_rom_z(size_t *z_len, size_t *raw_len);

/* mmap'd default NOR image (raw nc1020.fls, 1MB) used to seed the writable
 * save on first boot. Returns NULL if unavailable. */
const uint8_t *rom_store_nor(size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* ROM_STORE_H */
