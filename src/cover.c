#include "cover.h"
#include "app.h"

#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>

#include <jpeglib.h>
#include <png.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static uint32_t __attribute__((aligned(16))) s_cover_tex[COVER_TEX_SIZE * COVER_TEX_SIZE];

/* Accumulator buffers in static BSS (~260 KB total) so we never fragment heap */
static uint32_t s_acc_r[COVER_TEX_SIZE * COVER_TEX_SIZE];
static uint32_t s_acc_g[COVER_TEX_SIZE * COVER_TEX_SIZE];
static uint32_t s_acc_b[COVER_TEX_SIZE * COVER_TEX_SIZE];
static uint32_t s_acc_cnt[COVER_TEX_SIZE * COVER_TEX_SIZE];

static void acc_reset(void)
{
    memset(s_acc_r,   0, sizeof(s_acc_r));
    memset(s_acc_g,   0, sizeof(s_acc_g));
    memset(s_acc_b,   0, sizeof(s_acc_b));
    memset(s_acc_cnt, 0, sizeof(s_acc_cnt));
}

/* Feed one RGB/RGBA source scanline (y_src in 0..src_h-1) into center-cropped 128x128 accumulator */
static void acc_feed_scanline_rgb(const uint8_t *row, int src_w, int src_h,
                                  int channels, int y_src)
{
    int side = (src_w < src_h) ? src_w : src_h;
    if (side <= 0) return;

    int crop_x0 = (src_w - side) / 2;
    int crop_y0 = (src_h - side) / 2;

    if (y_src < crop_y0 || y_src >= crop_y0 + side) return;
    int cy = y_src - crop_y0;

    /* Support both downscaling (side >= 128) and upscaling (side < 128) */
    int dy0 = (cy * COVER_TEX_SIZE) / side;
    int dy1 = ((cy + 1) * COVER_TEX_SIZE - 1) / side;
    if (dy0 < 0) dy0 = 0;
    if (dy1 >= COVER_TEX_SIZE) dy1 = COVER_TEX_SIZE - 1;
    if (dy1 < dy0) dy1 = dy0;

    for (int cx = 0; cx < side; cx++) {
        const uint8_t *px = row + (crop_x0 + cx) * channels;
        uint32_t r = px[0];
        uint32_t g = (channels >= 3) ? px[1] : px[0];
        uint32_t b = (channels >= 3) ? px[2] : px[0];
        if (channels == 4) {
            /* Alpha composite over dark slate background (#181C22) */
            uint32_t a = px[3];
            r = (r * a + 24u * (255u - a)) / 255u;
            g = (g * a + 28u * (255u - a)) / 255u;
            b = (b * a + 34u * (255u - a)) / 255u;
        }

        int dx0 = (cx * COVER_TEX_SIZE) / side;
        int dx1 = ((cx + 1) * COVER_TEX_SIZE - 1) / side;
        if (dx0 < 0) dx0 = 0;
        if (dx1 >= COVER_TEX_SIZE) dx1 = COVER_TEX_SIZE - 1;
        if (dx1 < dx0) dx1 = dx0;

        for (int dy = dy0; dy <= dy1; dy++) {
            int row_off = dy * COVER_TEX_SIZE;
            for (int dx = dx0; dx <= dx1; dx++) {
                int idx = row_off + dx;
                s_acc_r[idx] += r;
                s_acc_g[idx] += g;
                s_acc_b[idx] += b;
                s_acc_cnt[idx] += 1;
            }
        }
    }
}

static void acc_finalize_to_tex(void)
{
    for (int i = 0; i < COVER_TEX_SIZE * COVER_TEX_SIZE; i++) {
        uint32_t c = s_acc_cnt[i];
        if (c > 0) {
            uint32_t r = s_acc_r[i] / c;
            uint32_t g = s_acc_g[i] / c;
            uint32_t b = s_acc_b[i] / c;
            s_cover_tex[i] = RGB8(r, g, b);
        } else {
            s_cover_tex[i] = RGB8(24, 28, 34);
        }
    }
    sceKernelDcacheWritebackAll();
}

/* ---------------- Safe JPEG Decoder ---------------- */
struct SafeJpegErr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

static void safe_jpeg_error_exit(j_common_ptr cinfo)
{
    struct SafeJpegErr *myerr = (struct SafeJpegErr *)cinfo->err;
    longjmp(myerr->setjmp_buffer, 1);
}

static bool decode_jpeg_common(struct jpeg_decompress_struct *cinfo)
{
    if (jpeg_read_header(cinfo, TRUE) != JPEG_HEADER_OK) {
        return false;
    }

    cinfo->out_color_space = JCS_RGB;
    cinfo->dct_method = JDCT_IFAST;

    /* Use DCT hardware-friendly scaling for high-res 1000..4000px covers */
    unsigned int min_dim = (cinfo->image_width < cinfo->image_height)
                               ? cinfo->image_width
                               : cinfo->image_height;
    cinfo->scale_num = 1;
    if (min_dim >= 1024) {
        cinfo->scale_denom = 8;
    } else if (min_dim >= 512) {
        cinfo->scale_denom = 4;
    } else if (min_dim >= 256) {
        cinfo->scale_denom = 2;
    } else {
        cinfo->scale_denom = 1;
    }

    jpeg_start_decompress(cinfo);

    int w = (int)cinfo->output_width;
    int h = (int)cinfo->output_height;
    int ch = (int)cinfo->output_components;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096 || (ch != 1 && ch != 3)) {
        jpeg_abort_decompress(cinfo);
        return false;
    }

    JSAMPROW row_buf = (JSAMPROW)malloc((size_t)w * (size_t)ch);
    if (!row_buf) {
        jpeg_abort_decompress(cinfo);
        return false;
    }

    acc_reset();
    while (cinfo->output_scanline < cinfo->output_height) {
        int y = (int)cinfo->output_scanline;
        if (jpeg_read_scanlines(cinfo, &row_buf, 1) == 1) {
            acc_feed_scanline_rgb(row_buf, w, h, ch, y);
        } else {
            break;
        }
    }

    free(row_buf);
    jpeg_finish_decompress(cinfo);
    acc_finalize_to_tex();
    return true;
}

static bool decode_jpeg_mem(const uint8_t *data, size_t size)
{
    struct jpeg_decompress_struct cinfo;
    struct SafeJpegErr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = safe_jpeg_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, (unsigned long)size);
    bool ok = decode_jpeg_common(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return ok;
}

static bool decode_jpeg_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    struct jpeg_decompress_struct cinfo;
    struct SafeJpegErr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = safe_jpeg_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    bool ok = decode_jpeg_common(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    return ok;
}

/* ---------------- Safe PNG Decoder ---------------- */
typedef struct {
    const uint8_t *data;
    size_t size;
    size_t offset;
} PngMemReader;

static void png_mem_read_cb(png_structp png_ptr, png_bytep out_bytes, png_size_t count)
{
    PngMemReader *r = (PngMemReader *)png_get_io_ptr(png_ptr);
    if (!r || r->offset + count > r->size) {
        png_error(png_ptr, "PNG EOF");
        return;
    }
    memcpy(out_bytes, r->data + r->offset, count);
    r->offset += count;
}

static bool decode_png_stream(png_structp png_ptr, png_infop info_ptr)
{
    png_read_info(png_ptr, info_ptr);

    png_uint_32 w = 0, h = 0;
    int bit_depth = 0, color_type = 0, interlace = 0;
    png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, &interlace, NULL, NULL);

    if (w == 0 || h == 0 || w > 2048 || h > 2048) {
        return false;
    }

    if (bit_depth == 16) png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    int passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    acc_reset();

    if (passes == 1) {
        /* Streaming scanline path: uses only 1 row of RAM! */
        uint8_t *row = (uint8_t *)malloc(rowbytes);
        if (!row) return false;
        for (png_uint_32 y = 0; y < h; y++) {
            png_read_row(png_ptr, row, NULL);
            acc_feed_scanline_rgb(row, (int)w, (int)h, 4, (int)y);
        }
        free(row);
    } else {
        /* Interlaced PNG fallback (capped to reasonable size on PSP) */
        if (w * h > 1024u * 1024u) return false;
        uint8_t *img = (uint8_t *)malloc(rowbytes * h);
        if (!img) return false;
        for (int p = 0; p < passes; p++) {
            for (png_uint_32 y = 0; y < h; y++) {
                png_read_row(png_ptr, img + y * rowbytes, NULL);
            }
        }
        for (png_uint_32 y = 0; y < h; y++) {
            acc_feed_scanline_rgb(img + y * rowbytes, (int)w, (int)h, 4, (int)y);
        }
        free(img);
    }

    acc_finalize_to_tex();
    return true;
}

static bool decode_png_mem(const uint8_t *data, size_t size)
{
    if (size < 8 || png_sig_cmp((png_const_bytep)data, 0, 8) != 0) return false;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return false;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return false;
    }

    PngMemReader reader = { data, size, 0 };
    png_set_read_fn(png_ptr, &reader, png_mem_read_cb);
    bool ok = decode_png_stream(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return ok;
}

static bool decode_png_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    uint8_t sig[8];
    if (fread(sig, 1, 8, fp) != 8 || png_sig_cmp(sig, 0, 8) != 0) {
        fclose(fp);
        return false;
    }
    rewind(fp);

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return false;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return false;
    }

    png_init_io(png_ptr, fp);
    bool ok = decode_png_stream(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return ok;
}

static bool load_image_file_auto(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    uint8_t hdr[8] = {0};
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    if (n < 4) return false;

    if (hdr[0] == 0xFF && hdr[1] == 0xD8 && hdr[2] == 0xFF) {
        return decode_jpeg_file(path);
    }
    if (n >= 8 && hdr[0] == 0x89 && hdr[1] == 'P' && hdr[2] == 'N' && hdr[3] == 'G') {
        return decode_png_file(path);
    }
    return false;
}

/* ---------------- Public Cover API ---------------- */

bool cover_load_from_memory(const uint8_t *data, size_t size)
{
    if (!data || size < 16) return false;

    /* Note: some quirky ID3v2 taggers leave 1-4 leading null bytes before magic */
    size_t off = 0;
    while (off + 8 < size && data[off] == 0x00) {
        off++;
    }
    const uint8_t *p = data + off;
    size_t rem = size - off;

    if (rem >= 4 && p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF) {
        return decode_jpeg_mem(p, rem);
    }
    if (rem >= 8 && p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G') {
        return decode_png_mem(p, rem);
    }
    return false;
}

bool cover_load_from_folder(const char *mp3_path)
{
    if (!mp3_path || !*mp3_path) return false;

    /* Extract directory and basename without extension */
    const char *slash = strrchr(mp3_path, '/');
    char dir[MAX_PATH_LEN];
    char stem[MAX_TITLE_LEN];

    if (slash) {
        size_t dlen = (size_t)(slash - mp3_path);
        if (dlen >= sizeof(dir)) dlen = sizeof(dir) - 1;
        memcpy(dir, mp3_path, dlen);
        dir[dlen] = '\0';
        snprintf(stem, sizeof(stem), "%s", slash + 1);
    } else {
        snprintf(dir, sizeof(dir), ".");
        snprintf(stem, sizeof(stem), "%s", mp3_path);
    }

    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';

    char candidate[MAX_PATH_LEN + MAX_TITLE_LEN + 32];

    /* 1. Exact track basename match (<song>.jpg / <song>.png) */
    const char *exts[] = { ".jpg", ".JPG", ".jpeg", ".png", ".PNG" };
    for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); i++) {
        snprintf(candidate, sizeof(candidate), "%s/%s%s", dir, stem, exts[i]);
        if (load_image_file_auto(candidate)) return true;
    }

    /* 2. Standard album folder cover filenames (including Turkish kapak) */
    static const char *const names[] = {
        "cover.jpg", "Cover.jpg", "COVER.JPG",
        "cover.png", "Cover.png", "COVER.PNG",
        "folder.jpg", "Folder.jpg", "FOLDER.JPG",
        "folder.png", "album.jpg", "Album.jpg",
        "front.jpg", "kapak.jpg", "Kapak.jpg", "kapak.png"
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, names[i]);
        if (load_image_file_auto(candidate)) return true;
    }

    return false;
}

/* Procedural Sony Color-Collection 2001 Iridescent MiniDisc Artwork */
void cover_generate_minidisc(const char *seed_text)
{
    uint32_t hash = 2166136261u;
    if (seed_text) {
        for (const unsigned char *s = (const unsigned char *)seed_text; *s; s++) {
            hash ^= *s;
            hash *= 16777619u;
        }
    }

    /* 6 iconic Y2K translucent MiniDisc shell palettes */
    static const uint8_t palettes[6][3] = {
        {  38, 128, 210 }, /* Cobalt Ice Blue */
        {  46, 175, 155 }, /* Cyber Mint Aqua */
        { 152,  78, 205 }, /* Atomic Violet */
        { 218, 124,  42 }, /* Tokyo Sunset Amber */
        {  72, 168,  84 }, /* Acid Lime Y2K */
        { 198,  58, 104 }  /* Ruby Magenta */
    };
    const uint8_t *pal = palettes[hash % 6];

    float cx = 64.0f, cy = 66.0f;

    for (int y = 0; y < COVER_TEX_SIZE; y++) {
        for (int x = 0; x < COVER_TEX_SIZE; x++) {
            /* Base translucent shell gradient */
            float gx = (float)x / 127.0f;
            float gy = (float)y / 127.0f;
            float sheen = 0.65f + 0.35f * (1.0f - (gx * 0.6f + gy * 0.4f));

            int r = (int)(pal[0] * sheen * 0.55f + 16.0f);
            int g = (int)(pal[1] * sheen * 0.55f + 20.0f);
            int b = (int)(pal[2] * sheen * 0.55f + 26.0f);

            /* Molded translucent cartridge ribs */
            if (((x + y) % 16 == 0) && (x < 24 || x > 104)) {
                r += 14; g += 16; b += 18;
            }

            /* Optical disc circular window inside MiniDisc shell */
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < 48.0f && dist > 13.0f) {
                float ang = atan2f(dy, dx);
                /* Iridescent CD/MD rainbow diffraction lobes */
                float lobe = sinf(ang * 2.0f + 0.8f);
                float ring = sinf(dist * 0.85f) * 0.08f;

                int dr = (int)(168.0f + 55.0f * sinf(ang * 2.0f + 0.2f) + ring * 120.0f);
                int dg = (int)(182.0f + 52.0f * sinf(ang * 2.0f + 1.9f) + ring * 120.0f);
                int db = (int)(198.0f + 50.0f * sinf(ang * 2.0f + 3.6f) + ring * 120.0f);

                if (lobe > 0.25f) {
                    dr += 28; dg += 32; db += 35;
                }
                /* Blend optical platter under smoked polycarbonate window */
                r = (r * 30 + dr * 70) / 100;
                g = (g * 30 + dg * 70) / 100;
                b = (b * 30 + db * 70) / 100;
            } else if (dist <= 13.0f) {
                /* Brushed steel clamping hub */
                int hub = (dist < 5.5f) ? 38 : (int)(175.0f - dist * 2.5f);
                r = hub; g = hub + 4; b = hub + 10;
            }

            /* Top brushed-metal sliding shutter (right-top section of MiniDisc) */
            if (y >= 4 && y <= 26 && x >= 52 && x <= 118) {
                int metal = 186 + ((y % 2) ? 8 : -4) - (x - 52) / 4;
                r = metal; g = metal + 4; b = metal + 9;
                if (y == 4 || y == 26 || x == 52 || x == 118) {
                    r = 95; g = 102; b = 112;
                }
            }

            /* Top-left molded insertion arrow */
            if (y >= 10 && y <= 20 && x >= 14 && x <= 24) {
                int ay = y - 10;
                int half = ay / 2;
                if (x >= 19 - half && x <= 19 + half) {
                    r = 225; g = 232; b = 240;
                }
            }

            /* Bottom label strip */
            if (y >= 108 && y <= 121 && x >= 14 && x <= 114) {
                r = 228; g = 232; b = 236;
                if (y >= 108 && y <= 111) {
                    r = pal[0]; g = pal[1]; b = pal[2];
                }
            }

            /* Outer cartridge bevel border */
            if (x < 3 || y < 3) {
                r += 35; g += 38; b += 42;
            } else if (x > 124 || y > 124) {
                r -= 35; g -= 35; b -= 35;
            }

            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;

            s_cover_tex[y * COVER_TEX_SIZE + x] = RGB8(r, g, b);
        }
    }

    sceKernelDcacheWritebackAll();
}

void cover_init(void)
{
    cover_generate_minidisc("MZ-WAVE 2001");
}

const uint32_t *cover_get_texture(void)
{
    return s_cover_tex;
}
