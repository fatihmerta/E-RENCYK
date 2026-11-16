#include "app.h"
#include "gfx.h"
#include "font.h"
#include "cover.h"
#include "audio.h"
#include "browser.h"
#include "playlist.h"
#include "ui.h"

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <psprtc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

PSP_MODULE_INFO("E-RENCYK", 0, 2, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

static AppState s_app;
static TrackRef s_dir_scratch[MAX_PLAYLIST_TRACKS];

static int exit_callback(int arg1, int arg2, void *common)
{
    (void)arg1;
    (void)arg2;
    (void)common;
    s_app.running = false;
    return 0;
}

static int callback_thread(SceSize args, void *argp)
{
    (void)args;
    (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", callback_thread,
                                     0x11, 0xFA0, 0, NULL);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, NULL);
    }
}

static void resolve_app_base_dir(int argc, char **argv, char *out, size_t out_sz)
{
    if (argc > 0 && argv && argv[0] && argv[0][0]) {
        snprintf(out, out_sz, "%s", argv[0]);
        char *slash = strrchr(out, '/');
        if (slash) {
            *slash = '\0';
            return;
        }
    }
    if (!getcwd(out, out_sz)) {
        snprintf(out, out_sz, "ms0:/PSP/GAME/ERENCYK");
    }
}

static void poll_system_status(AppState *app)
{
    int bat = scePowerGetBatteryLifePercent();
    app->battery_pct = (bat >= 0 && bat <= 100) ? bat : 100;
    app->battery_charging = (scePowerIsBatteryCharging() == 1);

    ScePspDateTime dt;
    if (sceRtcGetCurrentClockLocalTime(&dt) >= 0) {
        app->clock_hour = (int)dt.hour;
        app->clock_min  = (int)dt.minute;
    }
}

static void sync_scroll(int *selected, int *scroll_top, int count, int visible_rows)
{
    if (count <= 0) {
        *selected = 0;
        *scroll_top = 0;
        return;
    }
    if (*selected < 0) *selected = 0;
    if (*selected >= count) *selected = count - 1;

    if (*selected < *scroll_top) {
        *scroll_top = *selected;
    } else if (*selected >= *scroll_top + visible_rows) {
        *scroll_top = *selected - visible_rows + 1;
    }
    if (*scroll_top < 0) *scroll_top = 0;
}

/* ---------------- Playback Dispatch Across Folder / Queue / Playlist ---------------- */

static bool play_folder_track(AppState *app, int track_pos, bool auto_play)
{
    const char *path = browser_get_track_path(&app->browser, track_pos);
    if (!path) return false;

    if (audio_load_track(path, auto_play, app->theme_id)) {
        app->play_source = PLAY_SRC_FOLDER;
        app->browser.current_track_pos = track_pos;
        app->marquee_px = 0.0f;
        app->marquee_pause = 75;
        return true;
    }
    ui_show_osd(app, "Failed to decode MP3");
    return false;
}

static bool play_queue_track(AppState *app, int q_idx, bool auto_play)
{
    if (q_idx < 0 || q_idx >= app->queue.count) return false;
    if (audio_load_track(app->queue.tracks[q_idx].full_path, auto_play, app->theme_id)) {
        app->play_source = PLAY_SRC_QUEUE;
        app->queue.playing_index = q_idx;
        app->marquee_px = 0.0f;
        app->marquee_pause = 75;
        return true;
    }
    ui_show_osd(app, "Failed to decode queued MP3");
    return false;
}

static bool play_playlist_track(AppState *app, int pl_idx, int track_idx, bool auto_play)
{
    if (!playlists_activate_for_play(&app->playlists, pl_idx, track_idx)) {
        ui_show_osd(app, "Playlist is empty");
        return false;
    }
    int idx = app->playlists.active_playing_index;
    if (audio_load_track(app->playlists.active_tracks[idx].full_path, auto_play, app->theme_id)) {
        app->play_source = PLAY_SRC_PLAYLIST;
        app->marquee_px = 0.0f;
        app->marquee_pause = 75;
        return true;
    }
    ui_show_osd(app, "Failed to decode track");
    return false;
}

static bool play_active_playlist_index(AppState *app, int idx, bool auto_play)
{
    if (idx < 0 || idx >= app->playlists.active_count) return false;
    if (audio_load_track(app->playlists.active_tracks[idx].full_path, auto_play, app->theme_id)) {
        app->play_source = PLAY_SRC_PLAYLIST;
        app->playlists.active_playing_index = idx;
        app->marquee_px = 0.0f;
        app->marquee_pause = 75;
        return true;
    }
    return false;
}

static void step_next_track(AppState *app, bool manual_skip)
{
    if (app->play_source == PLAY_SRC_QUEUE && app->queue.count > 0) {
        int next = queue_next_index(&app->queue, app->play_mode, manual_skip);
        if (next >= 0) play_queue_track(app, next, true);
        return;
    }
    if (app->play_source == PLAY_SRC_PLAYLIST && app->playlists.active_count > 0) {
        int next = playlists_next_active_index(&app->playlists, app->play_mode, manual_skip);
        if (next >= 0) play_active_playlist_index(app, next, true);
        return;
    }
    if (app->browser.track_count > 0) {
        int next = browser_next_track_pos(&app->browser, app->play_mode, manual_skip);
        if (next >= 0) play_folder_track(app, next, true);
    }
}

static void step_prev_track(AppState *app)
{
    TrackInfo ti;
    audio_get_track_info(&ti);
    if (ti.position_sec > 3.5) {
        audio_seek_relative(-ti.position_sec);
        ui_show_osd(app, "<< Restart Track");
        return;
    }

    if (app->play_source == PLAY_SRC_QUEUE && app->queue.count > 0) {
        int prev = queue_prev_index(&app->queue);
        if (prev >= 0) play_queue_track(app, prev, true);
        return;
    }
    if (app->play_source == PLAY_SRC_PLAYLIST && app->playlists.active_count > 0) {
        int prev = playlists_prev_active_index(&app->playlists);
        if (prev >= 0) play_active_playlist_index(app, prev, true);
        return;
    }
    if (app->browser.track_count > 0) {
        int prev = browser_prev_track_pos(&app->browser);
        if (prev >= 0) play_folder_track(app, prev, true);
    }
}

/* Add highlighted item in FILES tab to Queue */
static void add_selected_file_to_queue(AppState *app)
{
    BrowserState *br = &app->browser;
    if (br->count <= 0 || br->selected < 0 || br->selected >= br->count) return;
    const BrowserItem *it = &br->items[br->selected];
    if (it->is_parent) return;

    bool was_empty = (app->queue.count == 0);
    int added = 0;

    if (it->is_dir) {
        int n = browser_collect_dir_mp3s(it->full_path, s_dir_scratch, MAX_PLAYLIST_TRACKS);
        for (int i = 0; i < n; i++) {
            if (queue_add_track(&app->queue, s_dir_scratch[i].title, s_dir_scratch[i].full_path)) {
                added++;
            }
        }
    } else {
        if (queue_add_track(&app->queue, it->name, it->full_path)) {
            added = 1;
        }
    }

    if (added > 0) {
        if (audio_get_state() == PLAY_STATE_STOPPED && was_empty) {
            play_queue_track(app, 0, false);
        } else if (app->play_source != PLAY_SRC_QUEUE) {
            /* Ensure Queue plays up next right after current song! */
            app->play_source = PLAY_SRC_QUEUE;
            app->queue.playing_index = -1;
        }
        char msg[72];
        snprintf(msg, sizeof(msg), "+%d Added to Queue (%d total)", added, app->queue.count);
        ui_show_osd(app, msg);
    } else {
        ui_show_osd(app, "No MP3 tracks to queue");
    }
}

static void execute_add_pending_to_playlist(AppState *app, int pl_idx)
{
    if (pl_idx < 0 || pl_idx >= app->playlists.count) return;

    int added = 0;
    if (app->save_queue_as_playlist) {
        added = playlists_append_many(&app->playlists, pl_idx,
                                      app->queue.tracks, app->queue.count);
    } else if (app->pending_add_is_dir) {
        int n = browser_collect_dir_mp3s(app->pending_add_path, s_dir_scratch, MAX_PLAYLIST_TRACKS);
        added = playlists_append_many(&app->playlists, pl_idx, s_dir_scratch, n);
    } else if (app->pending_add_path[0]) {
        if (playlists_append_one(&app->playlists, pl_idx,
                                 app->pending_add_title, app->pending_add_path)) {
            added = 1;
        }
    }

    char msg[72];
    snprintf(msg, sizeof(msg), "Saved %d song(s) to '%s'",
             added, app->playlists.list[pl_idx].name);
    ui_show_osd(app, msg);
}

/* ---------------- Input Controller ---------------- */

static void handle_input(AppState *app, uint32_t pressed, uint32_t held,
                         int analog_x, int analog_y, int *repeat_timer)
{
    app->hold_locked = ((held & PSP_CTRL_HOLD) != 0);
    app->held_buttons = app->hold_locked ? 0 : held;

    if (app->hold_locked) {
        *repeat_timer = 0;
        return;
    }

    uint32_t nav_mask = PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT;
    uint32_t repeat_fire = 0;
    if (held & nav_mask) {
        (*repeat_timer)++;
        if (*repeat_timer == 18 || (*repeat_timer > 18 && ((*repeat_timer % 4) == 0))) {
            repeat_fire = held & nav_mask;
        }
    } else {
        *repeat_timer = 0;
    }

    /* -------- MODAL 1: SELECT Theme Studio -------- */
    if (app->modal == MODAL_THEME_STUDIO) {
        if ((pressed | repeat_fire) & PSP_CTRL_UP) {
            app->theme_id = (DeviceThemeId)((app->theme_id + DEVICE_THEME_COUNT - 1) % DEVICE_THEME_COUNT);
        }
        if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
            app->theme_id = (DeviceThemeId)((app->theme_id + 1) % DEVICE_THEME_COUNT);
        }
        if ((pressed | repeat_fire) & PSP_CTRL_LEFT) {
            app->lcd_override = (LcdOverrideId)((app->lcd_override + LCD_OV_COUNT - 1) % LCD_OV_COUNT);
        }
        if ((pressed | repeat_fire) & PSP_CTRL_RIGHT) {
            app->lcd_override = (LcdOverrideId)((app->lcd_override + 1) % LCD_OV_COUNT);
        }
        if (pressed & PSP_CTRL_SQUARE) {
            app->theme_id = THEME_WALKMAN_SILVER;
            app->lcd_override = LCD_OV_AUTO;
            ui_show_osd(app, "Reset to Default Walkman Silver");
        }
        if (pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_SELECT)) {
            config_save(app);
            app->modal = MODAL_NONE;
            char msg[72];
            snprintf(msg, sizeof(msg), "Theme: %s", ui_device_theme_name(app->theme_id));
            ui_show_osd(app, msg);
        }
        return;
    }

    /* -------- MODAL 2: Add-to-Playlist Picker -------- */
    if (app->modal == MODAL_ADD_TO_PLAYLIST) {
        int total_opts = 1 + app->playlists.count;
        if ((pressed | repeat_fire) & PSP_CTRL_UP) {
            app->popup_selected = (app->popup_selected - 1 + total_opts) % total_opts;
            sync_scroll(&app->popup_selected, &app->popup_scroll, total_opts, 7);
        }
        if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
            app->popup_selected = (app->popup_selected + 1) % total_opts;
            sync_scroll(&app->popup_selected, &app->popup_scroll, total_opts, 7);
        }
        if (pressed & PSP_CTRL_CROSS) {
            if (app->popup_selected == 0) {
                app->preset_name_idx = 0;
                app->popup_scroll = 0;
                app->modal = MODAL_NEW_PLAYLIST_NAME;
            } else {
                execute_add_pending_to_playlist(app, app->popup_selected - 1);
                app->modal = MODAL_LIBRARY;
            }
        }
        if (pressed & PSP_CTRL_CIRCLE) {
            app->modal = MODAL_LIBRARY;
        }
        return;
    }

    /* -------- MODAL 3: Create New Playlist Name Picker -------- */
    if (app->modal == MODAL_NEW_PLAYLIST_NAME) {
        int total_names = playlist_preset_name_count();
        if ((pressed | repeat_fire) & PSP_CTRL_UP) {
            app->preset_name_idx = (app->preset_name_idx - 1 + total_names) % total_names;
            sync_scroll(&app->preset_name_idx, &app->popup_scroll, total_names, 7);
        }
        if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
            app->preset_name_idx = (app->preset_name_idx + 1) % total_names;
            sync_scroll(&app->preset_name_idx, &app->popup_scroll, total_names, 7);
        }
        if (pressed & PSP_CTRL_CROSS) {
            const char *chosen_name = playlist_preset_name(app->preset_name_idx);
            int pl_idx = playlists_create(&app->playlists, app->app_base_dir, chosen_name);
            if (pl_idx >= 0) {
                app->playlists.selected = pl_idx;
                if (app->save_queue_as_playlist || app->pending_add_path[0]) {
                    execute_add_pending_to_playlist(app, pl_idx);
                    app->pending_add_path[0] = '\0';
                    app->save_queue_as_playlist = false;
                } else {
                    char msg[72];
                    snprintf(msg, sizeof(msg), "Created Playlist '%s'",
                             app->playlists.list[pl_idx].name);
                    ui_show_osd(app, msg);
                }
            }
            app->modal = MODAL_LIBRARY;
        }
        if (pressed & PSP_CTRL_CIRCLE) {
            app->pending_add_path[0] = '\0';
            app->save_queue_as_playlist = false;
            app->modal = MODAL_LIBRARY;
        }
        return;
    }

    /* SELECT opens Theme Studio from both Main Deck and Library! */
    if (pressed & PSP_CTRL_SELECT) {
        app->modal = MODAL_THEME_STUDIO;
        return;
    }

    /* -------- MODAL 4: 3-Tab Library / Queue / Playlists Drawer -------- */
    if (app->modal == MODAL_LIBRARY) {
        if (pressed & PSP_CTRL_LTRIGGER) {
            app->lib_tab = (LibraryTab)((app->lib_tab + LIB_TAB_COUNT - 1) % LIB_TAB_COUNT);
            return;
        }
        if (pressed & PSP_CTRL_RTRIGGER) {
            app->lib_tab = (LibraryTab)((app->lib_tab + 1) % LIB_TAB_COUNT);
            return;
        }
        if (pressed & PSP_CTRL_CIRCLE) {
            app->modal = MODAL_NONE;
            return;
        }

        /* Tab 0: FILES */
        if (app->lib_tab == LIB_TAB_FILES) {
            BrowserState *br = &app->browser;
            if ((pressed | repeat_fire) & PSP_CTRL_UP) {
                if (br->count > 0) {
                    br->selected = (br->selected - 1 + br->count) % br->count;
                    sync_scroll(&br->selected, &br->scroll_top, br->count, 10);
                }
            }
            if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
                if (br->count > 0) {
                    br->selected = (br->selected + 1) % br->count;
                    sync_scroll(&br->selected, &br->scroll_top, br->count, 10);
                }
            }
            if ((pressed | repeat_fire) & PSP_CTRL_LEFT) {
                br->selected -= 8;
                sync_scroll(&br->selected, &br->scroll_top, br->count, 10);
            }
            if ((pressed | repeat_fire) & PSP_CTRL_RIGHT) {
                br->selected += 8;
                sync_scroll(&br->selected, &br->scroll_top, br->count, 10);
            }

            if (pressed & PSP_CTRL_CROSS) {
                if (br->count > 0 && br->selected >= 0 && br->selected < br->count) {
                    BrowserItem chosen = br->items[br->selected];
                    if (chosen.is_dir) {
                        browser_open_dir(br, chosen.full_path);
                    } else {
                        for (int t = 0; t < br->track_count; t++) {
                            if (br->track_item_indices[t] == br->selected) {
                                play_folder_track(app, t, true);
                                app->modal = MODAL_NONE;
                                break;
                            }
                        }
                    }
                }
            }

            /* Square: Add to Queue */
            if (pressed & PSP_CTRL_SQUARE) {
                add_selected_file_to_queue(app);
            }

            /* Triangle: Add to Playlist */
            if (pressed & PSP_CTRL_TRIANGLE) {
                if (br->count > 0 && br->selected >= 0 && br->selected < br->count) {
                    const BrowserItem *it = &br->items[br->selected];
                    if (!it->is_parent) {
                        snprintf(app->pending_add_path, sizeof(app->pending_add_path), "%s", it->full_path);
                        snprintf(app->pending_add_title, sizeof(app->pending_add_title), "%s", it->name);
                        app->pending_add_is_dir = it->is_dir;
                        app->save_queue_as_playlist = false;
                        app->popup_selected = (app->playlists.count > 0) ? 1 : 0;
                        app->popup_scroll = 0;
                        app->modal = MODAL_ADD_TO_PLAYLIST;
                    }
                }
            }

            /* Start: Switch Drive / Root Folder */
            if (pressed & PSP_CTRL_START) {
                browser_cycle_quick_root(br, app->app_base_dir);
                char msg[72];
                snprintf(msg, sizeof(msg), "Folder: %.58s", br->current_dir);
                ui_show_osd(app, msg);
            }
            return;
        }

        /* Tab 1: QUEUE */
        if (app->lib_tab == LIB_TAB_QUEUE) {
            QueueState *q = &app->queue;
            if ((pressed | repeat_fire) & PSP_CTRL_UP) {
                if (q->count > 0) {
                    q->selected = (q->selected - 1 + q->count) % q->count;
                    sync_scroll(&q->selected, &q->scroll_top, q->count, 10);
                }
            }
            if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
                if (q->count > 0) {
                    q->selected = (q->selected + 1) % q->count;
                    sync_scroll(&q->selected, &q->scroll_top, q->count, 10);
                }
            }
            if (pressed & PSP_CTRL_CROSS) {
                if (q->count > 0) {
                    play_queue_track(app, q->selected, true);
                    app->modal = MODAL_NONE;
                }
            }
            if (pressed & PSP_CTRL_SQUARE) {
                if (q->count > 0) {
                    queue_remove_at(q, q->selected);
                    sync_scroll(&q->selected, &q->scroll_top, q->count, 10);
                    ui_show_osd(app, "Removed from Queue");
                }
            }
            if (pressed & PSP_CTRL_TRIANGLE) {
                if (q->count > 0) {
                    app->save_queue_as_playlist = true;
                    app->pending_add_path[0] = '\0';
                    app->preset_name_idx = 0;
                    app->popup_scroll = 0;
                    app->modal = MODAL_NEW_PLAYLIST_NAME;
                } else {
                    ui_show_osd(app, "Queue is empty");
                }
            }
            if (pressed & PSP_CTRL_START) {
                queue_clear(q);
                if (app->play_source == PLAY_SRC_QUEUE) {
                    app->play_source = PLAY_SRC_FOLDER;
                }
                ui_show_osd(app, "Queue Cleared");
            }
            return;
        }

        /* Tab 2: PLAYLISTS */
        PlaylistBank *pb = &app->playlists;
        if (!pb->inside_playlist) {
            if ((pressed | repeat_fire) & PSP_CTRL_UP) {
                if (pb->count > 0) {
                    pb->selected = (pb->selected - 1 + pb->count) % pb->count;
                    sync_scroll(&pb->selected, &pb->scroll_top, pb->count, 10);
                }
            }
            if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
                if (pb->count > 0) {
                    pb->selected = (pb->selected + 1) % pb->count;
                    sync_scroll(&pb->selected, &pb->scroll_top, pb->count, 10);
                }
            }
            if (pressed & PSP_CTRL_CROSS) {
                if (pb->count > 0) {
                    playlists_open(pb, pb->selected);
                }
            }
            if (pressed & PSP_CTRL_TRIANGLE) {
                app->save_queue_as_playlist = false;
                app->pending_add_path[0] = '\0';
                app->preset_name_idx = 0;
                app->popup_scroll = 0;
                app->modal = MODAL_NEW_PLAYLIST_NAME;
            }
            if (pressed & PSP_CTRL_SQUARE) {
                if (pb->count > 0) {
                    playlists_delete(pb, app->app_base_dir, pb->selected);
                    sync_scroll(&pb->selected, &pb->scroll_top, pb->count, 10);
                    ui_show_osd(app, "Playlist Deleted");
                }
            }
            if (pressed & PSP_CTRL_START) {
                if (pb->count > 0 && play_playlist_track(app, pb->selected, 0, true)) {
                    app->modal = MODAL_NONE;
                }
            }
        } else {
            if ((pressed | repeat_fire) & PSP_CTRL_UP) {
                if (pb->opened_count > 0) {
                    pb->track_selected = (pb->track_selected - 1 + pb->opened_count) % pb->opened_count;
                    sync_scroll(&pb->track_selected, &pb->track_scroll_top, pb->opened_count, 10);
                }
            }
            if ((pressed | repeat_fire) & PSP_CTRL_DOWN) {
                if (pb->opened_count > 0) {
                    pb->track_selected = (pb->track_selected + 1) % pb->opened_count;
                    sync_scroll(&pb->track_selected, &pb->track_scroll_top, pb->opened_count, 10);
                }
            }
            if (pressed & PSP_CTRL_CROSS) {
                if (pb->opened_count > 0 &&
                    play_playlist_track(app, pb->opened_index, pb->track_selected, true)) {
                    app->modal = MODAL_NONE;
                }
            }
            if (pressed & PSP_CTRL_SQUARE) {
                if (pb->opened_count > 0) {
                    playlists_remove_opened_track(pb, pb->track_selected);
                    sync_scroll(&pb->track_selected, &pb->track_scroll_top, pb->opened_count, 10);
                    ui_show_osd(app, "Removed Song from Playlist");
                }
            }
            if (pressed & PSP_CTRL_TRIANGLE) {
                pb->inside_playlist = false;
            }
            if (pressed & PSP_CTRL_START) {
                if (pb->opened_count > 0) {
                    app->play_mode = PLAY_MODE_SHUFFLE;
                    int rnd = rand() % pb->opened_count;
                    if (play_playlist_track(app, pb->opened_index, rnd, true)) {
                        app->modal = MODAL_NONE;
                        ui_show_osd(app, "Shuffling Playlist");
                    }
                }
            }
        }
        return;
    }

    /* -------- Main Hi-Fi Deck Controls -------- */
    if (pressed & PSP_CTRL_LTRIGGER) {
        step_prev_track(app);
    }
    if (pressed & PSP_CTRL_RTRIGGER) {
        step_next_track(app, true);
    }

    if (pressed & PSP_CTRL_START) {
        app->play_mode = (PlayMode)((app->play_mode + 1) % PLAY_MODE_COUNT);
        config_save(app);
        char msg[64];
        snprintf(msg, sizeof(msg), "Play Mode: %s", ui_play_mode_name(app->play_mode));
        ui_show_osd(app, msg);
    }

    if (pressed & PSP_CTRL_CIRCLE) {
        app->modal = MODAL_LIBRARY;
        return;
    }

    if (pressed & PSP_CTRL_CROSS) {
        PlayState st = audio_get_state();
        if (st == PLAY_STATE_PLAYING || st == PLAY_STATE_PAUSED) {
            audio_toggle_pause();
        } else if (app->play_source == PLAY_SRC_QUEUE && app->queue.count > 0) {
            int pos = (app->queue.playing_index >= 0) ? app->queue.playing_index : 0;
            play_queue_track(app, pos, true);
        } else if (app->play_source == PLAY_SRC_PLAYLIST && app->playlists.active_count > 0) {
            int pos = (app->playlists.active_playing_index >= 0) ? app->playlists.active_playing_index : 0;
            play_active_playlist_index(app, pos, true);
        } else if (app->browser.track_count > 0) {
            int pos = (app->browser.current_track_pos >= 0) ? app->browser.current_track_pos : 0;
            play_folder_track(app, pos, true);
        } else {
            app->modal = MODAL_LIBRARY;
        }
    }

    if (pressed & PSP_CTRL_SQUARE) {
        app->eq_preset = (EqPreset)((app->eq_preset + 1) % EQ_PRESET_COUNT);
        audio_apply_eq(app->eq_preset);
        config_save(app);
        char msg[64];
        snprintf(msg, sizeof(msg), "Sound EQ: %s", audio_eq_name(app->eq_preset));
        ui_show_osd(app, msg);
    }

    if (pressed & PSP_CTRL_TRIANGLE) {
        app->vis_mode = (VisMode)((app->vis_mode + 1) % VIS_MODE_COUNT);
        config_save(app);
        char msg[64];
        snprintf(msg, sizeof(msg), "Meter: %s", ui_vis_name(app->vis_mode));
        ui_show_osd(app, msg);
    }

    if (((pressed | repeat_fire) & PSP_CTRL_UP) || (analog_y < -55 && ((*repeat_timer % 5) == 0))) {
        app->volume_pct += 4;
        if (app->volume_pct > 100) app->volume_pct = 100;
        audio_set_volume(app->volume_pct);
        config_save(app);
        char msg[48];
        snprintf(msg, sizeof(msg), "Volume: %d%%", app->volume_pct);
        ui_show_osd(app, msg);
    }
    if (((pressed | repeat_fire) & PSP_CTRL_DOWN) || (analog_y > 55 && ((*repeat_timer % 5) == 0))) {
        app->volume_pct -= 4;
        if (app->volume_pct < 0) app->volume_pct = 0;
        audio_set_volume(app->volume_pct);
        config_save(app);
        char msg[48];
        snprintf(msg, sizeof(msg), "Volume: %d%%", app->volume_pct);
        ui_show_osd(app, msg);
    }

    if ((pressed | repeat_fire) & PSP_CTRL_LEFT) {
        audio_seek_relative(-5.0);
        ui_show_osd(app, "<< Rewind 5s");
    } else if ((pressed | repeat_fire) & PSP_CTRL_RIGHT) {
        audio_seek_relative(+5.0);
        ui_show_osd(app, "Fast-Forward 5s >>");
    } else if (analog_x < -64) {
        audio_seek_relative(-0.35);
    } else if (analog_x > 64) {
        audio_seek_relative(+0.35);
    }
}

int main(int argc, char **argv)
{
    setup_callbacks();
    scePowerSetClockFrequency(333, 333, 166);
    srand((unsigned int)time(NULL));

    memset(&s_app, 0, sizeof(s_app));
    s_app.running      = true;
    s_app.modal        = MODAL_NONE;
    s_app.lib_tab      = LIB_TAB_FILES;
    s_app.play_source  = PLAY_SRC_FOLDER;
    s_app.play_mode    = PLAY_MODE_REPEAT_ALL;
    s_app.eq_preset    = EQ_FLAT; /* Pure reference audio by default! */
    s_app.vis_mode     = VIS_ANALOG_VU;
    s_app.theme_id     = THEME_WALKMAN_SILVER;
    s_app.lcd_override = LCD_OV_AUTO;
    s_app.volume_pct   = 85;
    s_app.battery_pct  = 100;

    resolve_app_base_dir(argc, argv, s_app.app_base_dir, sizeof(s_app.app_base_dir));
    config_load(&s_app);

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    gfx_init();
    font_init();
    cover_init();
    ui_init(&s_app);
    queue_init(&s_app.queue);
    playlists_init(&s_app.playlists, s_app.app_base_dir);

    audio_init();
    audio_set_volume(s_app.volume_pct);
    audio_apply_eq(s_app.eq_preset);

    browser_init(&s_app.browser, s_app.app_base_dir);
    if (s_app.browser.track_count > 0) {
        play_folder_track(&s_app, 0, false);
        ui_show_osd(&s_app, "Ready - Press Cross (X) to Play");
    } else {
        s_app.modal = MODAL_LIBRARY;
        ui_show_osd(&s_app, "Select an MP3 Folder");
    }

    poll_system_status(&s_app);

    uint32_t prev_buttons = 0;
    int repeat_timer = 0;
    int frame_counter = 0;

    while (s_app.running) {
        SceCtrlData pad;
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(&pad, 1);

        uint32_t pressed = pad.Buttons & ~prev_buttons;
        prev_buttons = pad.Buttons;

        int ax = (int)pad.Lx - 128;
        int ay = (int)pad.Ly - 128;

        handle_input(&s_app, pressed, pad.Buttons, ax, ay, &repeat_timer);

        if (audio_consume_track_ended()) {
            step_next_track(&s_app, false);
        }

        PlayState play_state = audio_get_state();

        if (play_state == PLAY_STATE_PLAYING && (frame_counter % 300) == 0) {
            scePowerTick(PSP_POWER_TICK_SUSPEND);
        }

        if ((frame_counter % 60) == 0) {
            poll_system_status(&s_app);
        }

        TrackInfo track;
        AudioVizSnapshot viz;
        audio_get_track_info(&track);
        audio_get_viz_snapshot(&viz);

        ui_update_physics(&s_app, play_state, &viz);
        ui_render(&s_app, play_state, &track, &viz);

        frame_counter++;
    }

    config_save(&s_app);
    audio_shutdown();
    gfx_term();
    sceKernelExitGame();
    return 0;
}
