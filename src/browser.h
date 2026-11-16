#ifndef ERENCYK_BROWSER_H
#define ERENCYK_BROWSER_H

#include "app.h"
#include <stdbool.h>

void browser_init(BrowserState *b, const char *app_base_dir);
bool browser_open_dir(BrowserState *b, const char *dir_path);
bool browser_go_parent(BrowserState *b);

/* Cycle between ms0:/MUSIC, ms0:/PSP/MUSIC, App ./music, and ms0:/ */
void browser_cycle_quick_root(BrowserState *b, const char *app_base_dir);

/* Collect sorted MP3 files directly inside dir_path (used when queuing/playlisting a folder) */
int browser_collect_dir_mp3s(const char *dir_path, TrackRef *out_refs, int max_refs);

const char *browser_get_track_path(const BrowserState *b, int track_pos);
int browser_next_track_pos(const BrowserState *b, PlayMode mode, bool manual_skip);
int browser_prev_track_pos(const BrowserState *b);

#endif /* ERENCYK_BROWSER_H */
