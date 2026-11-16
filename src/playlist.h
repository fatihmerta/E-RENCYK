#ifndef ERENCYK_PLAYLIST_H
#define ERENCYK_PLAYLIST_H

#include "app.h"
#include <stdbool.h>

/* -------- Play Queue ("Up Next") -------- */
void queue_init(QueueState *q);
bool queue_add_track(QueueState *q, const char *title, const char *full_path);
void queue_remove_at(QueueState *q, int idx);
void queue_clear(QueueState *q);
int  queue_next_index(const QueueState *q, PlayMode mode, bool manual_skip);
int  queue_prev_index(const QueueState *q);

/* -------- Persistent .m3u Playlists -------- */
int  playlist_preset_name_count(void);
const char *playlist_preset_name(int idx);

void playlists_init(PlaylistBank *pb, const char *app_base_dir);
void playlists_scan(PlaylistBank *pb, const char *app_base_dir);

/* Create a new .m3u playlist (returns index in pb->list, or -1 on error) */
int  playlists_create(PlaylistBank *pb, const char *app_base_dir, const char *raw_name);

/* Append one or multiple tracks to an existing playlist */
bool playlists_append_one(PlaylistBank *pb, int pl_idx, const char *title, const char *full_path);
int  playlists_append_many(PlaylistBank *pb, int pl_idx, const TrackRef *refs, int count);

/* Open playlist into pb->opened_tracks for browsing */
bool playlists_open(PlaylistBank *pb, int pl_idx);

/* Copy playlist into pb->active_tracks for playback */
bool playlists_activate_for_play(PlaylistBank *pb, int pl_idx, int start_track_idx);

bool playlists_remove_opened_track(PlaylistBank *pb, int track_idx);
bool playlists_delete(PlaylistBank *pb, const char *app_base_dir, int pl_idx);

int  playlists_next_active_index(const PlaylistBank *pb, PlayMode mode, bool manual_skip);
int  playlists_prev_active_index(const PlaylistBank *pb);

/* -------- Persistent User Settings (erencyk.cfg) -------- */
void config_load(AppState *app);
void config_save(const AppState *app);

#endif /* ERENCYK_PLAYLIST_H */
