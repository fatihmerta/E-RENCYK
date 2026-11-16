#ifndef ERENCYK_AUDIO_H
#define ERENCYK_AUDIO_H

#include "app.h"
#include <stdbool.h>

bool audio_init(void);
void audio_shutdown(void);

/* Load and optionally start playing an MP3 file */
bool audio_load_track(const char *path, bool auto_play, DeviceThemeId theme_hint);

void audio_toggle_pause(void);
void audio_set_paused(bool paused);
void audio_stop(void);

/* Relative seek in seconds */
void audio_seek_relative(double delta_sec);

/* Set digital volume 0..100 */
void audio_set_volume(int volume_pct);

/* Apply 32-band headroom-compensated subband EQ preset */
void audio_apply_eq(EqPreset preset);

PlayState audio_get_state(void);

/* Thread-safe snapshots for 60 FPS UI */
void audio_get_track_info(TrackInfo *out_info);
void audio_get_viz_snapshot(AudioVizSnapshot *out_viz);

/* Returns true once when current track finishes naturally */
bool audio_consume_track_ended(void);

const char *audio_eq_name(EqPreset preset);

#endif /* ERENCYK_AUDIO_H */
