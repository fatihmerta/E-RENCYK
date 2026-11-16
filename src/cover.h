#ifndef MZWAVE_COVER_H
#define MZWAVE_COVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void cover_init(void);

/* Decode embedded APIC image (JPEG or PNG) from memory into active 128x128 GU texture */
bool cover_load_from_memory(const uint8_t *data, size_t size);

/* Search MP3's directory for matching <track>.jpg/png or cover/folder/album/kapak.jpg/png */
bool cover_load_from_folder(const char *mp3_path);

/* Generate a procedural 2000s iridescent MiniDisc jewel graphic seeded by title */
void cover_generate_minidisc(const char *seed_text);

/* Returns 16-byte aligned 128x128 RGBA8888 texture pointer */
const uint32_t *cover_get_texture(void);

#endif /* MZWAVE_COVER_H */
