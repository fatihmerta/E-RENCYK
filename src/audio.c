#include "audio.h"
#include "cover.h"

#include <pspaudio.h>
#include <pspkernel.h>
#include <mpg123.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 2048 stereo samples (~46ms at 44.1kHz): rock-solid DMA margin, multiple of 64 */
#define AUDIO_SAMPLES_PER_BUF 2048
#define FFT_N                 256

static SceUID s_audio_sema  = -1;
static SceUID s_audio_thid  = -1;
static volatile bool s_thread_alive = false;

/* PSP Hardware Audio Output State (owned exclusively by audio thread) */
static bool s_using_src     = false;
static int  s_active_rate   = 0;
static int  s_fallback_ch   = -1;

static mpg123_handle *s_mh  = NULL;
static long s_stream_rate   = 44100;
static bool s_is_float_dec  = true;

static volatile PlayState s_state = PLAY_STATE_STOPPED;
static volatile bool s_track_ended = false;
static volatile int  s_volume_pct  = 85;
static EqPreset      s_cur_eq      = EQ_FLAT;

static TrackInfo        s_track_info;
static AudioVizSnapshot s_viz;

/* 64-byte aligned float decode buffer + double-buffered int16 DMA output buffers */
static float   __attribute__((aligned(64))) s_flt_buf[AUDIO_SAMPLES_PER_BUF * 2];
static int16_t __attribute__((aligned(64))) s_pcm_buf[2][AUDIO_SAMPLES_PER_BUF * 2];

static float   s_hann[FFT_N];
static uint8_t s_bitrev[FFT_N];

static void lock_audio(void)
{
    if (s_audio_sema >= 0) {
        sceKernelWaitSema(s_audio_sema, 1, NULL);
    }
}

static void unlock_audio(void)
{
    if (s_audio_sema >= 0) {
        sceKernelSignalSema(s_audio_sema, 1);
    }
}

static void init_fft_tables(void)
{
    for (int i = 0; i < FFT_N; i++) {
        s_hann[i] = 0.5f * (1.0f - cosf((2.0f * (float)M_PI * (float)i) / (float)(FFT_N - 1)));
        uint8_t x = (uint8_t)i;
        uint8_t r = 0;
        for (int b = 0; b < 8; b++) {
            r = (uint8_t)((r << 1) | (x & 1));
            x >>= 1;
        }
        s_bitrev[i] = r;
    }
}

/* Transparent audiophile soft-knee limiter:
 * Bit-accurate linear pass-through up to |x| <= 0.82f, then smooth rational
 * compression approaching 0.998f so bass peaks NEVER hard-clip ("patlama")!
 */
static inline int16_t float_to_s16_soft_limit(float x)
{
    float ax = fabsf(x);
    if (ax > 0.82f) {
        float excess = ax - 0.82f;
        float compressed = 0.82f + (0.178f * excess) / (0.178f + excess);
        x = (x >= 0.0f) ? compressed : -compressed;
    }
    int v = (int)(x * 32767.0f);
    if (v >  32767) v =  32767;
    if (v < -32768) v = -32768;
    return (int16_t)v;
}

static void release_hw_audio(void)
{
    if (s_using_src) {
        sceAudioSRCChRelease();
        s_using_src = false;
        s_active_rate = 0;
    }
    if (s_fallback_ch >= 0) {
        sceAudioChRelease(s_fallback_ch);
        s_fallback_ch = -1;
        s_active_rate = 0;
    }
}

static bool ensure_hw_audio_rate(int desired_rate)
{
    if (desired_rate <= 0) desired_rate = 44100;
    if ((s_using_src || s_fallback_ch >= 0) && s_active_rate == desired_rate) {
        return true;
    }

    release_hw_audio();

    /* Prefer PSP's dedicated hardware Sample Rate Converter (supports 8k..48k natively!) */
    int rc = sceAudioSRCChReserve(AUDIO_SAMPLES_PER_BUF, desired_rate, 2);
    if (rc >= 0) {
        s_using_src = true;
        s_active_rate = desired_rate;
        return true;
    }

    /* Fallback to standard 44.1kHz stereo hardware channel */
    int ch = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,
                               AUDIO_SAMPLES_PER_BUF,
                               PSP_AUDIO_FORMAT_STEREO);
    if (ch >= 0) {
        s_fallback_ch = ch;
        s_active_rate = desired_rate;
        return true;
    }

    return false;
}

static void analyze_pcm_block(const int16_t *pcm, int frames)
{
    double sum_l = 0.0, sum_r = 0.0;
    int max_l = 0, max_r = 0;

    for (int i = 0; i < frames; i++) {
        int sl = pcm[i * 2 + 0];
        int sr = pcm[i * 2 + 1];
        int al = (sl < 0) ? -sl : sl;
        int ar = (sr < 0) ? -sr : sr;
        if (al > max_l) max_l = al;
        if (ar > max_r) max_r = ar;
        sum_l += (double)sl * (double)sl;
        sum_r += (double)sr * (double)sr;
    }

    float rms_l = (float)(sqrt(sum_l / (double)frames) / 32768.0);
    float rms_r = (float)(sqrt(sum_r / (double)frames) / 32768.0);

    float drive_l = powf(rms_l * 2.45f, 0.68f);
    float drive_r = powf(rms_r * 2.45f, 0.68f);
    if (drive_l > 1.15f) drive_l = 1.15f;
    if (drive_r > 1.15f) drive_r = 1.15f;

    int16_t wl[128], wr[128];
    int step = frames / 128;
    if (step < 1) step = 1;
    for (int i = 0; i < 128; i++) {
        int idx = i * step;
        if (idx >= frames) idx = frames - 1;
        wl[i] = pcm[idx * 2 + 0];
        wr[i] = pcm[idx * 2 + 1];
    }

    float rex[FFT_N], imx[FFT_N];
    for (int i = 0; i < FFT_N; i++) {
        int src = i * 2;
        if (src >= frames) src = frames - 1;
        float mono = ((float)pcm[src * 2] + (float)pcm[src * 2 + 1]) * (0.5f / 32768.0f);
        int br = s_bitrev[i];
        rex[br] = mono * s_hann[i];
        imx[br] = 0.0f;
    }

    for (int len = 2; len <= FFT_N; len <<= 1) {
        int half = len >> 1;
        float ang = -2.0f * (float)M_PI / (float)len;
        float wlen_r = cosf(ang);
        float wlen_i = sinf(ang);
        for (int i = 0; i < FFT_N; i += len) {
            float wr_c = 1.0f, wi_c = 0.0f;
            for (int j = 0; j < half; j++) {
                int u = i + j;
                int v = u + half;
                float tr = rex[v] * wr_c - imx[v] * wi_c;
                float ti = rex[v] * wi_c + imx[v] * wr_c;
                rex[v] = rex[u] - tr;
                imx[v] = imx[u] - ti;
                rex[u] += tr;
                imx[u] += ti;
                float next_wr = wr_c * wlen_r - wi_c * wlen_i;
                wi_c = wr_c * wlen_i + wi_c * wlen_r;
                wr_c = next_wr;
            }
        }
    }

    static const int bin_edges[17] = {
        1, 2, 3, 4, 6, 8, 11, 15, 20, 27, 36, 48, 63, 80, 98, 114, 127
    };
    float bands[16];
    for (int b = 0; b < 16; b++) {
        int b0 = bin_edges[b];
        int b1 = bin_edges[b + 1];
        if (b1 <= b0) b1 = b0 + 1;
        float acc = 0.0f;
        for (int k = b0; k < b1; k++) {
            float mag = sqrtf(rex[k] * rex[k] + imx[k] * imx[k]) / (float)(FFT_N / 2);
            if (mag > acc) acc = mag;
        }
        float tilt = 1.0f + (float)b * 0.24f;
        float val = powf(acc * 6.8f * tilt, 0.58f);
        if (val > 1.0f) val = 1.0f;
        bands[b] = val;
    }

    s_viz.vu_left    = drive_l;
    s_viz.vu_right   = drive_r;
    s_viz.peak_left  = (float)max_l / 32768.0f;
    s_viz.peak_right = (float)max_r / 32768.0f;
    memcpy(s_viz.bands,  bands, sizeof(bands));
    memcpy(s_viz.wave_l, wl,    sizeof(wl));
    memcpy(s_viz.wave_r, wr,    sizeof(wr));
}

static void clear_viz(void)
{
    memset(&s_viz, 0, sizeof(s_viz));
}

/* Headroom-compensated audiophile EQ curves:
 * Preamp strictly matches max subband gain so loudest mastered MP3s NEVER clip!
 */
static void apply_eq_locked(EqPreset preset)
{
    if (!s_mh) return;
    mpg123_reset_eq(s_mh);

    double preamp = 0.96;
    switch (preset) {
        case EQ_FLAT:
        default:
            /* Pure uncolored reference playback with 0.35 dB inter-sample peak headroom */
            preamp = 0.96;
            break;

        case EQ_MEGA_BASS_1:
            /* Warm Sony Walkman DBB Step 1 (+4.0 dB low shelf, -1.5 dB mud cut) */
            mpg123_eq_change(s_mh, MPG123_LR, 0,  1, +4.0);
            mpg123_eq_change(s_mh, MPG123_LR, 2,  3, +2.2);
            mpg123_eq_change(s_mh, MPG123_LR, 6, 13, -1.2);
            mpg123_eq_change(s_mh, MPG123_LR, 22, 31, +1.5);
            preamp = 0.64;
            break;

        case EQ_MEGA_BASS_2:
            /* Deep Sony Groove DBB Step 2 (+6.5 dB sub-bass, clean mids, crisp air) */
            mpg123_eq_change(s_mh, MPG123_LR, 0,  1, +6.5);
            mpg123_eq_change(s_mh, MPG123_LR, 2,  3, +3.8);
            mpg123_eq_change(s_mh, MPG123_LR, 6, 15, -2.0);
            mpg123_eq_change(s_mh, MPG123_LR, 22, 31, +2.2);
            preamp = 0.50;
            break;

        case EQ_ROCK_STAGE:
            mpg123_eq_change(s_mh, MPG123_LR, 0,  2, +3.8);
            mpg123_eq_change(s_mh, MPG123_LR, 3,  5, +1.8);
            mpg123_eq_change(s_mh, MPG123_LR, 8, 14, -1.8);
            mpg123_eq_change(s_mh, MPG123_LR, 17, 31, +3.2);
            preamp = 0.65;
            break;

        case EQ_WARM_JAZZ:
            mpg123_eq_change(s_mh, MPG123_LR, 0,  2, +2.5);
            mpg123_eq_change(s_mh, MPG123_LR, 3,  9, +1.8);
            mpg123_eq_change(s_mh, MPG123_LR, 12, 18, +0.8);
            mpg123_eq_change(s_mh, MPG123_LR, 24, 31, +1.6);
            preamp = 0.74;
            break;

        case EQ_Y2K_CLUB:
            mpg123_eq_change(s_mh, MPG123_LR, 0,  2, +5.2);
            mpg123_eq_change(s_mh, MPG123_LR, 3,  4, +2.4);
            mpg123_eq_change(s_mh, MPG123_LR, 8, 16, -2.2);
            mpg123_eq_change(s_mh, MPG123_LR, 20, 31, +3.8);
            preamp = 0.56;
            break;
    }

    mpg123_volume(s_mh, preamp);
}

const char *audio_eq_name(EqPreset preset)
{
    switch (preset) {
        case EQ_FLAT:        return "FLAT STUDIO";
        case EQ_MEGA_BASS_1: return "MEGA BASS 1";
        case EQ_MEGA_BASS_2: return "MEGA BASS 2";
        case EQ_ROCK_STAGE:  return "ROCK STAGE";
        case EQ_WARM_JAZZ:   return "WARM JAZZ";
        case EQ_Y2K_CLUB:    return "Y2K CLUB";
        default:             return "FLAT STUDIO";
    }
}

static void trim_right(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (unsigned char)s[len - 1] <= ' ') {
        s[--len] = '\0';
    }
}

static void copy_mpg123_str(char *dst, size_t dst_sz, const mpg123_string *ms)
{
    if (!ms || !ms->p || ms->fill == 0) return;
    const char *p = ms->p;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (!*p) return;
    snprintf(dst, dst_sz, "%s", p);
    trim_right(dst);
}

static void copy_id3v1_field(char *dst, size_t dst_sz, const char *src, size_t max_src)
{
    if (dst[0] != '\0') return;
    char tmp[64];
    if (max_src >= sizeof(tmp)) max_src = sizeof(tmp) - 1;
    memcpy(tmp, src, max_src);
    tmp[max_src] = '\0';
    trim_right(tmp);
    if (tmp[0] != '\0') {
        snprintf(dst, dst_sz, "%s", tmp);
    }
}

static void configure_native_formats(mpg123_handle *mh, int enc_flag)
{
    static const long psp_rates[] = {
        8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000
    };
    mpg123_format_none(mh);
    for (size_t i = 0; i < sizeof(psp_rates) / sizeof(psp_rates[0]); i++) {
        mpg123_format(mh, psp_rates[i], MPG123_STEREO, enc_flag);
    }
}

static int audio_thread_entry(SceSize args, void *argp)
{
    (void)args;
    (void)argp;

    int cur_buf = 0;

    while (s_thread_alive) {
        if (s_state != PLAY_STATE_PLAYING) {
            clear_viz();
            sceKernelDelayThread(10000);
            continue;
        }

        int16_t *out_s16 = s_pcm_buf[cur_buf];
        size_t frames_got = 0;
        bool reached_eof = false;
        int target_hw_rate = 44100;

        lock_audio();
        if (s_mh && s_state == PLAY_STATE_PLAYING) {
            target_hw_rate = (int)s_stream_rate;

            if (s_is_float_dec) {
                const size_t target_bytes = AUDIO_SAMPLES_PER_BUF * 2 * sizeof(float);
                size_t bytes_got = 0;
                while (bytes_got < target_bytes) {
                    size_t got = 0;
                    int ret = mpg123_read(s_mh,
                                          (uint8_t *)s_flt_buf + bytes_got,
                                          target_bytes - bytes_got,
                                          &got);
                    bytes_got += got;

                    if (ret == MPG123_NEW_FORMAT) {
                        long rate = 0;
                        int ch = 0, enc = 0;
                        if (mpg123_getformat(s_mh, &rate, &ch, &enc) == MPG123_OK && rate > 0) {
                            s_stream_rate = rate;
                            target_hw_rate = (int)rate;
                        }
                        continue;
                    }
                    if (ret == MPG123_DONE || (ret != MPG123_OK && got == 0)) {
                        reached_eof = true;
                        break;
                    }
                }
                frames_got = bytes_got / (2 * sizeof(float));
                for (size_t i = 0; i < frames_got * 2; i++) {
                    out_s16[i] = float_to_s16_soft_limit(s_flt_buf[i]);
                }
            } else {
                const size_t target_bytes = AUDIO_SAMPLES_PER_BUF * 2 * sizeof(int16_t);
                size_t bytes_got = 0;
                while (bytes_got < target_bytes) {
                    size_t got = 0;
                    int ret = mpg123_read(s_mh,
                                          (uint8_t *)out_s16 + bytes_got,
                                          target_bytes - bytes_got,
                                          &got);
                    bytes_got += got;

                    if (ret == MPG123_NEW_FORMAT) {
                        long rate = 0;
                        int ch = 0, enc = 0;
                        if (mpg123_getformat(s_mh, &rate, &ch, &enc) == MPG123_OK && rate > 0) {
                            s_stream_rate = rate;
                            target_hw_rate = (int)rate;
                        }
                        continue;
                    }
                    if (ret == MPG123_DONE || (ret != MPG123_OK && got == 0)) {
                        reached_eof = true;
                        break;
                    }
                }
                frames_got = bytes_got / (2 * sizeof(int16_t));
            }

            int64_t sample_pos = mpg123_tell64(s_mh);
            if (sample_pos >= 0 && s_stream_rate > 0) {
                s_track_info.position_sec = (double)sample_pos / (double)s_stream_rate;
            }
            struct mpg123_frameinfo2 fi;
            if (mpg123_info2(s_mh, &fi) == MPG123_OK && fi.bitrate > 0) {
                s_track_info.bitrate_kbps = fi.bitrate;
            }

            if (reached_eof && frames_got == 0) {
                s_state = PLAY_STATE_STOPPED;
                s_track_ended = true;
            }
        }
        unlock_audio();

        if (frames_got > 0) {
            if (frames_got < AUDIO_SAMPLES_PER_BUF) {
                memset(&out_s16[frames_got * 2], 0,
                       (AUDIO_SAMPLES_PER_BUF - frames_got) * 2 * sizeof(int16_t));
            }

            analyze_pcm_block(out_s16, AUDIO_SAMPLES_PER_BUF);

            /* Flush CPU write-back D-cache before PSP Audio DMA reads physical RAM! */
            sceKernelDcacheWritebackRange(out_s16, AUDIO_SAMPLES_PER_BUF * 2 * sizeof(int16_t));

            int vol_pct = s_volume_pct;
            if (vol_pct < 0) vol_pct = 0;
            if (vol_pct > 100) vol_pct = 100;
            /* Smooth perceptual curve, strictly capped below 0x7E00 to avoid hardware DAC clip */
            float vn = (float)vol_pct / 100.0f;
            int hw_vol = (int)(0x7E00 * (0.35f * vn + 0.65f * vn * vn));

            if (ensure_hw_audio_rate(target_hw_rate)) {
                if (s_using_src) {
                    sceAudioSRCOutputBlocking(hw_vol, out_s16);
                } else if (s_fallback_ch >= 0) {
                    sceAudioOutputPannedBlocking(s_fallback_ch, hw_vol, hw_vol, out_s16);
                }
            } else {
                sceKernelDelayThread(15000);
            }

            cur_buf ^= 1;

            if (reached_eof) {
                lock_audio();
                if (s_state == PLAY_STATE_PLAYING) {
                    s_state = PLAY_STATE_STOPPED;
                    s_track_ended = true;
                }
                unlock_audio();
            }
        } else {
            sceKernelDelayThread(8000);
        }
    }

    release_hw_audio();
    sceKernelExitDeleteThread(0);
    return 0;
}

bool audio_init(void)
{
    init_fft_tables();
    memset(&s_track_info, 0, sizeof(s_track_info));
    clear_viz();

    if (mpg123_init() != MPG123_OK) {
        return false;
    }

    s_audio_sema = sceKernelCreateSema("erencyk_audio_sema", 0, 1, 1, NULL);
    if (s_audio_sema < 0) {
        return false;
    }

    s_thread_alive = true;
    s_audio_thid = sceKernelCreateThread("erencyk_audio_thr",
                                         audio_thread_entry,
                                         0x12,
                                         65536,
                                         PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU,
                                         NULL);
    if (s_audio_thid < 0) {
        s_thread_alive = false;
        return false;
    }

    sceKernelStartThread(s_audio_thid, 0, NULL);
    return true;
}

void audio_shutdown(void)
{
    s_thread_alive = false;
    s_state = PLAY_STATE_STOPPED;

    if (s_audio_thid >= 0) {
        SceUInt timeout = 500000;
        sceKernelWaitThreadEnd(s_audio_thid, &timeout);
        s_audio_thid = -1;
    }

    lock_audio();
    if (s_mh) {
        mpg123_close(s_mh);
        mpg123_delete(s_mh);
        s_mh = NULL;
    }
    unlock_audio();

    if (s_audio_sema >= 0) {
        sceKernelDeleteSema(s_audio_sema);
        s_audio_sema = -1;
    }

    mpg123_exit();
}

bool audio_load_track(const char *path, bool auto_play, DeviceThemeId theme_hint)
{
    (void)theme_hint;
    if (!path || !*path) return false;

    lock_audio();
    s_state = PLAY_STATE_STOPPED;
    s_track_ended = false;

    if (s_mh) {
        mpg123_close(s_mh);
        mpg123_delete(s_mh);
        s_mh = NULL;
    }

    int err = MPG123_OK;
    mpg123_handle *mh = mpg123_new(NULL, &err);
    if (!mh) {
        unlock_audio();
        return false;
    }

    /* High-precision decode at native MP3 sample rate (NO crude NtoM forced resampling!) */
    bool use_float = (mpg123_feature2(MPG123_FEATURE_OUTPUT_FLOAT32) == 1);
    long flags = MPG123_QUIET | MPG123_PICTURE | MPG123_FUZZY | MPG123_GAPLESS;
    if (use_float) flags |= MPG123_FORCE_FLOAT;
    mpg123_param2(mh, MPG123_ADD_FLAGS, flags, 0.0);

    configure_native_formats(mh, use_float ? MPG123_ENC_FLOAT_32 : MPG123_ENC_SIGNED_16);

    if (mpg123_open(mh, path) != MPG123_OK) {
        mpg123_delete(mh);
        unlock_audio();
        return false;
    }

    long out_rate = 0;
    int out_ch = 0, out_enc = 0;
    if (mpg123_getformat(mh, &out_rate, &out_ch, &out_enc) != MPG123_OK) {
        /* Fallback if non-standard sample rate encountered */
        mpg123_close(mh);
        mpg123_param2(mh, MPG123_FORCE_RATE, 44100, 0.0);
        configure_native_formats(mh, use_float ? MPG123_ENC_FLOAT_32 : MPG123_ENC_SIGNED_16);
        if (mpg123_open(mh, path) != MPG123_OK ||
            mpg123_getformat(mh, &out_rate, &out_ch, &out_enc) != MPG123_OK) {
            mpg123_delete(mh);
            unlock_audio();
            return false;
        }
    }

    s_stream_rate  = (out_rate > 0) ? out_rate : 44100;
    s_is_float_dec = (out_enc == MPG123_ENC_FLOAT_32);

    memset(&s_track_info, 0, sizeof(s_track_info));
    snprintf(s_track_info.path, sizeof(s_track_info.path), "%s", path);

    const char *slash = strrchr(path, '/');
    const char *fname = slash ? (slash + 1) : path;
    snprintf(s_track_info.filename, sizeof(s_track_info.filename), "%s", fname);

    struct mpg123_frameinfo2 fi;
    if (mpg123_info2(mh, &fi) == MPG123_OK) {
        s_track_info.sample_rate_hz = fi.rate > 0 ? fi.rate : s_stream_rate;
        s_track_info.bitrate_kbps   = fi.bitrate > 0 ? fi.bitrate : (fi.abr_rate > 0 ? fi.abr_rate : 192);
        s_track_info.is_vbr         = (fi.vbr != MPG123_CBR);
        s_track_info.is_stereo      = (fi.mode != MPG123_M_MONO);
    } else {
        s_track_info.sample_rate_hz = s_stream_rate;
        s_track_info.bitrate_kbps   = 192;
        s_track_info.is_stereo      = true;
    }

    bool cover_loaded = false;
    int  cover_source = 0;

    mpg123_id3v1 *v1 = NULL;
    mpg123_id3v2 *v2 = NULL;
    if (mpg123_id3(mh, &v1, &v2) == MPG123_OK) {
        if (v2) {
            copy_mpg123_str(s_track_info.title,  sizeof(s_track_info.title),  v2->title);
            copy_mpg123_str(s_track_info.artist, sizeof(s_track_info.artist), v2->artist);
            copy_mpg123_str(s_track_info.album,  sizeof(s_track_info.album),  v2->album);
            copy_mpg123_str(s_track_info.year,   sizeof(s_track_info.year),   v2->year);

            const mpg123_picture *best_pic = NULL;
            for (size_t i = 0; i < v2->pictures; i++) {
                if (v2->picture[i].data && v2->picture[i].size > 16) {
                    if (!best_pic || v2->picture[i].type == mpg123_id3_pic_front_cover) {
                        best_pic = &v2->picture[i];
                        if (v2->picture[i].type == mpg123_id3_pic_front_cover) break;
                    }
                }
            }
            if (best_pic && cover_load_from_memory(best_pic->data, best_pic->size)) {
                cover_loaded = true;
                cover_source = 1;
            }
        }
        if (v1) {
            copy_id3v1_field(s_track_info.title,  sizeof(s_track_info.title),  v1->title,  30);
            copy_id3v1_field(s_track_info.artist, sizeof(s_track_info.artist), v1->artist, 30);
            copy_id3v1_field(s_track_info.album,  sizeof(s_track_info.album),  v1->album,  30);
            copy_id3v1_field(s_track_info.year,   sizeof(s_track_info.year),   v1->year,    4);
        }
    }

    mpg123_meta_free(mh);

    if (s_track_info.title[0] == '\0') {
        snprintf(s_track_info.title, sizeof(s_track_info.title), "%s", fname);
        char *dot = strrchr(s_track_info.title, '.');
        if (dot) *dot = '\0';
    }
    if (s_track_info.artist[0] == '\0') {
        snprintf(s_track_info.artist, sizeof(s_track_info.artist), "Unknown Artist");
    }
    if (s_track_info.album[0] == '\0') {
        snprintf(s_track_info.album, sizeof(s_track_info.album), "MiniDisc Audio");
    }

    if (!cover_loaded && cover_load_from_folder(path)) {
        cover_loaded = true;
        cover_source = 2;
    }
    if (!cover_loaded) {
        cover_generate_minidisc(s_track_info.title);
        cover_source = 0;
    }

    s_track_info.has_cover_art = cover_loaded;
    s_track_info.cover_source  = cover_source;

    int64_t total_samples = mpg123_length64(mh);
    if (total_samples > 0 && s_stream_rate > 0) {
        s_track_info.duration_sec = (double)total_samples / (double)s_stream_rate;
    } else {
        s_track_info.duration_sec = 0.0;
    }
    s_track_info.position_sec = 0.0;

    s_mh = mh;
    apply_eq_locked(s_cur_eq);

    s_state = auto_play ? PLAY_STATE_PLAYING : PLAY_STATE_PAUSED;
    unlock_audio();
    return true;
}

void audio_toggle_pause(void)
{
    lock_audio();
    if (s_mh) {
        if (s_state == PLAY_STATE_PLAYING) {
            s_state = PLAY_STATE_PAUSED;
        } else {
            s_state = PLAY_STATE_PLAYING;
        }
    }
    unlock_audio();
}

void audio_set_paused(bool paused)
{
    lock_audio();
    if (s_mh) {
        s_state = paused ? PLAY_STATE_PAUSED : PLAY_STATE_PLAYING;
    }
    unlock_audio();
}

void audio_stop(void)
{
    lock_audio();
    s_state = PLAY_STATE_STOPPED;
    if (s_mh) {
        mpg123_seek64(s_mh, 0, SEEK_SET);
        s_track_info.position_sec = 0.0;
    }
    clear_viz();
    unlock_audio();
}

void audio_seek_relative(double delta_sec)
{
    lock_audio();
    if (s_mh && s_stream_rate > 0) {
        int64_t cur = mpg123_tell64(s_mh);
        if (cur < 0) cur = 0;
        int64_t delta_samples = (int64_t)(delta_sec * (double)s_stream_rate);
        int64_t target = cur + delta_samples;
        if (target < 0) target = 0;

        int64_t len = mpg123_length64(s_mh);
        if (len > 0 && target >= len - s_stream_rate / 2) {
            target = len - s_stream_rate / 2;
            if (target < 0) target = 0;
        }
        int64_t pos = mpg123_seek64(s_mh, target, SEEK_SET);
        if (pos >= 0) {
            s_track_info.position_sec = (double)pos / (double)s_stream_rate;
        }
    }
    unlock_audio();
}

void audio_set_volume(int volume_pct)
{
    if (volume_pct < 0) volume_pct = 0;
    if (volume_pct > 100) volume_pct = 100;
    s_volume_pct = volume_pct;
}

void audio_apply_eq(EqPreset preset)
{
    lock_audio();
    s_cur_eq = preset;
    apply_eq_locked(preset);
    unlock_audio();
}

PlayState audio_get_state(void)
{
    return s_state;
}

void audio_get_track_info(TrackInfo *out_info)
{
    if (!out_info) return;
    lock_audio();
    *out_info = s_track_info;
    unlock_audio();
}

void audio_get_viz_snapshot(AudioVizSnapshot *out_viz)
{
    if (!out_viz) return;
    *out_viz = s_viz;
}

bool audio_consume_track_ended(void)
{
    bool ended = false;
    lock_audio();
    if (s_track_ended) {
        ended = true;
        s_track_ended = false;
    }
    unlock_audio();
    return ended;
}
