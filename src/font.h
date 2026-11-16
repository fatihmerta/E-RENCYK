#ifndef MZWAVE_FONT_H
#define MZWAVE_FONT_H

#include <stdint.h>
#include <stdbool.h>

void font_init(void);

/* Measure UTF-8 string width in pixels at given scale (base advance = 6 * scale) */
float font_measure_utf8(const char *utf8, float scale);

/* Draw UTF-8 string (supports ASCII + Turkish ĞğÜüŞşİıÖöÇç + Latin accents) */
void font_draw_utf8(float x, float y, const char *utf8, float scale, uint32_t color);

/* Draw UTF-8 string truncated with "..." to fit max_w pixels */
void font_draw_utf8_ellipsis(float x, float y, float max_w,
                             const char *utf8, float scale, uint32_t color);

/* Authentic 7-segment digital LCD readout (supports '0'-'9', ':', '-', ' ')
 * Draws both ghost (unlit) segments and lit segments!
 */
float font_draw_7seg(float x, float y, float digit_w, float digit_h, float thick,
                     const char *text, uint32_t lit_col, uint32_t ghost_col);

#endif /* MZWAVE_FONT_H */
