#ifndef ERENCYK_APP_H
#define ERENCYK_APP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* PSP Display Constants */
#define SCREEN_W            480
#define SCREEN_H            272
#define BUF_W               512

/* Album Cover GU Texture Size (power-of-two for sceGuTexImage) */
#define COVER_TEX_SIZE      128

/* Limits */
#define MAX_PATH_LEN        512
#define MAX_TITLE_LEN       160
#define MAX_ARTIST_LEN      128
#define MAX_ALBUM_LEN       128
#define MAX_BROWSER_ITEMS   512
#define MAX_QUEUE_TRACKS    256
#define MAX_PLAYLISTS       32
#define MAX_PLAYLIST_TRACKS 256
#define MAX_PLAYLIST_NAME   48

/* PSP GU_COLOR_8888 packs ABGR in little-endian uint32_t */
#define RGBA8(r, g, b, a) \
    (((uint32_t)((a) & 0xFF) << 24) | \
     ((uint32_t)((b) & 0xFF) << 16) | \
     ((uint32_t)((g) & 0xFF) << 8)  | \
     ((uint32_t)((r) & 0xFF)))

#define RGB8(r, g, b) RGBA8((r), (g), (b), 255)

/* Playback States */
typedef enum {
    PLAY_STATE_STOPPED = 0,
    PLAY_STATE_PLAYING,
    PLAY_STATE_PAUSED
} PlayState;

/* Repeat / Play Order Modes */
typedef enum {
    PLAY_MODE_REPEAT_ALL = 0,
    PLAY_MODE_REPEAT_ONE,
    PLAY_MODE_SHUFFLE,
    PLAY_MODE_NORMAL,
    PLAY_MODE_COUNT
} PlayMode;

/* Active Playback List Source */
typedef enum {
    PLAY_SRC_FOLDER = 0,
    PLAY_SRC_QUEUE,
    PLAY_SRC_PLAYLIST
} PlaySource;

/* Sound DSP / Equalizer Presets (Headroom-compensated, zero-clipping) */
typedef enum {
    EQ_FLAT = 0,
    EQ_MEGA_BASS_1,
    EQ_MEGA_BASS_2,
    EQ_ROCK_STAGE,
    EQ_WARM_JAZZ,
    EQ_Y2K_CLUB,
    EQ_PRESET_COUNT
} EqPreset;

/* Meter / Visualizer Modes */
typedef enum {
    VIS_ANALOG_VU = 0,      /* Dual Warm-Backlit Ballistic Needle VU Meters */
    VIS_VFD_SPECTRUM,       /* 16-Band Y2K Fluorescent VFD Spectrum + Peak Hold */
    VIS_OSCILLOSCOPE,       /* Dual Stereo Waveform Oscilloscope */
    VIS_MODE_COUNT
} VisMode;

/* Full-Device Hardware Themes */
typedef enum {
    THEME_WALKMAN_SILVER = 0,   /* Brushed Silver Titanium + Ice Cyan EL + Ivory VU */
    THEME_CHAMPAGNE_GOLD,       /* Sony ES Champagne Gold + Warm Amber + Gold-Cream VU */
    THEME_MIDNIGHT_CYBER,       /* Matte Black Anodized + Matrix Phosphor + Dark Stealth VU */
    THEME_ATOMIC_COBALT,        /* Y2K Metallic Cobalt Blue + Electric Cyan + Cool Ice VU */
    THEME_CRIMSON_GT,           /* Dark Gunmetal Carbon + Alpine Ruby-Amber + Sport Black VU */
    THEME_FROST_MINT,           /* Metallic Icy Mint Y2K + Crisp Mint-White + Pearl VU */
    DEVICE_THEME_COUNT
} DeviceThemeId;

/* LCD Backlight Override Options */
typedef enum {
    LCD_OV_AUTO = 0,            /* Uses active Device Theme's signature LCD color */
    LCD_OV_ICE_CYAN,
    LCD_OV_PHOSPHOR_GREEN,
    LCD_OV_WARM_AMBER,
    LCD_OV_RUBY_ORANGE,
    LCD_OV_PURE_WHITE,
    LCD_OV_COUNT
} LcdOverrideId;

/* Theme Emblem Icon Types (top-right hardware badge replacing old G-SHOCK text) */
typedef enum {
    ICON_MINIDISC = 0,
    ICON_HIFI_SPEAKER,
    ICON_HEADPHONES,
    ICON_LASER_DISC,
    ICON_EQ_SLIDERS,
    ICON_MUSIC_NOTE
} ThemeIconType;

typedef struct {
    const char *name;
    uint32_t bg_top;
    uint32_t bg_bot;
    uint32_t ink_bright;
    uint32_t ink_mid;
    uint32_t ink_dim;
    uint32_t badge_bg;
    uint32_t badge_fg;
    uint32_t glow;
} LcdPalette;

typedef struct {
    const char *name;
    const char *subtitle;
    ThemeIconType icon_type;
    int default_lcd_idx;        /* 0..4 index into s_lcd_palettes */

    /* Chassis & Bezel Colors */
    uint32_t body_tl, body_tr, body_bl, body_br;
    uint32_t hdr_top, hdr_bot;
    uint32_t hdr_text_main, hdr_text_sub;
    uint32_t bevel_hi, bevel_lo;
    uint32_t module_frame_top, module_frame_bot;

    /* Optical Disc Chamber & Accent */
    uint32_t chamber_top, chamber_bot;
    uint32_t chamber_text;
    uint32_t accent_led;
    uint32_t fader_left, fader_right;

    /* Analog VU Gauge Colors */
    uint32_t vu_face_tl, vu_face_tr, vu_face_bl, vu_face_br;
    uint32_t vu_bulb_glow;
    uint32_t vu_scale_ink;
    uint32_t vu_red_ink;
    uint32_t vu_needle_shaft;
    uint32_t vu_needle_tip;

    /* VFD Spectrum Colors */
    uint32_t spec_low, spec_mid, spec_high;

    /* Bottom Control Strip */
    uint32_t bar_top, bar_bot;
    uint32_t bar_text;
    uint32_t bar_dim_text;
    uint32_t keycap_bg;
} DeviceTheme;

/* Track Metadata & Stream Info */
typedef struct {
    char path[MAX_PATH_LEN];
    char filename[MAX_TITLE_LEN];
    char title[MAX_TITLE_LEN];
    char artist[MAX_ARTIST_LEN];
    char album[MAX_ALBUM_LEN];
    char year[16];
    int  bitrate_kbps;
    long sample_rate_hz;
    bool is_vbr;
    bool is_stereo;
    bool has_cover_art;
    int  cover_source;      /* 0 = MiniDisc prism, 1 = ID3v2 APIC, 2 = Folder image */
    double duration_sec;
    double position_sec;
} TrackInfo;

/* Live Audio Telemetry */
typedef struct {
    float vu_left;
    float vu_right;
    float peak_left;
    float peak_right;
    float bands[16];
    int16_t wave_l[128];
    int16_t wave_r[128];
} AudioVizSnapshot;

/* File / Track Browser Entry */
typedef struct {
    char name[MAX_TITLE_LEN];
    char full_path[MAX_PATH_LEN];
    bool is_dir;
    bool is_parent;
} BrowserItem;

typedef struct {
    char current_dir[MAX_PATH_LEN];
    BrowserItem items[MAX_BROWSER_ITEMS];
    int count;
    int selected;
    int scroll_top;

    int track_item_indices[MAX_BROWSER_ITEMS];
    int track_count;
    int current_track_pos;
} BrowserState;

/* Queue / Playlist Track Entry */
typedef struct {
    char title[MAX_TITLE_LEN];
    char full_path[MAX_PATH_LEN];
} TrackRef;

/* Active Up-Next Queue */
typedef struct {
    TrackRef tracks[MAX_QUEUE_TRACKS];
    int count;
    int selected;
    int scroll_top;
    int playing_index;      /* Current index when PlaySource == PLAY_SRC_QUEUE */
} QueueState;

/* Saved Persistent Playlists (.m3u) */
typedef struct {
    char name[MAX_PLAYLIST_NAME];
    char file_path[MAX_PATH_LEN];
    int  track_count;
} PlaylistMeta;

typedef struct {
    PlaylistMeta list[MAX_PLAYLISTS];
    int count;
    int selected;
    int scroll_top;

    /* Currently opened playlist inside Playlists tab (inside_playlist == true) */
    bool inside_playlist;
    int  opened_index;
    TrackRef opened_tracks[MAX_PLAYLIST_TRACKS];
    int  opened_count;
    int  track_selected;
    int  track_scroll_top;

    /* Currently playing playlist tracks (independent copy so browsing other playlists doesn't interrupt playback) */
    char active_name[MAX_PLAYLIST_NAME];
    TrackRef active_tracks[MAX_PLAYLIST_TRACKS];
    int  active_count;
    int  active_playing_index;
} PlaylistBank;

/* Library Drawer Tabs */
typedef enum {
    LIB_TAB_FILES = 0,
    LIB_TAB_QUEUE,
    LIB_TAB_PLAYLISTS,
    LIB_TAB_COUNT
} LibraryTab;

/* Modal Overlays */
typedef enum {
    MODAL_NONE = 0,
    MODAL_LIBRARY,          /* Circle: 3-Tab Files / Queue / Playlists Drawer */
    MODAL_THEME_STUDIO,     /* Select: Full Device Theme + LCD Backlight Screen */
    MODAL_ADD_TO_PLAYLIST,  /* Popup: Choose target playlist or Create New */
    MODAL_NEW_PLAYLIST_NAME /* Popup: Pick preset playlist title / number */
} ActiveModal;

/* Global Application State */
typedef struct {
    bool running;
    ActiveModal modal;
    LibraryTab  lib_tab;
    float drawer_anim;      /* Smooth 0..1 animation for Library drawer */
    float theme_anim;       /* Smooth 0..1 animation for Theme Studio */

    PlaySource    play_source;
    PlayMode      play_mode;
    EqPreset      eq_preset;
    VisMode       vis_mode;
    DeviceThemeId theme_id;
    LcdOverrideId lcd_override;

    int  volume_pct;        /* 0 .. 100 */
    bool hold_locked;

    /* Add-to-Playlist / New-Playlist Popup State */
    char pending_add_path[MAX_PATH_LEN];
    char pending_add_title[MAX_TITLE_LEN];
    bool pending_add_is_dir;
    bool save_queue_as_playlist;
    int  popup_selected;
    int  popup_scroll;
    int  preset_name_idx;

    /* Smoothed UI Physics State (60 FPS) */
    float needle_l;
    float needle_r;
    float needle_vel_l;
    float needle_vel_r;
    int   clip_timer_l;
    int   clip_timer_r;

    float spec_bars[16];
    float spec_peaks[16];
    float spec_peak_vel[16];

    float disc_angle_deg;
    float disc_rpm_factor;

    float marquee_px;
    int   marquee_pause;

    /* Toast OSD inside LCD */
    char  osd_msg[80];
    int   osd_timer;

    uint32_t held_buttons;

    /* Hardware Status */
    int  battery_pct;
    bool battery_charging;
    int  clock_hour;
    int  clock_min;
    char app_base_dir[MAX_PATH_LEN];

    BrowserState browser;
    QueueState   queue;
    PlaylistBank playlists;
} AppState;

#endif /* ERENCYK_APP_H */
