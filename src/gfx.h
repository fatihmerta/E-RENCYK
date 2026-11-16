#ifndef MZWAVE_GFX_H
#define MZWAVE_GFX_H

#include <stdint.h>
#include <stdbool.h>

void gfx_init(void);
void gfx_term(void);

void gfx_begin_frame(uint32_t clear_color);
void gfx_end_frame(void);

/* Hardware Scissor Clipping */
void gfx_scissor(int x, int y, int w, int h);
void gfx_scissor_reset(void);

/* 2D Solid & Gouraud-Shaded Primitives */
void gfx_fill_rect(float x, float y, float w, float h, uint32_t color);
void gfx_grad_rect_v(float x, float y, float w, float h, uint32_t top_col, uint32_t bot_col);
void gfx_grad_rect_h(float x, float y, float w, float h, uint32_t left_col, uint32_t right_col);
void gfx_grad_rect_4(float x, float y, float w, float h,
                     uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br);

void gfx_rect_outline(float x, float y, float w, float h, float thickness, uint32_t color);
void gfx_bevel_rect(float x, float y, float w, float h,
                    uint32_t fill_top, uint32_t fill_bot,
                    uint32_t hi_col, uint32_t lo_col);

/* Anti-staircase quad-extruded lines, circles, arcs, and sectors */
void gfx_line(float x0, float y0, float x1, float y1, float thickness, uint32_t color);
void gfx_line_grad(float x0, float y0, float x1, float y1, float thickness,
                   uint32_t col0, uint32_t col1);

void gfx_circle_fill(float cx, float cy, float r, int segments,
                     uint32_t center_col, uint32_t edge_col);
void gfx_ring(float cx, float cy, float r_inner, float r_outer, int segments, uint32_t color);
void gfx_arc_band(float cx, float cy, float r_inner, float r_outer,
                  float deg_start, float deg_end, int segments, uint32_t color);
void gfx_sector_grad(float cx, float cy, float r_inner, float r_outer,
                     float deg_start, float deg_end, int segments,
                     uint32_t col_start, uint32_t col_end);

/* RGBA8888 Texture Drawing (tex_w & tex_h must be powers of two, 16-byte aligned data) */
void gfx_draw_tex(const uint32_t *tex_data, int tex_w, int tex_h,
                  float u0, float v0, float u1, float v1,
                  float x, float y, float w, float h,
                  uint32_t tint, bool linear_filter);

#endif /* MZWAVE_GFX_H */
