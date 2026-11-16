#ifndef ERENCYK_UI_H
#define ERENCYK_UI_H

#include "app.h"

void ui_init(AppState *app);
void ui_show_osd(AppState *app, const char *msg);
void ui_update_physics(AppState *app, PlayState play_state, const AudioVizSnapshot *viz);
void ui_render(const AppState *app, PlayState play_state,
               const TrackInfo *track, const AudioVizSnapshot *viz);

const char *ui_device_theme_name(DeviceThemeId id);
const char *ui_lcd_override_name(LcdOverrideId id);
const char *ui_vis_name(VisMode mode);
const char *ui_play_mode_name(PlayMode mode);

#endif /* ERENCYK_UI_H */
