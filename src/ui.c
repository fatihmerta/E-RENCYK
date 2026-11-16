#include "ui.h"
#include "gfx.h"
#include "font.h"
#include "cover.h"
#include "audio.h"
#include "playlist.h"

#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------------- 5 Backlit LCD Glass Palettes ---------------- */
static const LcdPalette s_lcd_palettes[5] = {
    /* 0: Ice Cyan EL (Sony MZ-N10) */
    {
        "Ice Cyan EL",
        RGB8( 10,  44,  56), RGB8(  6,  26,  36),
        RGB8(168, 250, 255), RGB8( 98, 208, 226), RGBA8( 28,  88, 106, 115),
        RGB8(146, 242, 252), RGB8(  6,  30,  40), RGBA8( 80, 225, 255,  45)
    },
    /* 1: Phosphor Green (Sony MZ-R900) */
    {
        "Phosphor Green",
        RGB8( 20,  48,  18), RGB8( 10,  28,  10),
        RGB8(192, 255, 128), RGB8(130, 214,  82), RGBA8( 46,  92,  38, 115),
        RGB8(176, 246, 112), RGB8( 12,  32,  10), RGBA8(140, 255,  90,  45)
    },
    /* 2: Warm Amber Hi-Fi (Marantz / Sony ES) */
    {
        "Warm Amber",
        RGB8( 52,  30,   8), RGB8( 30,  16,   4),
        RGB8(255, 214, 116), RGB8(232, 158,  58), RGBA8( 98,  56,  16, 115),
        RGB8(255, 198,  88), RGB8( 36,  18,   4), RGBA8(255, 175,  50,  45)
    },
    /* 3: Ruby Orange GT (Early-2000s Alpine Headunit) */
    {
        "Ruby Orange",
        RGB8( 52,  16,  14), RGB8( 30,   8,   8),
        RGB8(255, 166, 132), RGB8(236, 112,  82), RGBA8(102,  38,  30, 115),
        RGB8(255, 138, 102), RGB8( 36,  10,   8), RGBA8(255,  96,  64,  45)
    },
    /* 4: Pure White EL (Y2K Mint/Frost Edition) */
    {
        "Pure Frost EL",
        RGB8( 24,  38,  46), RGB8( 14,  24,  30),
        RGB8(238, 252, 255), RGB8(170, 206, 218), RGBA8( 62,  86,  98, 115),
        RGB8(226, 246, 252), RGB8( 16,  26,  34), RGBA8(200, 245, 255,  45)
    }
};

/* ---------------- 6 Complete Full-Device Hardware Themes ---------------- */
static const DeviceTheme s_device_themes[DEVICE_THEME_COUNT] = {
    /* 0: Walkman Silver 2001 */
    {
        "Walkman Silver 2001", "Brushed Titanium & Ice-Cyan EL",
        ICON_MINIDISC, 0,
        RGB8(226, 232, 240), RGB8(198, 206, 216), RGB8(166, 175, 186), RGB8(140, 149, 160),
        RGB8(202, 210, 220), RGB8(164, 173, 184),
        RGB8( 24,  30,  40), RGB8( 72,  82,  94),
        RGB8(246, 250, 255), RGB8( 88,  96, 108),
        RGB8(188, 196, 206), RGB8(148, 157, 168),
        RGB8( 36,  41,  48), RGB8( 22,  26,  32),
        RGB8(156, 168, 182), RGB8( 72, 248, 138),
        RGB8( 56, 186, 224), RGB8(142, 244, 192),
        RGB8(255, 240, 198), RGB8(252, 226, 170), RGB8(232, 198, 138), RGB8(218, 180, 118),
        RGBA8(255, 250, 220, 95), RGB8( 54,  40,  28), RGB8(204,  34,  28),
        RGB8( 34,  36,  42), RGB8(216,  36,  28),
        RGB8( 92, 246, 214), RGB8(255, 204,  74), RGB8(255,  78,  72),
        RGB8(196, 204, 214), RGB8(158, 167, 178),
        RGB8( 24,  30,  38), RGB8( 64,  72,  84), RGB8( 40,  46,  56)
    },
    /* 1: Champagne Gold ES */
    {
        "Champagne Gold ES", "Sony ES Luxury Gold & Warm Amber",
        ICON_HIFI_SPEAKER, 2,
        RGB8(236, 224, 196), RGB8(216, 200, 168), RGB8(188, 170, 136), RGB8(162, 144, 112),
        RGB8(222, 208, 178), RGB8(186, 168, 136),
        RGB8( 42,  32,  18), RGB8( 92,  76,  52),
        RGB8(252, 244, 224), RGB8(108,  92,  66),
        RGB8(208, 192, 160), RGB8(170, 152, 120),
        RGB8( 42,  34,  26), RGB8( 26,  20,  14),
        RGB8(196, 178, 148), RGB8(255, 192,  64),
        RGB8(226, 152,  48), RGB8(255, 218, 108),
        RGB8(255, 244, 214), RGB8(252, 234, 190), RGB8(236, 210, 158), RGB8(222, 192, 136),
        RGBA8(255, 236, 180, 100), RGB8( 58,  42,  24), RGB8(198,  38,  26),
        RGB8( 46,  34,  22), RGB8(208,  42,  28),
        RGB8(255, 198,  78), RGB8(255, 148,  52), RGB8(255,  72,  58),
        RGB8(214, 198, 166), RGB8(178, 160, 128),
        RGB8( 38,  28,  16), RGB8( 84,  68,  46), RGB8( 54,  42,  28)
    },
    /* 2: Midnight Cyber Black */
    {
        "Midnight Cyber Black", "Stealth Magnesium & Matrix Green",
        ICON_HEADPHONES, 1,
        RGB8( 58,  62,  70), RGB8( 46,  50,  58), RGB8( 34,  38,  44), RGB8( 24,  28,  34),
        RGB8( 52,  56,  64), RGB8( 36,  40,  48),
        RGB8(226, 236, 244), RGB8(142, 156, 168),
        RGB8( 92, 100, 112), RGB8( 14,  16,  20),
        RGB8( 50,  54,  62), RGB8( 34,  38,  46),
        RGB8( 20,  24,  28), RGB8( 12,  14,  18),
        RGB8(136, 178, 142), RGB8(124, 255,  96),
        RGB8( 64, 196,  92), RGB8(172, 255, 120),
        RGB8( 36,  44,  38), RGB8( 28,  36,  30), RGB8( 20,  26,  22), RGB8( 16,  22,  18),
        RGBA8(120, 255, 110,  42), RGB8(158, 232, 138), RGB8(255,  84,  76),
        RGB8(196, 220, 196), RGB8(136, 255, 104),
        RGB8(116, 250,  96), RGB8(224, 255,  88), RGB8(255,  92,  72),
        RGB8( 46,  50,  58), RGB8( 30,  34,  40),
        RGB8(224, 234, 242), RGB8(146, 160, 174), RGB8( 22,  26,  32)
    },
    /* 3: Cobalt Blue Y2K */
    {
        "Cobalt Blue Y2K", "Sony MZ-R700 Metallic Blue Edition",
        ICON_LASER_DISC, 0,
        RGB8( 86, 128, 196), RGB8( 68, 108, 176), RGB8( 48,  84, 148), RGB8( 36,  66, 124),
        RGB8( 82, 122, 190), RGB8( 56,  94, 158),
        RGB8(240, 248, 255), RGB8(184, 210, 242),
        RGB8(138, 178, 238), RGB8( 24,  44,  86),
        RGB8( 76, 116, 182), RGB8( 52,  86, 146),
        RGB8( 22,  34,  58), RGB8( 14,  22,  40),
        RGB8(152, 196, 238), RGB8( 96, 242, 255),
        RGB8( 58, 192, 255), RGB8(158, 248, 255),
        RGB8(232, 242, 255), RGB8(214, 230, 250), RGB8(186, 206, 234), RGB8(168, 190, 222),
        RGBA8(200, 235, 255, 95), RGB8( 32,  52,  82), RGB8(220,  42,  54),
        RGB8( 28,  42,  68), RGB8(232,  46,  62),
        RGB8( 84, 226, 255), RGB8(146, 184, 255), RGB8(255,  96, 132),
        RGB8( 68, 106, 170), RGB8( 46,  78, 136),
        RGB8(240, 248, 255), RGB8(188, 212, 242), RGB8( 28,  48,  86)
    },
    /* 4: Crimson Carbon GT */
    {
        "Crimson Carbon GT", "Gunmetal Alloy & Alpine Ruby-Orange",
        ICON_EQ_SLIDERS, 3,
        RGB8( 88,  90,  98), RGB8( 72,  74,  82), RGB8( 54,  56,  64), RGB8( 40,  42,  48),
        RGB8( 80,  82,  90), RGB8( 58,  60,  68),
        RGB8(246, 242, 240), RGB8(186, 176, 176),
        RGB8(126, 128, 138), RGB8( 22,  22,  26),
        RGB8( 76,  78,  86), RGB8( 52,  54,  62),
        RGB8( 28,  22,  24), RGB8( 18,  14,  16),
        RGB8(198, 152, 148), RGB8(255,  92,  64),
        RGB8(224,  68,  52), RGB8(255, 168,  76),
        RGB8( 38,  32,  34), RGB8( 30,  24,  26), RGB8( 22,  18,  20), RGB8( 18,  14,  16),
        RGBA8(255, 110,  70,  48), RGB8(228, 198, 186), RGB8(255,  68,  58),
        RGB8(220, 210, 208), RGB8(255,  76,  52),
        RGB8(255, 126,  72), RGB8(255, 188,  76), RGB8(255,  64,  64),
        RGB8( 68,  70,  78), RGB8( 46,  48,  56),
        RGB8(244, 240, 240), RGB8(184, 178, 180), RGB8( 28,  26,  30)
    },
    /* 5: Frost Mint Edition */
    {
        "Frost Mint Edition", "Y2K Metallic Mint & Pure Frost EL",
        ICON_MUSIC_NOTE, 4,
        RGB8(204, 232, 224), RGB8(178, 214, 204), RGB8(148, 190, 180), RGB8(124, 168, 158),
        RGB8(190, 222, 212), RGB8(154, 194, 184),
        RGB8( 22,  44,  40), RGB8( 62,  96,  90),
        RGB8(238, 252, 248), RGB8( 76, 114, 106),
        RGB8(174, 210, 200), RGB8(138, 178, 168),
        RGB8( 26,  42,  40), RGB8( 16,  28,  26),
        RGB8(158, 206, 196), RGB8( 96, 255, 204),
        RGB8( 56, 204, 176), RGB8(174, 255, 228),
        RGB8(242, 252, 248), RGB8(226, 244, 238), RGB8(202, 226, 218), RGB8(186, 212, 204),
        RGBA8(220, 255, 245, 95), RGB8( 34,  58,  52), RGB8(208,  46,  64),
        RGB8( 28,  48,  44), RGB8(218,  52,  72),
        RGB8(108, 248, 212), RGB8(188, 252, 236), RGB8(255, 112, 132),
        RGB8(180, 214, 204), RGB8(144, 182, 172),
        RGB8( 20,  40,  36), RGB8( 58,  90,  84), RGB8( 32,  56,  52)
    }
};

static const DeviceTheme *get_theme(const AppState *app)
{
    int id = (int)app->theme_id;
    if (id < 0 || id >= DEVICE_THEME_COUNT) id = 0;
    return &s_device_themes[id];
}

static const LcdPalette *get_lcd_palette(const AppState *app)
{
    if (app->lcd_override > LCD_OV_AUTO && app->lcd_override < LCD_OV_COUNT) {
        return &s_lcd_palettes[(int)app->lcd_override - 1];
    }
    const DeviceTheme *th = get_theme(app);
    return &s_lcd_palettes[th->default_lcd_idx % 5];
}

const char *ui_device_theme_name(DeviceThemeId id)
{
    if (id < 0 || id >= DEVICE_THEME_COUNT) return s_device_themes[0].name;
    return s_device_themes[id].name;
}

const char *ui_lcd_override_name(LcdOverrideId id)
{
    switch (id) {
        case LCD_OV_AUTO:           return "Theme Default";
        case LCD_OV_ICE_CYAN:       return "Ice Cyan EL";
        case LCD_OV_PHOSPHOR_GREEN: return "Phosphor Green";
        case LCD_OV_WARM_AMBER:     return "Warm Amber";
        case LCD_OV_RUBY_ORANGE:    return "Ruby Orange";
        case LCD_OV_PURE_WHITE:     return "Pure Frost EL";
        default:                    return "Theme Default";
    }
}

const char *ui_vis_name(VisMode mode)
{
    switch (mode) {
        case VIS_ANALOG_VU:    return "Analog VU Needles";
        case VIS_VFD_SPECTRUM: return "16-Band Spectrum";
        case VIS_OSCILLOSCOPE: return "Stereo Oscilloscope";
        default:               return "Analog VU";
    }
}

const char *ui_play_mode_name(PlayMode mode)
{
    switch (mode) {
        case PLAY_MODE_REPEAT_ALL: return "Repeat All";
        case PLAY_MODE_REPEAT_ONE: return "Repeat One";
        case PLAY_MODE_SHUFFLE:    return "Shuffle";
        case PLAY_MODE_NORMAL:     return "Play Once";
        default:                   return "Repeat All";
    }
}

void ui_init(AppState *app)
{
    app->needle_l = 0.0f;
    app->needle_r = 0.0f;
    app->needle_vel_l = 0.0f;
    app->needle_vel_r = 0.0f;
    app->disc_angle_deg = 0.0f;
    app->disc_rpm_factor = 0.0f;
    app->marquee_px = 0.0f;
    app->marquee_pause = 75;
}

void ui_show_osd(AppState *app, const char *msg)
{
    if (!app || !msg) return;
    snprintf(app->osd_msg, sizeof(app->osd_msg), "%s", msg);
    app->osd_timer = 115;
}

static void format_mmss(double sec, char *out, size_t out_sz)
{
    if (sec < 0.0 || isnan(sec) || isinf(sec)) sec = 0.0;
    int total = (int)(sec + 0.25);
    int mins = total / 60;
    int secs = total % 60;
    if (mins > 99) mins = 99;
    snprintf(out, out_sz, "%02d:%02d", mins, secs);
}

void ui_update_physics(AppState *app, PlayState play_state, const AudioVizSnapshot *viz)
{
    bool want_drawer = (app->modal == MODAL_LIBRARY ||
                        app->modal == MODAL_ADD_TO_PLAYLIST ||
                        app->modal == MODAL_NEW_PLAYLIST_NAME);
    float target_drawer = want_drawer ? 1.0f : 0.0f;
    app->drawer_anim += (target_drawer - app->drawer_anim) * 0.25f;
    if (fabsf(target_drawer - app->drawer_anim) < 0.005f) app->drawer_anim = target_drawer;

    float target_theme = (app->modal == MODAL_THEME_STUDIO) ? 1.0f : 0.0f;
    app->theme_anim += (target_theme - app->theme_anim) * 0.28f;
    if (fabsf(target_theme - app->theme_anim) < 0.005f) app->theme_anim = target_theme;

    float target_rpm = (play_state == PLAY_STATE_PLAYING) ? 1.0f : 0.0f;
    if (play_state == PLAY_STATE_PLAYING &&
        app->modal == MODAL_NONE &&
        (app->held_buttons & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT))) {
        target_rpm = 2.6f;
    }
    float accel = (target_rpm > app->disc_rpm_factor) ? 0.06f : 0.035f;
    app->disc_rpm_factor += (target_rpm - app->disc_rpm_factor) * accel;
    if (app->disc_rpm_factor < 0.001f) app->disc_rpm_factor = 0.0f;
    app->disc_angle_deg += app->disc_rpm_factor * 2.8f;
    if (app->disc_angle_deg >= 360.0f) app->disc_angle_deg -= 360.0f;

    float tl = (play_state == PLAY_STATE_PLAYING) ? viz->vu_left  : 0.0f;
    float tr = (play_state == PLAY_STATE_PLAYING) ? viz->vu_right : 0.0f;
    if (tl > 1.08f) tl = 1.08f;
    if (tr > 1.08f) tr = 1.08f;

    float kl = (tl > app->needle_l) ? 0.34f : 0.11f;
    float kr = (tr > app->needle_r) ? 0.34f : 0.11f;
    app->needle_vel_l = app->needle_vel_l * 0.56f + (tl - app->needle_l) * kl;
    app->needle_vel_r = app->needle_vel_r * 0.56f + (tr - app->needle_r) * kr;
    app->needle_l += app->needle_vel_l;
    app->needle_r += app->needle_vel_r;
    if (app->needle_l < 0.0f) app->needle_l = 0.0f;
    if (app->needle_r < 0.0f) app->needle_r = 0.0f;
    if (app->needle_l > 1.08f) app->needle_l = 1.08f;
    if (app->needle_r > 1.08f) app->needle_r = 1.08f;

    if (play_state == PLAY_STATE_PLAYING && (viz->peak_left > 0.86f || app->needle_l > 0.88f)) {
        app->clip_timer_l = 18;
    } else if (app->clip_timer_l > 0) {
        app->clip_timer_l--;
    }

    if (play_state == PLAY_STATE_PLAYING && (viz->peak_right > 0.86f || app->needle_r > 0.88f)) {
        app->clip_timer_r = 18;
    } else if (app->clip_timer_r > 0) {
        app->clip_timer_r--;
    }

    for (int i = 0; i < 16; i++) {
        float target = (play_state == PLAY_STATE_PLAYING) ? viz->bands[i] : 0.0f;
        if (target > app->spec_bars[i]) {
            app->spec_bars[i] += (target - app->spec_bars[i]) * 0.52f;
        } else {
            app->spec_bars[i] *= 0.86f;
            if (app->spec_bars[i] < 0.005f) app->spec_bars[i] = 0.0f;
        }

        if (app->spec_bars[i] >= app->spec_peaks[i]) {
            app->spec_peaks[i] = app->spec_bars[i];
            app->spec_peak_vel[i] = 0.0f;
        } else {
            app->spec_peak_vel[i] += 0.0018f;
            app->spec_peaks[i] -= app->spec_peak_vel[i];
            if (app->spec_peaks[i] < app->spec_bars[i]) {
                app->spec_peaks[i] = app->spec_bars[i];
            }
        }
    }

    if (app->osd_timer > 0) app->osd_timer--;
}

/* ---------------- Hardware Icons & PlayStation Button Glyphs ---------------- */

static void draw_screw(float cx, float cy, const DeviceTheme *th)
{
    gfx_circle_fill(cx, cy, 4.0f, 12, th->bevel_lo, RGB8(32, 36, 42));
    gfx_circle_fill(cx - 0.3f, cy - 0.3f, 2.9f, 12, th->bevel_hi, th->hdr_bot);
    gfx_line(cx - 1.8f, cy - 1.0f, cx + 1.8f, cy + 1.0f, 1.0f, th->bevel_lo);
    gfx_line(cx - 1.0f, cy + 1.8f, cx + 1.0f, cy - 1.8f, 1.0f, th->bevel_lo);
}

static void draw_led(float cx, float cy, float r, bool lit, uint32_t on_col)
{
    uint32_t glow = (on_col & 0x00FFFFFFu) | (95u << 24);
    gfx_circle_fill(cx, cy, r + 1.2f, 10, RGB8(30, 34, 40), RGB8(58, 64, 72));
    if (lit) {
        gfx_circle_fill(cx, cy, r + 3.0f, 12, glow, RGBA8(0, 0, 0, 0));
        gfx_circle_fill(cx, cy, r, 10, RGB8(255, 255, 255), on_col);
    } else {
        uint32_t dim = RGBA8((on_col & 0xFF) / 4,
                             ((on_col >> 8) & 0xFF) / 4,
                             ((on_col >> 16) & 0xFF) / 4, 255);
        gfx_circle_fill(cx, cy, r, 10, dim, RGB8(20, 24, 28));
    }
}

/* Custom Theme Emblem Icon replacing old G-SHOCK text on header bar */
static void draw_theme_emblem_icon(float x, float y, ThemeIconType type,
                                   uint32_t primary_col, uint32_t accent_col)
{
    /* x, y is 16x12 icon box */
    switch (type) {
        case ICON_MINIDISC:
            /* MiniDisc cartridge shell + optical ring + top shutter */
            gfx_rect_outline(x + 1.0f, y, 13.0f, 12.0f, 1.0f, primary_col);
            gfx_fill_rect(x + 7.0f, y + 1.0f, 6.0f, 3.0f, accent_col);
            gfx_ring(x + 7.5f, y + 6.5f, 2.0f, 4.2f, 14, accent_col);
            break;

        case ICON_HIFI_SPEAKER:
            /* Classic Hi-Fi monitor cabinet with tweeter + woofer cones */
            gfx_rect_outline(x + 2.0f, y, 11.0f, 12.0f, 1.0f, primary_col);
            gfx_circle_fill(x + 7.5f, y + 3.2f, 1.5f, 10, accent_col, primary_col);
            gfx_ring(x + 7.5f, y + 8.2f, 1.2f, 2.8f, 12, accent_col);
            break;

        case ICON_HEADPHONES:
            /* Studio headphones headband arch + left/right earcups */
            gfx_arc_band(x + 7.5f, y + 6.5f, 4.5f, 6.0f, -180.0f, 0.0f, 12, primary_col);
            gfx_fill_rect(x + 1.2f, y + 6.0f, 3.0f, 5.5f, accent_col);
            gfx_fill_rect(x + 10.8f, y + 6.0f, 3.0f, 5.5f, accent_col);
            break;

        case ICON_LASER_DISC:
            /* Optical CD ring + red/cyan laser pickup beam */
            gfx_ring(x + 7.5f, y + 6.0f, 2.0f, 5.6f, 16, primary_col);
            gfx_circle_fill(x + 11.2f, y + 8.8f, 1.8f, 8, accent_col, accent_col);
            break;

        case ICON_EQ_SLIDERS:
            /* 3 vertical graphic EQ faders */
            gfx_fill_rect(x + 2.5f, y + 1.0f, 1.2f, 10.0f, primary_col);
            gfx_fill_rect(x + 7.0f, y + 1.0f, 1.2f, 10.0f, primary_col);
            gfx_fill_rect(x + 11.5f, y + 1.0f, 1.2f, 10.0f, primary_col);
            gfx_fill_rect(x + 1.0f, y + 3.0f, 4.2f, 2.2f, accent_col);
            gfx_fill_rect(x + 5.5f, y + 6.8f, 4.2f, 2.2f, accent_col);
            gfx_fill_rect(x + 10.0f, y + 2.2f, 4.2f, 2.2f, accent_col);
            break;

        case ICON_MUSIC_NOTE:
        default:
            /* Beamed stereo eighth notes */
            gfx_fill_rect(x + 4.5f, y + 1.0f, 8.5f, 2.0f, accent_col);
            gfx_fill_rect(x + 4.5f, y + 1.0f, 1.4f, 8.5f, primary_col);
            gfx_fill_rect(x + 11.6f, y + 1.0f, 1.4f, 7.5f, primary_col);
            gfx_circle_fill(x + 3.6f, y + 9.5f, 2.2f, 10, accent_col, primary_col);
            gfx_circle_fill(x + 10.7f, y + 8.5f, 2.2f, 10, accent_col, primary_col);
            break;
    }
}

typedef enum {
    BTN_GLYPH_CROSS = 0,
    BTN_GLYPH_CIRCLE,
    BTN_GLYPH_SQUARE,
    BTN_GLYPH_TRIANGLE
} PsBtnGlyph;

/* Crisp drawn PlayStation face button icon (11x11 round cap + geometric symbol) */
static float draw_ps_face_btn(float x, float y, PsBtnGlyph glyph, bool pressed,
                              const char *label, uint32_t text_col, uint32_t cap_bg)
{
    float cx = x + 5.5f;
    float cy = y + 5.0f;
    uint32_t bg = pressed ? RGB8(72, 92, 118) : cap_bg;
    gfx_circle_fill(cx, cy, 5.6f, 14, bg, RGB8(18, 22, 28));

    switch (glyph) {
        case BTN_GLYPH_CROSS: {
            uint32_t c = RGB8(116, 196, 255);
            gfx_line(cx - 2.6f, cy - 2.6f, cx + 2.6f, cy + 2.6f, 1.5f, c);
            gfx_line(cx + 2.6f, cy - 2.6f, cx - 2.6f, cy + 2.6f, 1.5f, c);
            break;
        }
        case BTN_GLYPH_CIRCLE: {
            uint32_t c = RGB8(255, 118, 118);
            gfx_ring(cx, cy, 2.1f, 3.4f, 14, c);
            break;
        }
        case BTN_GLYPH_SQUARE: {
            uint32_t c = RGB8(244, 152, 228);
            gfx_rect_outline(cx - 2.7f, cy - 2.7f, 5.4f, 5.4f, 1.3f, c);
            break;
        }
        case BTN_GLYPH_TRIANGLE: {
            uint32_t c = RGB8(104, 242, 172);
            gfx_line(cx, cy - 3.0f, cx - 3.0f, cy + 2.4f, 1.3f, c);
            gfx_line(cx - 3.0f, cy + 2.4f, cx + 3.0f, cy + 2.4f, 1.3f, c);
            gfx_line(cx + 3.0f, cy + 2.4f, cx, cy - 3.0f, 1.3f, c);
            break;
        }
    }

    font_draw_utf8(x + 14.0f, y + 1.5f, label, 1.0f, text_col);
    return x + 14.0f + font_measure_utf8(label, 1.0f);
}

/* Crisp pill keycap badge for L/R, D-Pad, START, SELECT */
static float draw_pill_btn(float x, float y, const char *cap_txt, bool pressed,
                           const char *label, uint32_t text_col, uint32_t cap_bg)
{
    float cw = font_measure_utf8(cap_txt, 1.0f) + 6.0f;
    uint32_t bg = pressed ? RGB8(72, 102, 132) : cap_bg;
    gfx_fill_rect(x, y, cw, 10.5f, bg);
    gfx_rect_outline(x, y, cw, 10.5f, 1.0f, RGBA8(255, 255, 255, 48));
    font_draw_utf8(x + 3.0f, y + 1.5f, cap_txt, 1.0f, RGB8(236, 244, 252));

    font_draw_utf8(x + cw + 4.0f, y + 1.5f, label, 1.0f, text_col);
    return x + cw + 4.0f + font_measure_utf8(label, 1.0f);
}

/* ---------------- Main Deck Sections ---------------- */

static void draw_metallic_chassis(const AppState *app, PlayState play_state)
{
    const DeviceTheme *th = get_theme(app);

    /* Brushed metallic outer chassis */
    gfx_grad_rect_4(0, 0, SCREEN_W, SCREEN_H,
                    th->body_tl, th->body_tr, th->body_bl, th->body_br);

    for (int y = 2; y < SCREEN_H; y += 3) {
        uint32_t line_col = (y & 1)
            ? RGBA8(255, 255, 255, 14)
            : RGBA8(  0,   0,   0, 14);
        gfx_fill_rect(2.0f, (float)y, (float)(SCREEN_W - 4), 1.0f, line_col);
    }

    gfx_rect_outline(0, 0, SCREEN_W, SCREEN_H, 1.0f, th->bevel_hi);
    gfx_rect_outline(1, 1, SCREEN_W - 2, SCREEN_H - 2, 1.0f, th->bevel_lo);

    /* Top header strip (y = 3..22) */
    gfx_bevel_rect(4, 3, SCREEN_W - 8, 19,
                   th->hdr_top, th->hdr_bot, th->bevel_hi, th->bevel_lo);

    draw_screw(12.0f, 12.5f, th);
    draw_screw(468.0f, 12.5f, th);

    draw_led(24.5f, 12.5f, 2.5f, play_state == PLAY_STATE_PLAYING, th->accent_led);

    /* Brand Name: E-RENCYK */
    font_draw_utf8(33.0f, 8.5f, "E-RENCYK", 1.0f, th->hdr_text_main);
    char edition_lbl[64];
    snprintf(edition_lbl, sizeof(edition_lbl), "// %s", th->name);
    font_draw_utf8(87.0f, 8.5f, edition_lbl, 1.0f, th->hdr_text_sub);

    /* Right side: HOLD indicator + Theme Hardware Emblem Icon + Clock + Battery */
    float rx = 456.0f;

    /* Battery Gauge */
    gfx_bevel_rect(rx - 25.0f, 7.0f, 22.0f, 11.0f,
                   RGB8(36, 40, 48), RGB8(22, 26, 32),
                   th->bevel_lo, th->bevel_hi);
    gfx_fill_rect(rx - 3.0f, 9.5f, 2.0f, 6.0f, th->hdr_text_sub);

    int bat = app->battery_pct;
    if (bat < 0) bat = 100;
    int bars = (bat + 19) / 25;
    if (bars > 4) bars = 4;
    uint32_t bat_col = app->battery_charging
        ? RGB8(255, 196, 64)
        : (bat <= 20 ? RGB8(255, 76, 76) : th->accent_led);
    for (int b = 0; b < bars; b++) {
        gfx_fill_rect(rx - 23.0f + (float)(b * 5), 9.0f, 4.0f, 7.0f, bat_col);
    }

    char clk_buf[16];
    snprintf(clk_buf, sizeof(clk_buf), "%02d:%02d", app->clock_hour, app->clock_min);
    font_draw_utf8(rx - 60.0f, 8.5f, clk_buf, 1.0f, th->hdr_text_main);

    /* Theme Emblem Badge (replaces old G-SHOCK badge with theme-specific icon!) */
    gfx_bevel_rect(rx - 86.0f, 5.5f, 21.0f, 14.0f,
                   th->chamber_top, th->chamber_bot,
                   th->bevel_lo, th->bevel_hi);
    draw_theme_emblem_icon(rx - 83.0f, 6.5f, th->icon_type,
                           RGB8(224, 232, 242), th->accent_led);

    if (app->hold_locked) {
        gfx_bevel_rect(rx - 130.0f, 6.5f, 40.0f, 12.0f,
                       RGB8(224, 134, 24), RGB8(178, 92, 10),
                       RGB8(255, 192, 92), RGB8(96, 48, 4));
        font_draw_utf8(rx - 122.0f, 8.5f, "HOLD", 1.0f, RGB8(255, 250, 230));
    }
}

static void draw_optical_cover_bay(const AppState *app, PlayState play_state, const TrackInfo *track)
{
    const DeviceTheme *th = get_theme(app);

    /* Left Optical Bay Frame (y = 25 .. 232, height 207px) */
    gfx_bevel_rect(8.0f, 25.0f, 146.0f, 207.0f,
                   th->module_frame_top, th->module_frame_bot,
                   th->bevel_hi, th->bevel_lo);

    gfx_bevel_rect(13.0f, 30.0f, 136.0f, 197.0f,
                   th->chamber_top, th->chamber_bot,
                   RGB8(10, 12, 16), th->bevel_hi);

    font_draw_utf8(19.0f, 34.5f, "OPTICAL DISC BAY", 1.0f, th->chamber_text);
    bool laser_pulse = (play_state == PLAY_STATE_PLAYING) && (((int)(app->disc_angle_deg / 18.0f) & 1) == 0);
    draw_led(139.0f, 38.0f, 2.3f,
             play_state == PLAY_STATE_PLAYING,
             laser_pulse ? RGB8(255, 82, 96) : RGB8(216, 46, 62));

    /* Spinning Optical MiniDisc Platter */
    gfx_scissor(15, 44, 132, 116);
    float dcx = 97.0f, dcy = 102.0f, dr = 48.0f;

    gfx_circle_fill(dcx + 2.0f, dcy + 3.0f, dr + 1.0f, 28, RGBA8(0, 0, 0, 140), RGBA8(0, 0, 0, 0));
    gfx_circle_fill(dcx, dcy, dr, 32, RGB8(218, 226, 236), RGB8(144, 154, 168));

    float a = app->disc_angle_deg;
    uint32_t prism_a = (th->fader_left  & 0x00FFFFFFu) | (160u << 24);
    uint32_t prism_b = (th->fader_right & 0x00FFFFFFu) | (150u << 24);
    gfx_sector_grad(dcx, dcy, 14.0f, dr - 2.0f, a + 15.0f,  a + 68.0f,  10,
                    prism_a, RGBA8(190, 140, 255, 30));
    gfx_sector_grad(dcx, dcy, 14.0f, dr - 2.0f, a + 195.0f, a + 248.0f, 10,
                    prism_b, RGBA8(255, 190, 120, 30));
    gfx_sector_grad(dcx, dcy, 14.0f, dr - 2.0f, a + 105.0f, a + 135.0f, 6,
                    RGBA8(255, 255, 255, 120), RGBA8(255, 255, 255, 10));
    gfx_sector_grad(dcx, dcy, 14.0f, dr - 2.0f, a + 285.0f, a + 315.0f, 6,
                    RGBA8(255, 255, 255, 120), RGBA8(255, 255, 255, 10));

    gfx_ring(dcx, dcy, dr - 1.5f, dr, 32, RGB8(236, 242, 250));
    gfx_ring(dcx, dcy, 30.0f, 30.8f, 28, RGBA8(90, 102, 118, 90));
    gfx_circle_fill(dcx, dcy, 14.0f, 20, RGB8(198, 206, 216), RGB8(96, 104, 116));
    gfx_circle_fill(dcx, dcy, 5.2f,  14, RGB8(28, 32, 38),     RGB8(58, 64, 72));
    gfx_scissor_reset();

    /* 102x102 Album Cover Art Window */
    float cov_x = 18.0f, cov_y = 49.0f, cov_s = 102.0f;
    gfx_fill_rect(cov_x + 3.0f, cov_y + 3.0f, cov_s + 4.0f, cov_s + 4.0f, RGBA8(0, 0, 0, 140));
    gfx_bevel_rect(cov_x - 2.0f, cov_y - 2.0f, cov_s + 4.0f, cov_s + 4.0f,
                   th->bevel_hi, th->bevel_lo,
                   RGB8(255, 255, 255), RGB8(38, 42, 50));

    gfx_draw_tex(cover_get_texture(), COVER_TEX_SIZE, COVER_TEX_SIZE,
                 0.0f, 0.0f, (float)COVER_TEX_SIZE, (float)COVER_TEX_SIZE,
                 cov_x, cov_y, cov_s, cov_s,
                 0xFFFFFFFFu, true);

    gfx_grad_rect_4(cov_x, cov_y, cov_s, cov_s * 0.40f,
                    RGBA8(255, 255, 255, 40), RGBA8(255, 255, 255, 8),
                    RGBA8(255, 255, 255,  4), RGBA8(255, 255, 255, 0));
    gfx_rect_outline(cov_x, cov_y, cov_s, cov_s, 1.0f, RGBA8(255, 255, 255, 55));

    const char *art_badge = "MINIDISC ART";
    if (track->cover_source == 1) art_badge = "ID3v2 COVER ART";
    else if (track->cover_source == 2) art_badge = "ALBUM COVER ART";
    font_draw_utf8(19.0f, 157.0f, art_badge, 1.0f, th->chamber_text);

    /* Volume Fader */
    char vol_lbl[24];
    snprintf(vol_lbl, sizeof(vol_lbl), "VOLUME %3d%%", app->volume_pct);
    font_draw_utf8(19.0f, 171.0f, vol_lbl, 1.0f, RGB8(224, 232, 240));

    float fx = 20.0f, fy = 183.0f, fw = 122.0f;
    gfx_bevel_rect(fx, fy, fw, 6.0f,
                   RGB8(12, 15, 20), RGB8(24, 28, 34),
                   RGB8(8, 10, 14), RGB8(84, 92, 104));
    float fill_w = (fw - 2.0f) * ((float)app->volume_pct / 100.0f);
    gfx_grad_rect_h(fx + 1.0f, fy + 1.0f, fill_w, 4.0f, th->fader_left, th->fader_right);

    float kx = fx + fill_w - 4.0f;
    if (kx < fx - 1.0f) kx = fx - 1.0f;
    if (kx > fx + fw - 7.0f) kx = fx + fw - 7.0f;
    gfx_bevel_rect(kx, fy - 2.5f, 8.0f, 11.0f,
                   RGB8(236, 242, 248), RGB8(148, 156, 168),
                   RGB8(255, 255, 255), RGB8(42, 48, 56));

    /* DSP / EQ Hardware Pill */
    bool eq_active = (app->eq_preset != EQ_FLAT);
    gfx_bevel_rect(19.0f, 201.0f, 124.0f, 19.0f,
                   eq_active ? RGB8(46, 56, 70) : RGB8(30, 34, 42),
                   eq_active ? RGB8(26, 34, 44) : RGB8(20, 24, 30),
                   RGB8(84, 96, 112), RGB8(12, 14, 18));
    draw_led(28.0f, 210.5f, 2.3f, eq_active, th->accent_led);
    font_draw_utf8(36.0f, 206.5f, audio_eq_name(app->eq_preset), 1.0f,
                   eq_active ? RGB8(244, 248, 255) : RGB8(168, 178, 190));
}

static void draw_lcd_badge(float x, float y, float w, const char *label,
                           const LcdPalette *pal, bool inverted)
{
    if (inverted) {
        gfx_fill_rect(x, y, w, 11.0f, pal->badge_bg);
        float tw = font_measure_utf8(label, 1.0f);
        font_draw_utf8(x + (w - tw) * 0.5f, y + 2.0f, label, 1.0f, pal->badge_fg);
    } else {
        gfx_rect_outline(x, y, w, 11.0f, 1.0f, pal->ink_mid);
        float tw = font_measure_utf8(label, 1.0f);
        font_draw_utf8(x + (w - tw) * 0.5f, y + 2.0f, label, 1.0f, pal->ink_mid);
    }
}

static void get_active_index_counts(const AppState *app, int *out_idx1, int *out_total, const char **out_src_tag)
{
    if (app->play_source == PLAY_SRC_QUEUE && app->queue.count > 0) {
        *out_idx1 = (app->queue.playing_index >= 0) ? (app->queue.playing_index + 1) : 1;
        *out_total = app->queue.count;
        *out_src_tag = "QUEUE";
    } else if (app->play_source == PLAY_SRC_PLAYLIST && app->playlists.active_count > 0) {
        *out_idx1 = (app->playlists.active_playing_index >= 0) ? (app->playlists.active_playing_index + 1) : 1;
        *out_total = app->playlists.active_count;
        *out_src_tag = "PLIST";
    } else {
        *out_idx1 = (app->browser.current_track_pos >= 0) ? (app->browser.current_track_pos + 1) : 0;
        *out_total = app->browser.track_count;
        *out_src_tag = "TRK";
    }
}

static void draw_backlit_lcd(AppState *app_mut, const AppState *app,
                             PlayState play_state, const TrackInfo *track)
{
    const DeviceTheme *th = get_theme(app);
    const LcdPalette *pal = get_lcd_palette(app);

    /* LCD Bezel (y = 25 .. 131, height 106px) */
    gfx_bevel_rect(160.0f, 25.0f, 312.0f, 106.0f,
                   RGB8(58, 64, 74), RGB8(36, 42, 50),
                   RGB8(16, 20, 24), th->bevel_hi);

    float lx = 165.0f, ly = 30.0f, lw = 302.0f, lh = 96.0f;
    gfx_grad_rect_v(lx, ly, lw, lh, pal->bg_top, pal->bg_bot);

    gfx_grad_rect_h(lx, ly, 16.0f, lh, pal->glow, RGBA8(0, 0, 0, 0));
    gfx_grad_rect_h(lx + lw - 16.0f, ly, 16.0f, lh, RGBA8(0, 0, 0, 0), pal->glow);

    for (int sy = (int)ly + 2; sy < (int)(ly + lh - 1); sy += 2) {
        gfx_fill_rect(lx + 2.0f, (float)sy, lw - 4.0f, 1.0f, RGBA8(0, 0, 0, 26));
    }
    gfx_rect_outline(lx, ly, lw, lh, 1.0f, RGBA8(0, 0, 0, 150));

    /* Row 1: Playback State + Codec + Sample Rate + Play Mode */
    const char *st_txt = "STOP";
    if (play_state == PLAY_STATE_PLAYING) st_txt = "PLAY >";
    else if (play_state == PLAY_STATE_PAUSED) st_txt = "PAUSE ||";
    draw_lcd_badge(lx + 6.0f, ly + 4.0f, 56.0f, st_txt, pal, play_state == PLAY_STATE_PLAYING);

    char rate_badge[32];
    if (track->path[0]) {
        snprintf(rate_badge, sizeof(rate_badge), "%s %dk",
                 track->is_vbr ? "VBR" : "MP3", track->bitrate_kbps);
    } else {
        snprintf(rate_badge, sizeof(rate_badge), "NO DISC");
    }
    draw_lcd_badge(lx + 66.0f, ly + 4.0f, 66.0f, rate_badge, pal, false);

    char khz_badge[32];
    snprintf(khz_badge, sizeof(khz_badge), "%.1fk %s",
             (double)(track->sample_rate_hz > 0 ? track->sample_rate_hz : 44100) / 1000.0,
             track->is_stereo ? "ST" : "MO");
    draw_lcd_badge(lx + 136.0f, ly + 4.0f, 66.0f, khz_badge, pal, false);

    const char *pm_short = "RPT ALL";
    if (app->play_mode == PLAY_MODE_REPEAT_ONE) pm_short = "RPT ONE";
    else if (app->play_mode == PLAY_MODE_SHUFFLE) pm_short = "SHUFFLE";
    else if (app->play_mode == PLAY_MODE_NORMAL)  pm_short = "PLAY 1X";
    draw_lcd_badge(lx + 206.0f, ly + 4.0f, 90.0f, pm_short, pal,
                   app->play_mode != PLAY_MODE_NORMAL);

    /* Row 2: 7-Segment Track Index & Elapsed Time */
    int idx1 = 0, total_cnt = 0;
    const char *src_tag = "TRK";
    get_active_index_counts(app, &idx1, &total_cnt, &src_tag);

    font_draw_utf8(lx + 7.0f, ly + 19.0f, src_tag, 1.0f, pal->ink_mid);

    char trk_digits[8];
    int disp_trk = idx1 > 99 ? 99 : idx1;
    snprintf(trk_digits, sizeof(trk_digits), "%02d", disp_trk);
    font_draw_7seg(lx + 38.0f, ly + 18.0f, 9.5f, 18.0f, 2.1f,
                   trk_digits, pal->ink_bright, pal->ink_dim);

    char total_trk_str[16];
    snprintf(total_trk_str, sizeof(total_trk_str), "/%02d", total_cnt > 99 ? 99 : total_cnt);
    font_draw_utf8(lx + 64.0f, ly + 28.0f, total_trk_str, 1.0f, pal->ink_mid);

    gfx_fill_rect(lx + 92.0f, ly + 18.0f, 1.0f, 20.0f, pal->ink_dim);

    char pos_str[16], dur_str[24], raw_dur[16];
    format_mmss(track->position_sec, pos_str, sizeof(pos_str));
    format_mmss(track->duration_sec, raw_dur, sizeof(raw_dur));
    snprintf(dur_str, sizeof(dur_str), "/ %s", raw_dur);

    font_draw_utf8(lx + 99.0f, ly + 19.0f, "TIME", 1.0f, pal->ink_mid);
    font_draw_7seg(lx + 130.0f, ly + 17.5f, 11.5f, 21.0f, 2.4f,
                   pos_str, pal->ink_bright, pal->ink_dim);
    font_draw_utf8(lx + 214.0f, ly + 29.0f, dur_str, 1.0f, pal->ink_mid);

    gfx_fill_rect(lx + 6.0f, ly + 42.5f, lw - 12.0f, 1.0f, pal->ink_dim);

    /* Row 3: Smooth Marquee Scrolling Title */
    const char *title_txt = track->title[0]
        ? track->title
        : "Press Circle (O) to Open Music Library";
    float title_scale = 1.35f;
    float box_w = lw - 14.0f;
    float title_w = font_measure_utf8(title_txt, title_scale);

    gfx_scissor((int)(lx + 7.0f), (int)(ly + 45.0f), (int)box_w, 15);
    if (title_w <= box_w) {
        app_mut->marquee_px = 0.0f;
        font_draw_utf8(lx + 7.0f, ly + 47.0f, title_txt, title_scale, pal->ink_bright);
    } else {
        float gap = 42.0f;
        if (app_mut->marquee_pause > 0) {
            app_mut->marquee_pause--;
        } else {
            app_mut->marquee_px += 0.65f;
            if (app_mut->marquee_px >= title_w + gap) {
                app_mut->marquee_px = 0.0f;
                app_mut->marquee_pause = 75;
            }
        }
        float x1 = lx + 7.0f - app_mut->marquee_px;
        font_draw_utf8(x1, ly + 47.0f, title_txt, title_scale, pal->ink_bright);
        font_draw_utf8(x1 + title_w + gap, ly + 47.0f, title_txt, title_scale, pal->ink_bright);
    }
    gfx_scissor_reset();

    /* Row 4: Artist — Album */
    char sub_line[300];
    if (track->path[0]) {
        if (track->year[0]) {
            snprintf(sub_line, sizeof(sub_line), "%s  -  %s (%s)",
                     track->artist, track->album, track->year);
        } else {
            snprintf(sub_line, sizeof(sub_line), "%s  -  %s",
                     track->artist, track->album);
        }
    } else {
        snprintf(sub_line, sizeof(sub_line), "Place MP3 files in ms0:/MUSIC or ./music");
    }
    font_draw_utf8_ellipsis(lx + 7.0f, ly + 64.0f, box_w, sub_line, 1.0f, pal->ink_mid);

    /* Row 5: Segmented Optical Progress Bar */
    float px = lx + 7.0f, py = ly + 79.0f, pw = lw - 14.0f, ph = 9.0f;
    gfx_rect_outline(px, py, pw, ph, 1.0f, pal->ink_mid);

    double prog = (track->duration_sec > 0.05) ? (track->position_sec / track->duration_sec) : 0.0;
    if (prog < 0.0) prog = 0.0;
    if (prog > 1.0) prog = 1.0;

    int total_segs = 36;
    int lit_segs = (int)(prog * (double)total_segs + 0.5);
    float seg_step = (pw - 4.0f) / (float)total_segs;
    for (int s = 0; s < total_segs; s++) {
        float sx = px + 2.0f + (float)s * seg_step;
        gfx_fill_rect(sx, py + 2.0f, seg_step - 1.4f, ph - 4.0f,
                      (s < lit_segs) ? pal->ink_bright : pal->ink_dim);
    }

    /* Toast OSD Popup inside LCD */
    if (app->osd_timer > 0 && app->osd_msg[0]) {
        float bw = font_measure_utf8(app->osd_msg, 1.0f) + 20.0f;
        if (bw < 130.0f) bw = 130.0f;
        if (bw > lw - 12.0f) bw = lw - 12.0f;
        float bx = lx + (lw - bw) * 0.5f;
        float by = ly + 3.5f;
        gfx_fill_rect(bx - 1.0f, by - 1.0f, bw + 2.0f, 15.0f, pal->bg_bot);
        gfx_fill_rect(bx, by, bw, 13.0f, pal->badge_bg);
        float tw = font_measure_utf8(app->osd_msg, 1.0f);
        font_draw_utf8(bx + (bw - tw) * 0.5f, by + 2.5f, app->osd_msg, 1.0f, pal->badge_fg);
    }
}

/* ---------------- Analog VU / Spectrum / Oscilloscope Module ---------------- */

static void draw_single_vu_gauge(float x, float y, float w, float h,
                                 const char *ch_label, float level, bool clip_on,
                                 const DeviceTheme *th)
{
    gfx_bevel_rect(x, y, w, h,
                   RGB8(40, 44, 52), RGB8(24, 28, 34),
                   RGB8(12, 14, 18), th->bevel_hi);

    float ix = x + 3.0f, iy = y + 3.0f, iw = w - 6.0f, ih = h - 6.0f;
    gfx_grad_rect_4(ix, iy, iw, ih,
                    th->vu_face_tl, th->vu_face_tr, th->vu_face_bl, th->vu_face_br);

    gfx_circle_fill(ix + iw * 0.5f, iy + 6.0f, iw * 0.55f, 18,
                    th->vu_bulb_glow, RGBA8(0, 0, 0, 0));

    gfx_scissor((int)ix, (int)iy, (int)iw, (int)ih);

    float px = ix + iw * 0.5f;
    float py = iy + ih + 25.0f;
    float arc_r = ih + 5.0f;

    gfx_arc_band(px, py, arc_r - 1.5f, arc_r,        -130.0f, -68.0f, 18, th->vu_scale_ink);
    gfx_arc_band(px, py, arc_r - 3.5f, arc_r + 0.5f,  -68.0f, -50.0f,  8, th->vu_red_ink);

    static const float tick_norm[7] = { 0.0f, 0.22f, 0.42f, 0.60f, 0.77f, 0.90f, 1.0f };
    for (int t = 0; t < 7; t++) {
        float deg = -130.0f + tick_norm[t] * 80.0f;
        float rad = deg * ((float)M_PI / 180.0f);
        float cs = cosf(rad), sn = sinf(rad);
        uint32_t tc = (t >= 4) ? th->vu_red_ink : th->vu_scale_ink;
        gfx_line(px + cs * (arc_r - 6.5f), py + sn * (arc_r - 6.5f),
                 px + cs * (arc_r + 1.5f), py + sn * (arc_r + 1.5f),
                 (t == 4 || t == 0 || t == 6) ? 1.6f : 1.0f, tc);
    }

    font_draw_utf8(ix + 8.0f,   iy + 23.0f, "-20", 1.0f, th->vu_scale_ink);
    font_draw_utf8(ix + 40.0f,  iy + 11.0f, "-7",  1.0f, th->vu_scale_ink);
    font_draw_utf8(ix + 69.0f,  iy + 8.0f,  "-3",  1.0f, th->vu_scale_ink);
    font_draw_utf8(ix + 96.0f,  iy + 12.0f, "0",   1.0f, th->vu_red_ink);
    font_draw_utf8(ix + 114.0f, iy + 22.0f, "+3",  1.0f, th->vu_red_ink);

    font_draw_utf8(ix + iw * 0.5f - 6.0f, iy + 39.0f, "VU", 1.15f, th->vu_scale_ink);
    font_draw_utf8(ix + 6.0f, iy + ih - 12.0f, ch_label, 1.0f, th->vu_scale_ink);

    font_draw_utf8(ix + iw - 38.0f, iy + 5.0f, "PEAK", 1.0f, th->vu_red_ink);
    draw_led(ix + iw - 8.0f, iy + 8.5f, 2.5f, clip_on, RGB8(255, 42, 36));

    float clamped = level;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > 1.06f) clamped = 1.06f;
    float ndeg = -130.0f + (clamped / 1.06f) * 82.0f;
    float nrad = ndeg * ((float)M_PI / 180.0f);
    float nx = cosf(nrad), ny = sinf(nrad);

    gfx_line(px + 2.0f, py + 2.0f,
             px + nx * (arc_r + 3.0f) + 2.0f, py + ny * (arc_r + 3.0f) + 2.0f,
             1.8f, RGBA8(0, 0, 0, 60));

    gfx_line(px, py,
             px + nx * (arc_r * 0.72f), py + ny * (arc_r * 0.72f),
             1.9f, th->vu_needle_shaft);
    gfx_line(px + nx * (arc_r * 0.68f), py + ny * (arc_r * 0.68f),
             px + nx * (arc_r + 3.5f),  py + ny * (arc_r + 3.5f),
             1.8f, th->vu_needle_tip);

    gfx_circle_fill(px, iy + ih + 3.0f, 10.5f, 16, RGB8(72, 78, 88), RGB8(28, 32, 38));
    gfx_scissor_reset();

    gfx_grad_rect_v(ix, iy, iw, 12.0f, RGBA8(255, 255, 255, 44), RGBA8(255, 255, 255, 0));
    gfx_rect_outline(ix, iy, iw, ih, 1.0f, RGBA8(0, 0, 0, 120));
}

static void draw_vfd_spectrum(const AppState *app, const DeviceTheme *th)
{
    float x = 165.0f, y = 139.0f, w = 302.0f, h = 88.0f;
    gfx_grad_rect_v(x, y, w, h, RGB8(10, 16, 22), RGB8(6, 10, 14));
    gfx_rect_outline(x, y, w, h, 1.0f, RGB8(20, 30, 40));

    font_draw_utf8(x + 7.0f, y + 5.0f, "16-BAND VFD SPECTRUM ANALYZER", 1.0f, th->spec_low);
    font_draw_utf8(x + w - 58.0f, y + 5.0f, "PEAK HOLD", 1.0f, th->spec_mid);

    float bx0 = x + 9.0f;
    float by_bot = y + h - 14.0f;
    float max_bh = h - 31.0f;
    float bw = (w - 18.0f) / 16.0f;
    int segs = 12;
    float seg_h = max_bh / (float)segs;

    for (int i = 0; i < 16; i++) {
        float bx = bx0 + (float)i * bw;
        int lit = (int)(app->spec_bars[i] * (float)segs + 0.4f);
        int pk  = (int)(app->spec_peaks[i] * (float)segs + 0.4f);
        if (pk >= segs) pk = segs - 1;

        for (int s = 0; s < segs; s++) {
            float sy = by_bot - (float)(s + 1) * seg_h;
            uint32_t col;
            if (s < lit) {
                if (s >= 10)      col = th->spec_high;
                else if (s >= 7)  col = th->spec_mid;
                else              col = th->spec_low;
            } else {
                col = RGBA8(28, 46, 56, 105);
            }
            gfx_fill_rect(bx + 1.0f, sy + 1.0f, bw - 3.0f, seg_h - 1.5f, col);
        }

        if (pk > 0) {
            float py = by_bot - (float)(pk + 1) * seg_h;
            gfx_fill_rect(bx + 1.0f, py + 1.0f, bw - 3.0f, 2.0f, RGB8(255, 245, 200));
        }
    }

    font_draw_utf8(x +   9.0f, y + h - 11.0f, "60Hz",  1.0f, RGB8(110, 144, 160));
    font_draw_utf8(x +  78.0f, y + h - 11.0f, "250Hz", 1.0f, RGB8(110, 144, 160));
    font_draw_utf8(x + 152.0f, y + h - 11.0f, "1kHz",  1.0f, RGB8(110, 144, 160));
    font_draw_utf8(x + 216.0f, y + h - 11.0f, "4kHz",  1.0f, RGB8(110, 144, 160));
    font_draw_utf8(x + 266.0f, y + h - 11.0f, "16kHz", 1.0f, RGB8(110, 144, 160));
}

static void draw_oscilloscope(PlayState play_state, const AudioVizSnapshot *viz, const DeviceTheme *th)
{
    float x = 165.0f, y = 139.0f, w = 302.0f, h = 88.0f;
    gfx_grad_rect_v(x, y, w, h, RGB8(8, 18, 20), RGB8(4, 10, 12));

    for (int gx = 1; gx < 8; gx++) {
        gfx_fill_rect(x + (w * (float)gx) / 8.0f, y + 2.0f, 1.0f, h - 4.0f, RGBA8(50, 96, 96, 55));
    }
    gfx_fill_rect(x + 2.0f, y + h * 0.34f, w - 4.0f, 1.0f, RGBA8(50, 96, 96, 70));
    gfx_fill_rect(x + 2.0f, y + h * 0.73f, w - 4.0f, 1.0f, RGBA8(50, 96, 96, 70));

    font_draw_utf8(x + 6.0f, y + 4.0f,  "LEFT CHANNEL WAVE",  1.0f, th->spec_low);
    font_draw_utf8(x + 6.0f, y + 46.0f, "RIGHT CHANNEL WAVE", 1.0f, th->fader_left);

    gfx_scissor((int)x + 2, (int)y + 2, (int)w - 4, (int)h - 4);
    float cy_l = y + h * 0.34f;
    float cy_r = y + h * 0.73f;
    float amp = 15.5f;
    float step_x = (w - 12.0f) / 127.0f;

    for (int i = 0; i < 127; i++) {
        float x0 = x + 6.0f + (float)i * step_x;
        float x1 = x0 + step_x;
        float sl0 = (play_state == PLAY_STATE_PLAYING) ? ((float)viz->wave_l[i]     / 32768.0f) : 0.0f;
        float sl1 = (play_state == PLAY_STATE_PLAYING) ? ((float)viz->wave_l[i + 1] / 32768.0f) : 0.0f;
        float sr0 = (play_state == PLAY_STATE_PLAYING) ? ((float)viz->wave_r[i]     / 32768.0f) : 0.0f;
        float sr1 = (play_state == PLAY_STATE_PLAYING) ? ((float)viz->wave_r[i + 1] / 32768.0f) : 0.0f;

        gfx_line(x0, cy_l - sl0 * amp, x1, cy_l - sl1 * amp, 1.5f, th->spec_low);
        gfx_line(x0, cy_r - sr0 * amp, x1, cy_r - sr1 * amp, 1.5f, th->fader_left);
    }
    gfx_scissor_reset();
    gfx_rect_outline(x, y, w, h, 1.0f, RGB8(24, 48, 52));
}

static void draw_meter_bay(const AppState *app, PlayState play_state, const AudioVizSnapshot *viz)
{
    const DeviceTheme *th = get_theme(app);

    /* Meter Bay Housing (y = 134 .. 232, height 98px) */
    gfx_bevel_rect(160.0f, 134.0f, 312.0f, 98.0f,
                   RGB8(62, 68, 78), RGB8(38, 44, 52),
                   RGB8(18, 22, 28), th->bevel_hi);

    if (app->vis_mode == VIS_ANALOG_VU) {
        draw_single_vu_gauge(165.0f, 139.0f, 148.0f, 88.0f,
                             "LEFT CH", app->needle_l, app->clip_timer_l > 0, th);
        draw_single_vu_gauge(319.0f, 139.0f, 148.0f, 88.0f,
                             "RIGHT CH", app->needle_r, app->clip_timer_r > 0, th);
    } else if (app->vis_mode == VIS_VFD_SPECTRUM) {
        draw_vfd_spectrum(app, th);
    } else {
        draw_oscilloscope(play_state, viz, th);
    }
}

/* ---------------- Contextual 2-Row Self-Explanatory Control Guide ---------------- */

static void draw_control_guide_bar(const AppState *app, PlayState play_state)
{
    const DeviceTheme *th = get_theme(app);
    uint32_t b = app->held_buttons;

    /* Dedicated two-row machined control legend panel (y = 235 .. 269, height 34px) */
    gfx_bevel_rect(6.0f, 235.0f, SCREEN_W - 12.0f, 34.0f,
                   th->bar_top, th->bar_bot, th->bevel_hi, th->bevel_lo);

    float r1y = 238.5f;
    float r2y = 254.0f;
    uint32_t tc  = th->bar_text;
    uint32_t tc2 = th->bar_dim_text;
    uint32_t cap = th->keycap_bg;

    /* Subtle separator line between Row 1 and Row 2 */
    gfx_fill_rect(14.0f, 251.5f, SCREEN_W - 28.0f, 1.0f, RGBA8(0, 0, 0, 26));

    if (app->modal == MODAL_THEME_STUDIO) {
        draw_ps_face_btn( 14.0f, r1y, BTN_GLYPH_CROSS,  (b & PSP_CTRL_CROSS) != 0,  "Apply & Save",  tc, cap);
        draw_ps_face_btn(132.0f, r1y, BTN_GLYPH_SQUARE, (b & PSP_CTRL_SQUARE) != 0, "Reset Default", tc, cap);
        draw_ps_face_btn(256.0f, r1y, BTN_GLYPH_CIRCLE, (b & PSP_CTRL_CIRCLE) != 0, "Close Studio",  tc, cap);

        draw_pill_btn( 14.0f, r2y, "UP/DN",  (b & (PSP_CTRL_UP | PSP_CTRL_DOWN)) != 0,     "Select Hardware Theme", tc2, cap);
        draw_pill_btn(232.0f, r2y, "L / R",  (b & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT)) != 0,  "LCD Backlight Color",   tc2, cap);
        return;
    }

    if (app->modal == MODAL_ADD_TO_PLAYLIST || app->modal == MODAL_NEW_PLAYLIST_NAME) {
        draw_ps_face_btn( 14.0f, r1y, BTN_GLYPH_CROSS,  (b & PSP_CTRL_CROSS) != 0,  "Confirm & Save", tc, cap);
        draw_ps_face_btn(156.0f, r1y, BTN_GLYPH_CIRCLE, (b & PSP_CTRL_CIRCLE) != 0, "Cancel",         tc, cap);

        draw_pill_btn( 14.0f, r2y, "UP/DN", (b & (PSP_CTRL_UP | PSP_CTRL_DOWN)) != 0, "Choose Playlist Option", tc2, cap);
        return;
    }

    if (app->modal == MODAL_LIBRARY) {
        if (app->lib_tab == LIB_TAB_FILES) {
            draw_ps_face_btn( 12.0f, r1y, BTN_GLYPH_CROSS,    (b & PSP_CTRL_CROSS) != 0,    "Play / Open",  tc, cap);
            draw_ps_face_btn(118.0f, r1y, BTN_GLYPH_SQUARE,   (b & PSP_CTRL_SQUARE) != 0,   "Add to Queue", tc, cap);
            draw_ps_face_btn(232.0f, r1y, BTN_GLYPH_TRIANGLE, (b & PSP_CTRL_TRIANGLE) != 0, "+ Playlist",   tc, cap);
            draw_ps_face_btn(336.0f, r1y, BTN_GLYPH_CIRCLE,   (b & PSP_CTRL_CIRCLE) != 0,   "Close",        tc, cap);

            draw_pill_btn( 12.0f, r2y, "L / R",  (b & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) != 0, "Switch Tab",   tc2, cap);
            draw_pill_btn(138.0f, r2y, "START",  (b & PSP_CTRL_START) != 0,                          "Change Drive", tc2, cap);
            draw_pill_btn(274.0f, r2y, "SELECT", (b & PSP_CTRL_SELECT) != 0,                         "Themes",       tc2, cap);
        } else if (app->lib_tab == LIB_TAB_QUEUE) {
            draw_ps_face_btn( 12.0f, r1y, BTN_GLYPH_CROSS,    (b & PSP_CTRL_CROSS) != 0,    "Play Track",    tc, cap);
            draw_ps_face_btn(116.0f, r1y, BTN_GLYPH_SQUARE,   (b & PSP_CTRL_SQUARE) != 0,   "Remove",        tc, cap);
            draw_ps_face_btn(204.0f, r1y, BTN_GLYPH_TRIANGLE, (b & PSP_CTRL_TRIANGLE) != 0, "Save Playlist", tc, cap);
            draw_ps_face_btn(326.0f, r1y, BTN_GLYPH_CIRCLE,   (b & PSP_CTRL_CIRCLE) != 0,   "Close",         tc, cap);

            draw_pill_btn( 12.0f, r2y, "L / R",  (b & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) != 0, "Switch Tab",  tc2, cap);
            draw_pill_btn(138.0f, r2y, "START",  (b & PSP_CTRL_START) != 0,                          "Clear Queue", tc2, cap);
            draw_pill_btn(274.0f, r2y, "SELECT", (b & PSP_CTRL_SELECT) != 0,                         "Themes",      tc2, cap);
        } else {
            if (!app->playlists.inside_playlist) {
                draw_ps_face_btn( 12.0f, r1y, BTN_GLYPH_CROSS,    (b & PSP_CTRL_CROSS) != 0,    "Open List",    tc, cap);
                draw_ps_face_btn(112.0f, r1y, BTN_GLYPH_TRIANGLE, (b & PSP_CTRL_TRIANGLE) != 0, "New Playlist", tc, cap);
                draw_ps_face_btn(228.0f, r1y, BTN_GLYPH_SQUARE,   (b & PSP_CTRL_SQUARE) != 0,   "Delete",       tc, cap);
                draw_ps_face_btn(314.0f, r1y, BTN_GLYPH_CIRCLE,   (b & PSP_CTRL_CIRCLE) != 0,   "Close",        tc, cap);

                draw_pill_btn( 12.0f, r2y, "L / R",  (b & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) != 0, "Switch Tab", tc2, cap);
                draw_pill_btn(138.0f, r2y, "START",  (b & PSP_CTRL_START) != 0,                          "Play All",   tc2, cap);
                draw_pill_btn(262.0f, r2y, "SELECT", (b & PSP_CTRL_SELECT) != 0,                         "Themes",     tc2, cap);
            } else {
                draw_ps_face_btn( 12.0f, r1y, BTN_GLYPH_CROSS,    (b & PSP_CTRL_CROSS) != 0,    "Play Song", tc, cap);
                draw_ps_face_btn(114.0f, r1y, BTN_GLYPH_SQUARE,   (b & PSP_CTRL_SQUARE) != 0,   "Remove",    tc, cap);
                draw_ps_face_btn(204.0f, r1y, BTN_GLYPH_TRIANGLE, (b & PSP_CTRL_TRIANGLE) != 0, "Back",      tc, cap);
                draw_ps_face_btn(282.0f, r1y, BTN_GLYPH_CIRCLE,   (b & PSP_CTRL_CIRCLE) != 0,   "Close",     tc, cap);

                draw_pill_btn( 12.0f, r2y, "L / R",  (b & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) != 0, "Switch Tab",   tc2, cap);
                draw_pill_btn(138.0f, r2y, "START",  (b & PSP_CTRL_START) != 0,                          "Shuffle Play", tc2, cap);
                draw_pill_btn(282.0f, r2y, "SELECT", (b & PSP_CTRL_SELECT) != 0,                         "Themes",       tc2, cap);
            }
        }
        return;
    }

    /* Default Main Hi-Fi Deck Controls — spacious, unmistakable English labels! */
    const char *play_lbl = (play_state == PLAY_STATE_PLAYING) ? "Pause" : "Play";
    draw_ps_face_btn( 14.0f, r1y, BTN_GLYPH_CROSS,    (b & PSP_CTRL_CROSS) != 0,    play_lbl,          tc, cap);
    draw_ps_face_btn( 98.0f, r1y, BTN_GLYPH_CIRCLE,   (b & PSP_CTRL_CIRCLE) != 0,   "Library & Queue", tc, cap);
    draw_ps_face_btn(228.0f, r1y, BTN_GLYPH_SQUARE,   (b & PSP_CTRL_SQUARE) != 0,   "Sound EQ",        tc, cap);
    draw_ps_face_btn(322.0f, r1y, BTN_GLYPH_TRIANGLE, (b & PSP_CTRL_TRIANGLE) != 0, "Visualizer",      tc, cap);

    draw_pill_btn( 14.0f, r2y, "L / R",  (b & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) != 0, "Prev/Next", tc2, cap);
    draw_pill_btn(126.0f, r2y, "D-PAD",  (b & (PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT)) != 0,
                  "Seek / Vol", tc2, cap);
    draw_pill_btn(246.0f, r2y, "START",  (b & PSP_CTRL_START) != 0,  "Repeat", tc2, cap);
    draw_pill_btn(350.0f, r2y, "SELECT", (b & PSP_CTRL_SELECT) != 0, "Themes", tc2, cap);
}

/* ---------------- 3-Tab Library / Queue / Playlists Drawer ---------------- */

static void draw_scrollbar(float x, float y0, float h, int visible, int total, int scroll_top,
                           const LcdPalette *pal)
{
    if (total <= visible) return;
    gfx_fill_rect(x, y0, 4.0f, h, pal->ink_dim);
    float thumb_h = (h * (float)visible) / (float)total;
    if (thumb_h < 12.0f) thumb_h = 12.0f;
    float max_top = (float)(total - visible);
    float thumb_y = y0 + ((float)scroll_top / max_top) * (h - thumb_h);
    gfx_fill_rect(x, thumb_y, 4.0f, thumb_h, pal->ink_bright);
}

static void draw_library_drawer(const AppState *app)
{
    if (app->drawer_anim <= 0.01f) return;

    const DeviceTheme *th = get_theme(app);
    const LcdPalette *pal = get_lcd_palette(app);

    float target_x = 160.0f;
    float start_x  = 484.0f;
    float dx = start_x + (target_x - start_x) * app->drawer_anim;
    float dy = 25.0f, dw = 312.0f, dh = 207.0f;

    gfx_fill_rect(dx - 6.0f, dy + 3.0f, 6.0f, dh, RGBA8(0, 0, 0, 115));
    gfx_bevel_rect(dx, dy, dw, dh,
                   th->module_frame_top, th->module_frame_bot,
                   th->bevel_hi, th->bevel_lo);

    float lx = dx + 5.0f, ly = dy + 5.0f, lw = dw - 10.0f, lh = dh - 10.0f;
    gfx_grad_rect_v(lx, ly, lw, lh, pal->bg_top, pal->bg_bot);
    gfx_rect_outline(lx, ly, lw, lh, 1.0f, RGBA8(0, 0, 0, 160));

    gfx_scissor((int)lx, (int)ly, (int)lw, (int)lh);

    /* Top 3-Tab Header Bar ([L] FILES | QUEUE (n) | PLAYLISTS [R]) */
    char q_tab_lbl[32];
    snprintf(q_tab_lbl, sizeof(q_tab_lbl), "QUEUE (%d)", app->queue.count);
    const char *tab_names[3] = { "FILES", q_tab_lbl, "PLAYLISTS" };
    float tab_w = (lw - 8.0f) / 3.0f;
    for (int t = 0; t < 3; t++) {
        float tx = lx + 4.0f + (float)t * tab_w;
        bool active = ((int)app->lib_tab == t);
        if (active) {
            gfx_fill_rect(tx, ly + 3.0f, tab_w - 2.0f, 14.0f, pal->badge_bg);
            float tw = font_measure_utf8(tab_names[t], 1.0f);
            font_draw_utf8(tx + (tab_w - 2.0f - tw) * 0.5f, ly + 6.0f,
                           tab_names[t], 1.0f, pal->badge_fg);
        } else {
            gfx_rect_outline(tx, ly + 3.0f, tab_w - 2.0f, 14.0f, 1.0f, pal->ink_dim);
            float tw = font_measure_utf8(tab_names[t], 1.0f);
            font_draw_utf8(tx + (tab_w - 2.0f - tw) * 0.5f, ly + 6.0f,
                           tab_names[t], 1.0f, pal->ink_mid);
        }
    }

    /* Sub-header context line */
    char sub_hdr[MAX_PATH_LEN + 64];
    if (app->lib_tab == LIB_TAB_FILES) {
        snprintf(sub_hdr, sizeof(sub_hdr), "Folder: %s (%d MP3)",
                 app->browser.current_dir, app->browser.track_count);
    } else if (app->lib_tab == LIB_TAB_QUEUE) {
        snprintf(sub_hdr, sizeof(sub_hdr), "Up Next Queue - %d track(s) ready",
                 app->queue.count);
    } else {
        if (app->playlists.inside_playlist && app->playlists.opened_index >= 0) {
            snprintf(sub_hdr, sizeof(sub_hdr), "Playlist: %s (%d songs)",
                     app->playlists.list[app->playlists.opened_index].name,
                     app->playlists.opened_count);
        } else {
            snprintf(sub_hdr, sizeof(sub_hdr), "Saved Playlists (.m3u) - %d total",
                     app->playlists.count);
        }
    }
    font_draw_utf8_ellipsis(lx + 6.0f, ly + 21.0f, lw - 12.0f, sub_hdr, 1.0f, pal->ink_bright);
    gfx_fill_rect(lx + 4.0f, ly + 31.0f, lw - 8.0f, 1.0f, pal->ink_dim);

    int visible_rows = 10;
    float row_y0 = ly + 34.0f;
    float row_h  = 15.8f;

    /* ---- TAB 0: FILES ---- */
    if (app->lib_tab == LIB_TAB_FILES) {
        const BrowserState *br = &app->browser;
        if (br->count == 0) {
            font_draw_utf8(lx + 12.0f, row_y0 + 36.0f, "No MP3 files found in this folder.", 1.0f, pal->ink_bright);
            font_draw_utf8(lx + 12.0f, row_y0 + 52.0f, "Press START to switch drive/folder.", 1.0f, pal->ink_mid);
        } else {
            for (int r = 0; r < visible_rows; r++) {
                int idx = br->scroll_top + r;
                if (idx >= br->count) break;
                const BrowserItem *it = &br->items[idx];
                float ry = row_y0 + (float)r * row_h;
                bool is_sel = (idx == br->selected);

                bool is_playing = (app->play_source == PLAY_SRC_FOLDER &&
                                   !it->is_dir &&
                                   br->current_track_pos >= 0 &&
                                   br->current_track_pos < br->track_count &&
                                   br->track_item_indices[br->current_track_pos] == idx);

                if (is_sel) {
                    gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, pal->badge_bg);
                } else if (r & 1) {
                    gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, RGBA8(0, 0, 0, 24));
                }

                uint32_t fg = is_sel ? pal->badge_fg : (is_playing ? pal->ink_bright : pal->ink_mid);
                char line[240];
                if (it->is_dir)      snprintf(line, sizeof(line), "[DIR] %s", it->name);
                else if (is_playing) snprintf(line, sizeof(line), "> %s", it->name);
                else                 snprintf(line, sizeof(line), "  %s", it->name);

                font_draw_utf8_ellipsis(lx + 8.0f, ry + 4.0f, lw - 22.0f, line, 1.0f, fg);
            }
            draw_scrollbar(lx + lw - 7.0f, row_y0, (float)visible_rows * row_h,
                           visible_rows, br->count, br->scroll_top, pal);
        }
    }
    /* ---- TAB 1: QUEUE ---- */
    else if (app->lib_tab == LIB_TAB_QUEUE) {
        const QueueState *q = &app->queue;
        if (q->count == 0) {
            font_draw_utf8(lx + 12.0f, row_y0 + 30.0f, "Your Play Queue is empty.", 1.0f, pal->ink_bright);
            font_draw_utf8(lx + 12.0f, row_y0 + 48.0f, "In FILES tab, press Square [] on any", 1.0f, pal->ink_mid);
            font_draw_utf8(lx + 12.0f, row_y0 + 62.0f, "song or folder to add it to Queue!", 1.0f, pal->ink_mid);
        } else {
            for (int r = 0; r < visible_rows; r++) {
                int idx = q->scroll_top + r;
                if (idx >= q->count) break;
                float ry = row_y0 + (float)r * row_h;
                bool is_sel = (idx == q->selected);
                bool is_playing = (app->play_source == PLAY_SRC_QUEUE && q->playing_index == idx);

                if (is_sel) {
                    gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, pal->badge_bg);
                } else if (r & 1) {
                    gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, RGBA8(0, 0, 0, 24));
                }

                uint32_t fg = is_sel ? pal->badge_fg : (is_playing ? pal->ink_bright : pal->ink_mid);
                char line[240];
                snprintf(line, sizeof(line), "%s%02d. %s",
                         is_playing ? "> " : "  ", idx + 1, q->tracks[idx].title);
                font_draw_utf8_ellipsis(lx + 8.0f, ry + 4.0f, lw - 22.0f, line, 1.0f, fg);
            }
            draw_scrollbar(lx + lw - 7.0f, row_y0, (float)visible_rows * row_h,
                           visible_rows, q->count, q->scroll_top, pal);
        }
    }
    /* ---- TAB 2: PLAYLISTS ---- */
    else {
        const PlaylistBank *pb = &app->playlists;
        if (!pb->inside_playlist) {
            if (pb->count == 0) {
                font_draw_utf8(lx + 12.0f, row_y0 + 28.0f, "No saved playlists yet.", 1.0f, pal->ink_bright);
                font_draw_utf8(lx + 12.0f, row_y0 + 46.0f, "Press Triangle (/\\) to create a new", 1.0f, pal->ink_mid);
                font_draw_utf8(lx + 12.0f, row_y0 + 60.0f, "playlist, or add songs from FILES!", 1.0f, pal->ink_mid);
            } else {
                for (int r = 0; r < visible_rows; r++) {
                    int idx = pb->scroll_top + r;
                    if (idx >= pb->count) break;
                    float ry = row_y0 + (float)r * row_h;
                    bool is_sel = (idx == pb->selected);

                    if (is_sel) {
                        gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, pal->badge_bg);
                    } else if (r & 1) {
                        gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, RGBA8(0, 0, 0, 24));
                    }

                    uint32_t fg = is_sel ? pal->badge_fg : pal->ink_bright;
                    char line[160];
                    snprintf(line, sizeof(line), "[M3U] %s  (%d songs)",
                             pb->list[idx].name, pb->list[idx].track_count);
                    font_draw_utf8_ellipsis(lx + 8.0f, ry + 4.0f, lw - 22.0f, line, 1.0f, fg);
                }
                draw_scrollbar(lx + lw - 7.0f, row_y0, (float)visible_rows * row_h,
                               visible_rows, pb->count, pb->scroll_top, pal);
            }
        } else {
            if (pb->opened_count == 0) {
                font_draw_utf8(lx + 12.0f, row_y0 + 34.0f, "This playlist is currently empty.", 1.0f, pal->ink_bright);
                font_draw_utf8(lx + 12.0f, row_y0 + 50.0f, "Go to FILES tab and press Triangle", 1.0f, pal->ink_mid);
                font_draw_utf8(lx + 12.0f, row_y0 + 64.0f, "(/\\) on songs to add them here.", 1.0f, pal->ink_mid);
            } else {
                for (int r = 0; r < visible_rows; r++) {
                    int idx = pb->track_scroll_top + r;
                    if (idx >= pb->opened_count) break;
                    float ry = row_y0 + (float)r * row_h;
                    bool is_sel = (idx == pb->track_selected);
                    bool is_playing = (app->play_source == PLAY_SRC_PLAYLIST &&
                                       pb->opened_index >= 0 &&
                                       strcmp(pb->active_name, pb->list[pb->opened_index].name) == 0 &&
                                       pb->active_playing_index == idx);

                    if (is_sel) {
                        gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, pal->badge_bg);
                    } else if (r & 1) {
                        gfx_fill_rect(lx + 4.0f, ry, lw - 14.0f, row_h - 1.0f, RGBA8(0, 0, 0, 24));
                    }

                    uint32_t fg = is_sel ? pal->badge_fg : (is_playing ? pal->ink_bright : pal->ink_mid);
                    char line[240];
                    snprintf(line, sizeof(line), "%s%02d. %s",
                             is_playing ? "> " : "  ", idx + 1, pb->opened_tracks[idx].title);
                    font_draw_utf8_ellipsis(lx + 8.0f, ry + 4.0f, lw - 22.0f, line, 1.0f, fg);
                }
                draw_scrollbar(lx + lw - 7.0f, row_y0, (float)visible_rows * row_h,
                               visible_rows, pb->opened_count, pb->track_scroll_top, pal);
            }
        }
    }

    gfx_scissor_reset();
}

/* ---------------- Popup Modals: Add to Playlist / Create Playlist ---------------- */

static void draw_playlist_popup(const AppState *app)
{
    if (app->modal != MODAL_ADD_TO_PLAYLIST && app->modal != MODAL_NEW_PLAYLIST_NAME) {
        return;
    }

    const DeviceTheme *th = get_theme(app);
    const LcdPalette *pal = get_lcd_palette(app);

    gfx_fill_rect(0, 24, SCREEN_W, 210, RGBA8(0, 0, 0, 120));

    float wx = 110.0f, wy = 44.0f, ww = 260.0f, wh = 168.0f;
    gfx_bevel_rect(wx, wy, ww, wh,
                   th->module_frame_top, th->module_frame_bot,
                   th->bevel_hi, th->bevel_lo);

    float lx = wx + 5.0f, ly = wy + 5.0f, lw = ww - 10.0f, lh = wh - 10.0f;
    gfx_grad_rect_v(lx, ly, lw, lh, pal->bg_top, pal->bg_bot);
    gfx_rect_outline(lx, ly, lw, lh, 1.0f, pal->ink_mid);

    gfx_fill_rect(lx + 2.0f, ly + 2.0f, lw - 4.0f, 16.0f, pal->badge_bg);

    if (app->modal == MODAL_ADD_TO_PLAYLIST) {
        font_draw_utf8(lx + 8.0f, ly + 6.0f, "ADD TO PLAYLIST", 1.0f, pal->badge_fg);
        font_draw_utf8_ellipsis(lx + 8.0f, ly + 22.0f, lw - 16.0f,
                                app->pending_add_title, 1.0f, pal->ink_bright);
        gfx_fill_rect(lx + 6.0f, ly + 33.0f, lw - 12.0f, 1.0f, pal->ink_dim);

        int total_opts = 1 + app->playlists.count; /* 0 = "+ Create New Playlist..." */
        int vis = 7;
        float ry0 = ly + 37.0f;
        for (int r = 0; r < vis; r++) {
            int idx = app->popup_scroll + r;
            if (idx >= total_opts) break;
            float ry = ry0 + (float)r * 16.5f;
            bool sel = (idx == app->popup_selected);
            if (sel) {
                gfx_fill_rect(lx + 6.0f, ry, lw - 12.0f, 15.5f, pal->badge_bg);
            }
            uint32_t fg = sel ? pal->badge_fg : pal->ink_bright;
            char row_txt[96];
            if (idx == 0) {
                snprintf(row_txt, sizeof(row_txt), "+ Create New Playlist...");
            } else {
                const PlaylistMeta *m = &app->playlists.list[idx - 1];
                snprintf(row_txt, sizeof(row_txt), "%s (%d songs)", m->name, m->track_count);
            }
            font_draw_utf8_ellipsis(lx + 10.0f, ry + 4.0f, lw - 20.0f, row_txt, 1.0f, fg);
        }
    } else {
        /* MODAL_NEW_PLAYLIST_NAME */
        font_draw_utf8(lx + 8.0f, ly + 6.0f,
                       app->save_queue_as_playlist ? "SAVE QUEUE AS PLAYLIST" : "CREATE NEW PLAYLIST",
                       1.0f, pal->badge_fg);
        font_draw_utf8(lx + 8.0f, ly + 22.0f, "Choose a playlist title:", 1.0f, pal->ink_mid);
        gfx_fill_rect(lx + 6.0f, ly + 33.0f, lw - 12.0f, 1.0f, pal->ink_dim);

        int total_presets = playlist_preset_name_count();
        int vis = 7;
        float ry0 = ly + 37.0f;
        for (int r = 0; r < vis; r++) {
            int idx = app->popup_scroll + r;
            if (idx >= total_presets) break;
            float ry = ry0 + (float)r * 16.5f;
            bool sel = (idx == app->preset_name_idx);
            if (sel) {
                gfx_fill_rect(lx + 6.0f, ry, lw - 12.0f, 15.5f, pal->badge_bg);
            }
            uint32_t fg = sel ? pal->badge_fg : pal->ink_bright;
            font_draw_utf8(lx + 12.0f, ry + 4.0f, playlist_preset_name(idx), 1.0f, fg);
        }
    }
}

/* ---------------- SELECT: Full-Device Theme Studio Screen ---------------- */

static void draw_theme_studio_modal(const AppState *app)
{
    if (app->theme_anim <= 0.01f) return;

    const DeviceTheme *cur_th = get_theme(app);
    const LcdPalette *cur_pal = get_lcd_palette(app);

    /* Semi-transparent dim over right side while keeping Left Optical Bay visible for live preview! */
    float target_x = 160.0f;
    float start_x  = 484.0f;
    float wx = start_x + (target_x - start_x) * app->theme_anim;
    float wy = 25.0f, ww = 312.0f, wh = 207.0f;

    gfx_fill_rect(wx - 6.0f, wy + 3.0f, 6.0f, wh, RGBA8(0, 0, 0, 120));
    gfx_bevel_rect(wx, wy, ww, wh,
                   cur_th->module_frame_top, cur_th->module_frame_bot,
                   cur_th->bevel_hi, cur_th->bevel_lo);

    float lx = wx + 5.0f, ly = wy + 5.0f, lw = ww - 10.0f, lh = wh - 10.0f;
    gfx_grad_rect_v(lx, ly, lw, lh, cur_pal->bg_top, cur_pal->bg_bot);
    gfx_rect_outline(lx, ly, lw, lh, 1.0f, RGBA8(0, 0, 0, 160));

    gfx_scissor((int)lx, (int)ly, (int)lw, (int)lh);

    /* Header */
    gfx_fill_rect(lx + 2.0f, ly + 2.0f, lw - 4.0f, 16.0f, cur_pal->badge_bg);
    font_draw_utf8(lx + 8.0f, ly + 6.0f, "E-RENCYK THEME STUDIO", 1.0f, cur_pal->badge_fg);

    /* 6 Full Device Hardware Theme Rows with Live Color Swatches + Emblem Icon! */
    float ry0 = ly + 22.0f;
    float row_h = 22.5f;

    for (int i = 0; i < DEVICE_THEME_COUNT; i++) {
        const DeviceTheme *t = &s_device_themes[i];
        const LcdPalette *tp = &s_lcd_palettes[t->default_lcd_idx % 5];
        float ry = ry0 + (float)i * row_h;
        bool sel = ((int)app->theme_id == i);

        if (sel) {
            gfx_fill_rect(lx + 4.0f, ry, lw - 8.0f, row_h - 2.0f, cur_pal->badge_bg);
        } else {
            gfx_fill_rect(lx + 4.0f, ry, lw - 8.0f, row_h - 2.0f, RGBA8(0, 0, 0, 34));
            gfx_rect_outline(lx + 4.0f, ry, lw - 8.0f, row_h - 2.0f, 1.0f, cur_pal->ink_dim);
        }

        /* Theme Emblem Icon */
        draw_theme_emblem_icon(lx + 9.0f, ry + 4.0f, t->icon_type,
                               sel ? cur_pal->badge_fg : cur_pal->ink_bright,
                               t->accent_led);

        /* Theme Title */
        uint32_t fg = sel ? cur_pal->badge_fg : cur_pal->ink_bright;
        font_draw_utf8(lx + 30.0f, ry + 6.5f, t->name, 1.0f, fg);

        /* 4 Hardware Color Swatch Chips (Chassis, Chamber, LCD, VU Face) */
        float sx = lx + lw - 74.0f;
        float sy = ry + 4.5f;
        uint32_t chips[4] = { t->body_tl, t->chamber_top, tp->badge_bg, t->vu_face_tl };
        for (int c = 0; c < 4; c++) {
            gfx_fill_rect(sx + (float)(c * 16), sy, 13.0f, 11.0f, chips[c]);
            gfx_rect_outline(sx + (float)(c * 16), sy, 13.0f, 11.0f, 1.0f,
                             sel ? cur_pal->badge_fg : RGB8(220, 228, 238));
        }
    }

    /* Bottom LCD Backlight Tint Selector (< Left / Right >) */
    float by = ly + lh - 36.0f;
    gfx_fill_rect(lx + 6.0f, by, lw - 12.0f, 1.0f, cur_pal->ink_dim);
    font_draw_utf8(lx + 8.0f, by + 6.0f, "LCD Backlight:", 1.0f, cur_pal->ink_mid);

    char ov_buf[64];
    snprintf(ov_buf, sizeof(ov_buf), "< %s >", ui_lcd_override_name(app->lcd_override));
    float ow = font_measure_utf8(ov_buf, 1.0f) + 12.0f;
    gfx_fill_rect(lx + lw - ow - 8.0f, by + 4.0f, ow, 13.0f, cur_pal->badge_bg);
    font_draw_utf8(lx + lw - ow - 2.0f, by + 6.5f, ov_buf, 1.0f, cur_pal->badge_fg);

    font_draw_utf8_ellipsis(lx + 8.0f, by + 21.0f, lw - 16.0f,
                            cur_th->subtitle, 1.0f, cur_pal->ink_bright);

    gfx_scissor_reset();
}

/* ---------------- Master Render ---------------- */

void ui_render(const AppState *app, PlayState play_state,
               const TrackInfo *track, const AudioVizSnapshot *viz)
{
    const DeviceTheme *th = get_theme(app);
    gfx_begin_frame(th->body_bl);

    draw_metallic_chassis(app, play_state);
    draw_optical_cover_bay(app, play_state, track);
    draw_backlit_lcd((AppState *)app, app, play_state, track);
    draw_meter_bay(app, play_state, viz);

    draw_library_drawer(app);
    draw_theme_studio_modal(app);
    draw_playlist_popup(app);

    draw_control_guide_bar(app, play_state);

    gfx_end_frame();
}
