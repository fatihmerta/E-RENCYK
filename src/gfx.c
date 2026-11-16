#include "gfx.h"
#include "app.h"

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 1 MB 16-byte aligned GU display list */
static unsigned int __attribute__((aligned(16))) s_gu_list[262144];

typedef struct {
    uint32_t color;
    float x, y, z;
} ColorVertex;

typedef struct {
    float u, v;
    uint32_t color;
    float x, y, z;
} TexVertex;

void gfx_init(void)
{
    sceGuInit();

    sceGuStart(GU_DIRECT, s_gu_list);
    /* Double-buffered 32-bit RGBA8888 in 2MB VRAM:
     * Buffer 0: 0x000000 (512 * 272 * 4 = 0x88000 bytes)
     * Buffer 1: 0x088000 (512 * 272 * 4 = 0x88000 bytes)
     */
    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_W);
    sceGuDispBuffer(SCREEN_W, SCREEN_H, (void *)0x88000, BUF_W);
    sceGuDepthBuffer((void *)0x110000, BUF_W);

    sceGuOffset(2048 - (SCREEN_W / 2), 2048 - (SCREEN_H / 2));
    sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);
    sceGuDepthRange(65535, 0);

    sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_LIGHTING);

    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void gfx_term(void)
{
    sceGuTerm();
}

void gfx_begin_frame(uint32_t clear_color)
{
    sceGuStart(GU_DIRECT, s_gu_list);
    sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
    sceGuClearColor(clear_color);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuDisable(GU_TEXTURE_2D);
}

void gfx_end_frame(void)
{
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

void gfx_scissor(int x, int y, int w, int h)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    sceGuScissor(x, y, w, h);
}

void gfx_scissor_reset(void)
{
    sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
}

void gfx_grad_rect_4(float x, float y, float w, float h,
                     uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br)
{
    if (w <= 0.0f || h <= 0.0f) return;
    sceGuDisable(GU_TEXTURE_2D);

    ColorVertex *v = (ColorVertex *)sceGuGetMemory(4 * sizeof(ColorVertex));
    if (!v) return;

    v[0].color = tl; v[0].x = x;     v[0].y = y;     v[0].z = 0.0f;
    v[1].color = tr; v[1].x = x + w; v[1].y = y;     v[1].z = 0.0f;
    v[2].color = bl; v[2].x = x;     v[2].y = y + h; v[2].z = 0.0f;
    v[3].color = br; v[3].x = x + w; v[3].y = y + h; v[3].z = 0.0f;

    sceGuDrawArray(GU_TRIANGLE_STRIP,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   4, NULL, v);
}

void gfx_fill_rect(float x, float y, float w, float h, uint32_t color)
{
    gfx_grad_rect_4(x, y, w, h, color, color, color, color);
}

void gfx_grad_rect_v(float x, float y, float w, float h, uint32_t top_col, uint32_t bot_col)
{
    gfx_grad_rect_4(x, y, w, h, top_col, top_col, bot_col, bot_col);
}

void gfx_grad_rect_h(float x, float y, float w, float h, uint32_t left_col, uint32_t right_col)
{
    gfx_grad_rect_4(x, y, w, h, left_col, right_col, left_col, right_col);
}

void gfx_rect_outline(float x, float y, float w, float h, float t, uint32_t color)
{
    if (w <= 0.0f || h <= 0.0f) return;
    gfx_fill_rect(x, y, w, t, color);
    gfx_fill_rect(x, y + h - t, w, t, color);
    gfx_fill_rect(x, y + t, t, h - 2.0f * t, color);
    gfx_fill_rect(x + w - t, y + t, t, h - 2.0f * t, color);
}

void gfx_bevel_rect(float x, float y, float w, float h,
                    uint32_t fill_top, uint32_t fill_bot,
                    uint32_t hi_col, uint32_t lo_col)
{
    gfx_grad_rect_v(x, y, w, h, fill_top, fill_bot);
    /* Top & left specular edge */
    gfx_fill_rect(x, y, w, 1.0f, hi_col);
    gfx_fill_rect(x, y + 1.0f, 1.0f, h - 2.0f, hi_col);
    /* Bottom & right shadow edge */
    gfx_fill_rect(x, y + h - 1.0f, w, 1.0f, lo_col);
    gfx_fill_rect(x + w - 1.0f, y + 1.0f, 1.0f, h - 2.0f, lo_col);
}

void gfx_line_grad(float x0, float y0, float x1, float y1, float thickness,
                   uint32_t col0, uint32_t col1)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    float half = thickness * 0.5f;
    float nx = (-dy / len) * half;
    float ny = ( dx / len) * half;

    sceGuDisable(GU_TEXTURE_2D);
    ColorVertex *v = (ColorVertex *)sceGuGetMemory(4 * sizeof(ColorVertex));
    if (!v) return;

    v[0].color = col0; v[0].x = x0 + nx; v[0].y = y0 + ny; v[0].z = 0.0f;
    v[1].color = col0; v[1].x = x0 - nx; v[1].y = y0 - ny; v[1].z = 0.0f;
    v[2].color = col1; v[2].x = x1 + nx; v[2].y = y1 + ny; v[2].z = 0.0f;
    v[3].color = col1; v[3].x = x1 - nx; v[3].y = y1 - ny; v[3].z = 0.0f;

    sceGuDrawArray(GU_TRIANGLE_STRIP,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   4, NULL, v);
}

void gfx_line(float x0, float y0, float x1, float y1, float thickness, uint32_t color)
{
    gfx_line_grad(x0, y0, x1, y1, thickness, color, color);
}

void gfx_circle_fill(float cx, float cy, float r, int segments,
                     uint32_t center_col, uint32_t edge_col)
{
    if (segments < 6 || r <= 0.0f) return;
    sceGuDisable(GU_TEXTURE_2D);

    int count = segments + 2;
    ColorVertex *v = (ColorVertex *)sceGuGetMemory(count * sizeof(ColorVertex));
    if (!v) return;

    v[0].color = center_col;
    v[0].x = cx;
    v[0].y = cy;
    v[0].z = 0.0f;

    float step = (float)(2.0 * M_PI) / (float)segments;
    for (int i = 0; i <= segments; i++) {
        float a = (float)i * step;
        v[i + 1].color = edge_col;
        v[i + 1].x = cx + cosf(a) * r;
        v[i + 1].y = cy + sinf(a) * r;
        v[i + 1].z = 0.0f;
    }

    sceGuDrawArray(GU_TRIANGLE_FAN,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   count, NULL, v);
}

void gfx_sector_grad(float cx, float cy, float r_inner, float r_outer,
                     float deg_start, float deg_end, int segments,
                     uint32_t col_start, uint32_t col_end)
{
    if (segments < 1 || r_outer <= 0.0f) return;
    sceGuDisable(GU_TEXTURE_2D);

    int count = (segments + 1) * 2;
    ColorVertex *v = (ColorVertex *)sceGuGetMemory(count * sizeof(ColorVertex));
    if (!v) return;

    float rad0 = deg_start * ((float)M_PI / 180.0f);
    float rad1 = deg_end   * ((float)M_PI / 180.0f);

    uint32_t r0 = col_start & 0xFF, g0 = (col_start >> 8) & 0xFF, b0 = (col_start >> 16) & 0xFF, a0 = (col_start >> 24) & 0xFF;
    uint32_t r1 = col_end   & 0xFF, g1 = (col_end   >> 8) & 0xFF, b1 = (col_end   >> 16) & 0xFF, a1 = (col_end   >> 24) & 0xFF;

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float a = rad0 + (rad1 - rad0) * t;
        float cs = cosf(a);
        float sn = sinf(a);

        uint32_t cr = (uint32_t)(r0 + (float)((int)r1 - (int)r0) * t);
        uint32_t cg = (uint32_t)(g0 + (float)((int)g1 - (int)g0) * t);
        uint32_t cb = (uint32_t)(b0 + (float)((int)b1 - (int)b0) * t);
        uint32_t ca = (uint32_t)(a0 + (float)((int)a1 - (int)a0) * t);
        uint32_t col = RGBA8(cr, cg, cb, ca);

        v[i * 2 + 0].color = col;
        v[i * 2 + 0].x = cx + cs * r_inner;
        v[i * 2 + 0].y = cy + sn * r_inner;
        v[i * 2 + 0].z = 0.0f;

        v[i * 2 + 1].color = col;
        v[i * 2 + 1].x = cx + cs * r_outer;
        v[i * 2 + 1].y = cy + sn * r_outer;
        v[i * 2 + 1].z = 0.0f;
    }

    sceGuDrawArray(GU_TRIANGLE_STRIP,
                   GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   count, NULL, v);
}

void gfx_arc_band(float cx, float cy, float r_inner, float r_outer,
                  float deg_start, float deg_end, int segments, uint32_t color)
{
    gfx_sector_grad(cx, cy, r_inner, r_outer, deg_start, deg_end, segments, color, color);
}

void gfx_ring(float cx, float cy, float r_inner, float r_outer, int segments, uint32_t color)
{
    gfx_arc_band(cx, cy, r_inner, r_outer, 0.0f, 360.0f, segments, color);
}

void gfx_draw_tex(const uint32_t *tex_data, int tex_w, int tex_h,
                  float u0, float v0, float u1, float v1,
                  float x, float y, float w, float h,
                  uint32_t tint, bool linear_filter)
{
    if (!tex_data || w <= 0.0f || h <= 0.0f) return;

    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, tex_w, tex_h, tex_w, tex_data);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(linear_filter ? GU_LINEAR : GU_NEAREST,
                   linear_filter ? GU_LINEAR : GU_NEAREST);

    TexVertex *v = (TexVertex *)sceGuGetMemory(2 * sizeof(TexVertex));
    if (!v) {
        sceGuDisable(GU_TEXTURE_2D);
        return;
    }

    v[0].u = u0; v[0].v = v0; v[0].color = tint; v[0].x = x;     v[0].y = y;     v[0].z = 0.0f;
    v[1].u = u1; v[1].v = v1; v[1].color = tint; v[1].x = x + w; v[1].y = y + h; v[1].z = 0.0f;

    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   2, NULL, v);

    sceGuDisable(GU_TEXTURE_2D);
}
