#include "playlist.h"

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>

static const char *const s_preset_names[] = {
    "Favorites",
    "Night Drive",
    "Chill Mix",
    "Y2K Classics",
    "Workout",
    "Club & Synth",
    "Rock Anthems",
    "Late Night Jazz",
    "Study Beats",
    "Mixtape 01",
    "Mixtape 02",
    "Mixtape 03"
};

int playlist_preset_name_count(void)
{
    return (int)(sizeof(s_preset_names) / sizeof(s_preset_names[0]));
}

const char *playlist_preset_name(int idx)
{
    int n = playlist_preset_name_count();
    if (idx < 0 || idx >= n) return "My Mixtape";
    return s_preset_names[idx];
}

static void clean_display_title(char *dst, size_t dst_sz, const char *raw)
{
    if (!raw || !*raw) {
        snprintf(dst, dst_sz, "Untitled Track");
        return;
    }
    const char *slash = strrchr(raw, '/');
    const char *base = slash ? (slash + 1) : raw;
    snprintf(dst, dst_sz, "%.*s", (int)dst_sz - 1, base);
    char *dot = strrchr(dst, '.');
    if (dot && (strcasecmp(dot, ".mp3") == 0 || strcasecmp(dot, ".mp2") == 0 || strcasecmp(dot, ".mpa") == 0)) {
        *dot = '\0';
    }
}

/* ---------------- Queue Implementation ---------------- */

void queue_init(QueueState *q)
{
    memset(q, 0, sizeof(*q));
    q->playing_index = -1;
}

bool queue_add_track(QueueState *q, const char *title, const char *full_path)
{
    if (!q || !full_path || !*full_path || q->count >= MAX_QUEUE_TRACKS) {
        return false;
    }
    TrackRef *tr = &q->tracks[q->count++];
    if (title && *title) {
        clean_display_title(tr->title, sizeof(tr->title), title);
    } else {
        clean_display_title(tr->title, sizeof(tr->title), full_path);
    }
    snprintf(tr->full_path, sizeof(tr->full_path), "%.*s",
             (int)sizeof(tr->full_path) - 1, full_path);
    return true;
}

void queue_remove_at(QueueState *q, int idx)
{
    if (!q || idx < 0 || idx >= q->count) return;
    for (int i = idx; i + 1 < q->count; i++) {
        q->tracks[i] = q->tracks[i + 1];
    }
    q->count--;
    if (q->selected >= q->count) {
        q->selected = (q->count > 0) ? (q->count - 1) : 0;
    }
    if (q->playing_index == idx) {
        q->playing_index = -1;
    } else if (q->playing_index > idx) {
        q->playing_index--;
    }
}

void queue_clear(QueueState *q)
{
    if (!q) return;
    q->count = 0;
    q->selected = 0;
    q->scroll_top = 0;
    q->playing_index = -1;
}

int queue_next_index(const QueueState *q, PlayMode mode, bool manual_skip)
{
    if (!q || q->count <= 0) return -1;
    if (!manual_skip && mode == PLAY_MODE_REPEAT_ONE) {
        return (q->playing_index >= 0) ? q->playing_index : 0;
    }
    if (mode == PLAY_MODE_SHUFFLE && q->count > 1) {
        int next = rand() % q->count;
        if (next == q->playing_index) next = (next + 1) % q->count;
        return next;
    }
    int next = q->playing_index + 1;
    if (next >= q->count) {
        if (manual_skip || mode == PLAY_MODE_REPEAT_ALL) return 0;
        return -1;
    }
    return next;
}

int queue_prev_index(const QueueState *q)
{
    if (!q || q->count <= 0) return -1;
    int prev = q->playing_index - 1;
    if (prev < 0) prev = q->count - 1;
    return prev;
}

/* ---------------- Persistent .m3u Playlists ---------------- */

static void get_playlists_dir(const char *app_base_dir, char *out, size_t out_sz)
{
    if (app_base_dir && *app_base_dir) {
        snprintf(out, out_sz, "%.*s/playlists", (int)out_sz - 12, app_base_dir);
    } else {
        snprintf(out, out_sz, "ms0:/PSP/GAME/ERENCYK/playlists");
    }
}

static void trim_crlf(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' || s[len - 1] == ' ')) {
        s[--len] = '\0';
    }
}

static int parse_m3u_file(const char *file_path, TrackRef *out_tracks, int max_tracks)
{
    FILE *fp = fopen(file_path, "r");
    if (!fp) return 0;

    char line[MAX_PATH_LEN + 128];
    char pending_title[MAX_TITLE_LEN] = {0};
    int count = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_crlf(line);
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') continue;

        if (strncmp(p, "#EXTINF:", 8) == 0) {
            const char *comma = strchr(p, ',');
            if (comma && *(comma + 1)) {
                snprintf(pending_title, sizeof(pending_title), "%.*s",
                         (int)sizeof(pending_title) - 1, comma + 1);
            }
            continue;
        }
        if (*p == '#') continue;

        if (out_tracks && count < max_tracks) {
            if (pending_title[0]) {
                snprintf(out_tracks[count].title, sizeof(out_tracks[count].title),
                         "%.*s", (int)sizeof(out_tracks[count].title) - 1, pending_title);
            } else {
                clean_display_title(out_tracks[count].title, sizeof(out_tracks[count].title), p);
            }
            snprintf(out_tracks[count].full_path, sizeof(out_tracks[count].full_path),
                     "%.*s", (int)sizeof(out_tracks[count].full_path) - 1, p);
        }
        pending_title[0] = '\0';
        count++;
        if (out_tracks && count >= max_tracks) break;
    }

    fclose(fp);
    return count;
}

static bool write_m3u_file(const char *file_path, const TrackRef *tracks, int count)
{
    FILE *fp = fopen(file_path, "w");
    if (!fp) return false;
    fprintf(fp, "#EXTM3U\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "#EXTINF:-1,%s\n%s\n", tracks[i].title, tracks[i].full_path);
    }
    fclose(fp);
    return true;
}

static int cmp_playlist_meta(const void *a, const void *b)
{
    const PlaylistMeta *pa = (const PlaylistMeta *)a;
    const PlaylistMeta *pb = (const PlaylistMeta *)b;
    return strcasecmp(pa->name, pb->name);
}

void playlists_scan(PlaylistBank *pb, const char *app_base_dir)
{
    if (!pb) return;
    char pl_dir[MAX_PATH_LEN];
    get_playlists_dir(app_base_dir, pl_dir, sizeof(pl_dir));
    mkdir(pl_dir, 0777);

    pb->count = 0;
    DIR *d = opendir(pl_dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && pb->count < MAX_PLAYLISTS) {
        if (ent->d_name[0] == '.') continue;
        const char *dot = strrchr(ent->d_name, '.');
        if (!dot || (strcasecmp(dot, ".m3u") != 0 && strcasecmp(dot, ".m3u8") != 0)) {
            continue;
        }

        PlaylistMeta *m = &pb->list[pb->count++];
        size_t nlen = (size_t)(dot - ent->d_name);
        if (nlen >= sizeof(m->name)) nlen = sizeof(m->name) - 1;
        memcpy(m->name, ent->d_name, nlen);
        m->name[nlen] = '\0';

        snprintf(m->file_path, sizeof(m->file_path), "%.*s/%.*s",
                 MAX_PATH_LEN / 2, pl_dir, MAX_PATH_LEN / 2 - 4, ent->d_name);
        m->track_count = parse_m3u_file(m->file_path, NULL, MAX_PLAYLIST_TRACKS);
    }
    closedir(d);

    if (pb->count > 1) {
        qsort(pb->list, (size_t)pb->count, sizeof(PlaylistMeta), cmp_playlist_meta);
    }
    if (pb->selected >= pb->count) {
        pb->selected = (pb->count > 0) ? (pb->count - 1) : 0;
    }
}

void playlists_init(PlaylistBank *pb, const char *app_base_dir)
{
    memset(pb, 0, sizeof(*pb));
    pb->opened_index = -1;
    pb->active_playing_index = -1;
    playlists_scan(pb, app_base_dir);
}

int playlists_create(PlaylistBank *pb, const char *app_base_dir, const char *raw_name)
{
    if (!pb || !raw_name || !*raw_name) return -1;

    char pl_dir[MAX_PATH_LEN];
    get_playlists_dir(app_base_dir, pl_dir, sizeof(pl_dir));
    mkdir(pl_dir, 0777);

    char safe[MAX_PLAYLIST_NAME];
    size_t j = 0;
    for (size_t i = 0; raw_name[i] && j + 1 < sizeof(safe); i++) {
        unsigned char c = (unsigned char)raw_name[i];
        if (isalnum(c) || c == ' ' || c == '-' || c == '_' || c == '&') {
            safe[j++] = (char)c;
        }
    }
    safe[j] = '\0';
    if (j == 0) snprintf(safe, sizeof(safe), "Playlist");

    char final_name[MAX_PLAYLIST_NAME];
    char full_path[MAX_PATH_LEN];
    for (int suffix = 1; suffix <= 99; suffix++) {
        if (suffix == 1) {
            snprintf(final_name, sizeof(final_name), "%s", safe);
        } else {
            snprintf(final_name, sizeof(final_name), "%.*s %d",
                     (int)sizeof(final_name) - 5, safe, suffix);
        }
        snprintf(full_path, sizeof(full_path), "%.*s/%s.m3u",
                 MAX_PATH_LEN - 64, pl_dir, final_name);
        struct stat st;
        if (stat(full_path, &st) != 0) {
            break;
        }
    }

    if (!write_m3u_file(full_path, NULL, 0)) {
        return -1;
    }

    playlists_scan(pb, app_base_dir);
    for (int i = 0; i < pb->count; i++) {
        if (strcasecmp(pb->list[i].name, final_name) == 0) {
            return i;
        }
    }
    return (pb->count > 0) ? 0 : -1;
}

bool playlists_append_one(PlaylistBank *pb, int pl_idx, const char *title, const char *full_path)
{
    if (!pb || pl_idx < 0 || pl_idx >= pb->count || !full_path || !*full_path) return false;
    if (pb->list[pl_idx].track_count >= MAX_PLAYLIST_TRACKS) return false;

    FILE *fp = fopen(pb->list[pl_idx].file_path, "a");
    if (!fp) return false;

    char clean[MAX_TITLE_LEN];
    clean_display_title(clean, sizeof(clean), (title && *title) ? title : full_path);
    fprintf(fp, "#EXTINF:-1,%s\n%s\n", clean, full_path);
    fclose(fp);

    pb->list[pl_idx].track_count++;
    if (pb->inside_playlist && pb->opened_index == pl_idx) {
        playlists_open(pb, pl_idx);
    }
    return true;
}

int playlists_append_many(PlaylistBank *pb, int pl_idx, const TrackRef *refs, int count)
{
    if (!pb || pl_idx < 0 || pl_idx >= pb->count || !refs || count <= 0) return 0;
    int added = 0;
    for (int i = 0; i < count; i++) {
        if (playlists_append_one(pb, pl_idx, refs[i].title, refs[i].full_path)) {
            added++;
        }
    }
    return added;
}

bool playlists_open(PlaylistBank *pb, int pl_idx)
{
    if (!pb || pl_idx < 0 || pl_idx >= pb->count) return false;
    pb->opened_count = parse_m3u_file(pb->list[pl_idx].file_path,
                                      pb->opened_tracks,
                                      MAX_PLAYLIST_TRACKS);
    pb->list[pl_idx].track_count = pb->opened_count;
    pb->opened_index = pl_idx;
    pb->inside_playlist = true;
    if (pb->track_selected >= pb->opened_count) {
        pb->track_selected = (pb->opened_count > 0) ? (pb->opened_count - 1) : 0;
    }
    pb->track_scroll_top = 0;
    return true;
}

bool playlists_activate_for_play(PlaylistBank *pb, int pl_idx, int start_track_idx)
{
    if (!pb || pl_idx < 0 || pl_idx >= pb->count) return false;
    int cnt = parse_m3u_file(pb->list[pl_idx].file_path,
                             pb->active_tracks,
                             MAX_PLAYLIST_TRACKS);
    if (cnt <= 0) return false;
    snprintf(pb->active_name, sizeof(pb->active_name), "%s", pb->list[pl_idx].name);
    pb->active_count = cnt;
    if (start_track_idx < 0 || start_track_idx >= cnt) start_track_idx = 0;
    pb->active_playing_index = start_track_idx;
    return true;
}

bool playlists_remove_opened_track(PlaylistBank *pb, int track_idx)
{
    if (!pb || !pb->inside_playlist || pb->opened_index < 0 || pb->opened_index >= pb->count)
        return false;
    if (track_idx < 0 || track_idx >= pb->opened_count)
        return false;

    for (int i = track_idx; i + 1 < pb->opened_count; i++) {
        pb->opened_tracks[i] = pb->opened_tracks[i + 1];
    }
    pb->opened_count--;
    if (pb->track_selected >= pb->opened_count) {
        pb->track_selected = (pb->opened_count > 0) ? (pb->opened_count - 1) : 0;
    }

    write_m3u_file(pb->list[pb->opened_index].file_path, pb->opened_tracks, pb->opened_count);
    pb->list[pb->opened_index].track_count = pb->opened_count;
    return true;
}

bool playlists_delete(PlaylistBank *pb, const char *app_base_dir, int pl_idx)
{
    if (!pb || pl_idx < 0 || pl_idx >= pb->count) return false;
    unlink(pb->list[pl_idx].file_path);
    pb->inside_playlist = false;
    pb->opened_index = -1;
    playlists_scan(pb, app_base_dir);
    return true;
}

int playlists_next_active_index(const PlaylistBank *pb, PlayMode mode, bool manual_skip)
{
    if (!pb || pb->active_count <= 0) return -1;
    if (!manual_skip && mode == PLAY_MODE_REPEAT_ONE) {
        return (pb->active_playing_index >= 0) ? pb->active_playing_index : 0;
    }
    if (mode == PLAY_MODE_SHUFFLE && pb->active_count > 1) {
        int next = rand() % pb->active_count;
        if (next == pb->active_playing_index) next = (next + 1) % pb->active_count;
        return next;
    }
    int next = pb->active_playing_index + 1;
    if (next >= pb->active_count) {
        if (manual_skip || mode == PLAY_MODE_REPEAT_ALL) return 0;
        return -1;
    }
    return next;
}

int playlists_prev_active_index(const PlaylistBank *pb)
{
    if (!pb || pb->active_count <= 0) return -1;
    int prev = pb->active_playing_index - 1;
    if (prev < 0) prev = pb->active_count - 1;
    return prev;
}

/* ---------------- Persistent Config (erencyk.cfg) ---------------- */

void config_load(AppState *app)
{
    if (!app || !app->app_base_dir[0]) return;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%.*s/erencyk.cfg", MAX_PATH_LEN - 16, app->app_base_dir);

    FILE *fp = fopen(path, "r");
    if (!fp) return;

    char key[64];
    int val = 0;
    while (fscanf(fp, "%63[^=]=%d\n", key, &val) == 2) {
        if (strcmp(key, "theme") == 0 && val >= 0 && val < DEVICE_THEME_COUNT) {
            app->theme_id = (DeviceThemeId)val;
        } else if (strcmp(key, "lcd_override") == 0 && val >= 0 && val < LCD_OV_COUNT) {
            app->lcd_override = (LcdOverrideId)val;
        } else if (strcmp(key, "eq") == 0 && val >= 0 && val < EQ_PRESET_COUNT) {
            app->eq_preset = (EqPreset)val;
        } else if (strcmp(key, "vis") == 0 && val >= 0 && val < VIS_MODE_COUNT) {
            app->vis_mode = (VisMode)val;
        } else if (strcmp(key, "play_mode") == 0 && val >= 0 && val < PLAY_MODE_COUNT) {
            app->play_mode = (PlayMode)val;
        } else if (strcmp(key, "volume") == 0 && val >= 0 && val <= 100) {
            app->volume_pct = val;
        }
    }
    fclose(fp);
}

void config_save(const AppState *app)
{
    if (!app || !app->app_base_dir[0]) return;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%.*s/erencyk.cfg", MAX_PATH_LEN - 16, app->app_base_dir);

    FILE *fp = fopen(path, "w");
    if (!fp) return;
    fprintf(fp, "theme=%d\n",        (int)app->theme_id);
    fprintf(fp, "lcd_override=%d\n", (int)app->lcd_override);
    fprintf(fp, "eq=%d\n",           (int)app->eq_preset);
    fprintf(fp, "vis=%d\n",          (int)app->vis_mode);
    fprintf(fp, "play_mode=%d\n",    (int)app->play_mode);
    fprintf(fp, "volume=%d\n",       app->volume_pct);
    fclose(fp);
}
