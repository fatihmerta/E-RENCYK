#include "browser.h"

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static bool is_mp3_filename(const char *name)
{
    if (!name || name[0] == '.') return false;
    const char *dot = strrchr(name, '.');
    if (!dot) return false;
    return (strcasecmp(dot, ".mp3") == 0 ||
            strcasecmp(dot, ".mp2") == 0 ||
            strcasecmp(dot, ".mpa") == 0);
}

static int cmp_browser_items(const void *a, const void *b)
{
    const BrowserItem *ia = (const BrowserItem *)a;
    const BrowserItem *ib = (const BrowserItem *)b;

    if (ia->is_parent != ib->is_parent) {
        return ia->is_parent ? -1 : 1;
    }
    if (ia->is_dir != ib->is_dir) {
        return ia->is_dir ? -1 : 1;
    }
    return strcasecmp(ia->name, ib->name);
}

static int cmp_track_refs(const void *a, const void *b)
{
    const TrackRef *ta = (const TrackRef *)a;
    const TrackRef *tb = (const TrackRef *)b;
    return strcasecmp(ta->title, tb->title);
}

static void trim_trailing_slash(char *path)
{
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/' && path[len - 2] != ':') {
        path[--len] = '\0';
    }
}

static bool dir_has_music_or_subdirs(const char *dir_path)
{
    DIR *d = opendir(dir_path);
    if (!d) return false;

    bool found = false;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (is_mp3_filename(ent->d_name)) {
            found = true;
            break;
        }
        char full[MAX_PATH_LEN + 260];
        size_t dlen = strlen(dir_path);
        if (dlen > 0 && dir_path[dlen - 1] == '/') {
            snprintf(full, sizeof(full), "%.*s%s", MAX_PATH_LEN - 1, dir_path, ent->d_name);
        } else {
            snprintf(full, sizeof(full), "%.*s/%s", MAX_PATH_LEN - 1, dir_path, ent->d_name);
        }
        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

bool browser_open_dir(BrowserState *b, const char *dir_path)
{
    if (!b || !dir_path || !*dir_path) return false;

    char clean_dir[MAX_PATH_LEN];
    snprintf(clean_dir, sizeof(clean_dir), "%s", dir_path);
    trim_trailing_slash(clean_dir);

    DIR *d = opendir(clean_dir);
    if (!d) return false;

    snprintf(b->current_dir, sizeof(b->current_dir), "%s", clean_dir);
    b->count = 0;
    b->selected = 0;
    b->scroll_top = 0;
    b->track_count = 0;
    b->current_track_pos = -1;

    size_t clen = strlen(clean_dir);
    bool is_root = (clen <= 5 && clean_dir[clen - 1] == '/');
    if (!is_root && b->count < MAX_BROWSER_ITEMS) {
        BrowserItem *it = &b->items[b->count++];
        snprintf(it->name, sizeof(it->name), ".. (Parent Folder)");
        it->is_dir = true;
        it->is_parent = true;

        snprintf(it->full_path, sizeof(it->full_path), "%s", clean_dir);
        char *slash = strrchr(it->full_path, '/');
        if (slash) {
            if (slash > it->full_path && *(slash - 1) == ':') {
                *(slash + 1) = '\0';
            } else {
                *slash = '\0';
            }
        }
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && b->count < MAX_BROWSER_ITEMS) {
        if (ent->d_name[0] == '.') continue;

        char full[MAX_PATH_LEN + 260];
        if (clean_dir[clen - 1] == '/') {
            snprintf(full, sizeof(full), "%.*s%s", MAX_PATH_LEN - 1, clean_dir, ent->d_name);
        } else {
            snprintf(full, sizeof(full), "%.*s/%s", MAX_PATH_LEN - 1, clean_dir, ent->d_name);
        }

        struct stat st;
        if (stat(full, &st) != 0) continue;

        bool is_dir = S_ISDIR(st.st_mode);
        if (!is_dir && !is_mp3_filename(ent->d_name)) continue;

        BrowserItem *it = &b->items[b->count++];
        snprintf(it->name, sizeof(it->name), "%.*s", (int)sizeof(it->name) - 1, ent->d_name);
        snprintf(it->full_path, sizeof(it->full_path), "%.*s", (int)sizeof(it->full_path) - 1, full);
        it->is_dir = is_dir;
        it->is_parent = false;
    }
    closedir(d);

    if (b->count > 1) {
        qsort(b->items, (size_t)b->count, sizeof(BrowserItem), cmp_browser_items);
    }

    for (int i = 0; i < b->count; i++) {
        if (!b->items[i].is_dir) {
            b->track_item_indices[b->track_count++] = i;
        }
    }

    if (b->track_count > 0) {
        b->selected = b->track_item_indices[0];
    } else {
        b->selected = 0;
    }

    return true;
}

bool browser_go_parent(BrowserState *b)
{
    if (!b || b->count <= 0) return false;
    if (b->items[0].is_parent) {
        char target[MAX_PATH_LEN];
        snprintf(target, sizeof(target), "%s", b->items[0].full_path);
        return browser_open_dir(b, target);
    }
    return false;
}

int browser_collect_dir_mp3s(const char *dir_path, TrackRef *out_refs, int max_refs)
{
    if (!dir_path || !out_refs || max_refs <= 0) return 0;

    char clean_dir[MAX_PATH_LEN];
    snprintf(clean_dir, sizeof(clean_dir), "%s", dir_path);
    trim_trailing_slash(clean_dir);

    DIR *d = opendir(clean_dir);
    if (!d) return 0;

    size_t clen = strlen(clean_dir);
    int n = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && n < max_refs) {
        if (ent->d_name[0] == '.' || !is_mp3_filename(ent->d_name)) continue;

        char full[MAX_PATH_LEN + 260];
        if (clean_dir[clen - 1] == '/') {
            snprintf(full, sizeof(full), "%.*s%s", MAX_PATH_LEN - 1, clean_dir, ent->d_name);
        } else {
            snprintf(full, sizeof(full), "%.*s/%s", MAX_PATH_LEN - 1, clean_dir, ent->d_name);
        }

        struct stat st;
        if (stat(full, &st) != 0 || S_ISDIR(st.st_mode)) continue;

        snprintf(out_refs[n].title, sizeof(out_refs[n].title), "%.*s",
                 (int)sizeof(out_refs[n].title) - 1, ent->d_name);
        char *dot = strrchr(out_refs[n].title, '.');
        if (dot) *dot = '\0';
        snprintf(out_refs[n].full_path, sizeof(out_refs[n].full_path), "%.*s",
                 (int)sizeof(out_refs[n].full_path) - 1, full);
        n++;
    }
    closedir(d);

    if (n > 1) {
        qsort(out_refs, (size_t)n, sizeof(TrackRef), cmp_track_refs);
    }
    return n;
}

void browser_init(BrowserState *b, const char *app_base_dir)
{
    memset(b, 0, sizeof(*b));
    b->current_track_pos = -1;

    char app_music[MAX_PATH_LEN] = {0};
    if (app_base_dir && *app_base_dir) {
        snprintf(app_music, sizeof(app_music), "%.*s/music", MAX_PATH_LEN - 8, app_base_dir);
    }

    const char *candidates[] = {
        "ms0:/MUSIC",
        "ms0:/PSP/MUSIC",
        "ef0:/MUSIC",
        "ef0:/PSP/MUSIC",
        app_music
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (candidates[i][0] && dir_has_music_or_subdirs(candidates[i])) {
            if (browser_open_dir(b, candidates[i])) return;
        }
    }

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (candidates[i][0] && browser_open_dir(b, candidates[i])) return;
    }

    if (!browser_open_dir(b, "ms0:/")) {
        browser_open_dir(b, app_base_dir && *app_base_dir ? app_base_dir : ".");
    }
}

void browser_cycle_quick_root(BrowserState *b, const char *app_base_dir)
{
    char app_music[MAX_PATH_LEN] = {0};
    if (app_base_dir && *app_base_dir) {
        snprintf(app_music, sizeof(app_music), "%.*s/music", MAX_PATH_LEN - 8, app_base_dir);
    }

    const char *roots[4] = {
        "ms0:/MUSIC",
        "ms0:/PSP/MUSIC",
        app_music,
        "ms0:/"
    };

    int cur = -1;
    for (int i = 0; i < 4; i++) {
        if (roots[i][0] && strcasecmp(b->current_dir, roots[i]) == 0) {
            cur = i;
            break;
        }
    }

    for (int step = 1; step <= 4; step++) {
        int idx = (cur + step) % 4;
        if (roots[idx][0] && browser_open_dir(b, roots[idx])) {
            return;
        }
    }
}

const char *browser_get_track_path(const BrowserState *b, int track_pos)
{
    if (!b || track_pos < 0 || track_pos >= b->track_count) return NULL;
    int item_idx = b->track_item_indices[track_pos];
    if (item_idx < 0 || item_idx >= b->count) return NULL;
    return b->items[item_idx].full_path;
}

int browser_next_track_pos(const BrowserState *b, PlayMode mode, bool manual_skip)
{
    if (!b || b->track_count <= 0) return -1;

    if (!manual_skip && mode == PLAY_MODE_REPEAT_ONE) {
        return (b->current_track_pos >= 0) ? b->current_track_pos : 0;
    }

    if (mode == PLAY_MODE_SHUFFLE && b->track_count > 1) {
        int next = rand() % b->track_count;
        if (next == b->current_track_pos) {
            next = (next + 1) % b->track_count;
        }
        return next;
    }

    int next = b->current_track_pos + 1;
    if (next >= b->track_count) {
        if (manual_skip || mode == PLAY_MODE_REPEAT_ALL) {
            return 0;
        }
        return -1;
    }
    return next;
}

int browser_prev_track_pos(const BrowserState *b)
{
    if (!b || b->track_count <= 0) return -1;
    int prev = b->current_track_pos - 1;
    if (prev < 0) prev = b->track_count - 1;
    return prev;
}
