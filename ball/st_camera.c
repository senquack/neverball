//senquack - st_controls.c and st_controls.h define our new controls settings screen
//    NOTE: adapted from share/st_common.c

/*
 * Copyright (C) 2013 Neverball authors
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */


#include <SDL.h>


#include "audio.h"
#include "config.h"
#include "geom.h"
#include "gui.h"

#include "st_common.h"
#include "st_camera.h"

#define AUD_MENU "snd/menu.ogg"

#ifdef GCWZERO
#define ROT_SPEED_RANGE_MIN  50
#define ROT_SPEED_RANGE_INC  50
#define ROT_SPEED_RANGE_MAX (ROT_SPEED_RANGE_MIN + (ROT_SPEED_RANGE_INC * 10))

/*
 * Map rot_speed values to [0, 10]
 */
#define ROT_SPEED_RANGE_MAP(m) \
    CLAMP(0, ((m - ROT_SPEED_RANGE_MIN) / ROT_SPEED_RANGE_INC), 10)

#define ROT_SPEED_RANGE_UNMAP(i) \
    (ROT_SPEED_RANGE_MIN + (i * ROT_SPEED_RANGE_INC))
#endif //GCWZERO

/*---------------------------------------------------------------------------*/

enum
{
    CAMERA_ROT_SPEED = GUI_LAST,
    CAMERA_NORMAL_ROT_ACCEL,
    CAMERA_FINESSE_ROT_ACCEL,
    CAMERA_SCREEN_TILT_ENABLED,
    CAMERA_REVERSED_CAMERA_ROTATION,
};

static int rot_speed_id[11];
static int normal_rot_accel_id[11];
static int finesse_rot_accel_id[11];

static struct state *camera_back;

static int camera_action(int tok, int val)
{
    int r = 1;

    int rot_speed = ROT_SPEED_RANGE_MAP(config_get_d(CONFIG_ROTATE_FAST));  
    int normal_rot_accel = config_get_d(CONFIG_ROTATE_ACCEL_NORMAL);
    int finesse_rot_accel = config_get_d(CONFIG_ROTATE_ACCEL_FINESSE);

    audio_play(AUD_MENU, 1.0f);

    switch (tok) {
        case GUI_BACK:
            goto_state(camera_back);
            camera_back = NULL;
            break;

        case CAMERA_ROT_SPEED:
            config_set_d(CONFIG_ROTATE_FAST, ROT_SPEED_RANGE_UNMAP(val));
            gui_toggle(rot_speed_id[val]);
            gui_toggle(rot_speed_id[rot_speed]);
            break;

        case CAMERA_NORMAL_ROT_ACCEL:
            config_set_d(CONFIG_ROTATE_ACCEL_NORMAL, val);
            gui_toggle(normal_rot_accel_id[val]);
            gui_toggle(normal_rot_accel_id[normal_rot_accel]);
            break;

        case CAMERA_FINESSE_ROT_ACCEL:
            config_set_d(CONFIG_ROTATE_ACCEL_FINESSE, val);
            gui_toggle(finesse_rot_accel_id[val]);
            gui_toggle(finesse_rot_accel_id[finesse_rot_accel]);
            break;

        case CAMERA_SCREEN_TILT_ENABLED:
            goto_state(&st_null);
            config_set_d(CONFIG_SCREEN_TILT_ENABLED, val);
            goto_state(&st_camera);
            break;

        case CAMERA_REVERSED_CAMERA_ROTATION:
            goto_state(&st_null);
            config_set_d(CONFIG_REVERSED_CAMERA_ROTATION, val);
            goto_state(&st_camera);
            break;
        
        default:
            break;
    }
    return r;
}

static int camera_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Camera"), GUI_BACK);

        int rot_speed = ROT_SPEED_RANGE_MAP(config_get_d(CONFIG_ROTATE_FAST));
        int normal_rot_accel = config_get_d(CONFIG_ROTATE_ACCEL_NORMAL);
        int finesse_rot_accel = config_get_d(CONFIG_ROTATE_ACCEL_FINESSE);

        conf_slider(id, _("Max rotation speed"), CAMERA_ROT_SPEED, rot_speed,
                    rot_speed_id, ARRAYSIZE(rot_speed_id));

        conf_slider(id, _("Normal rotation accel."), CAMERA_NORMAL_ROT_ACCEL, normal_rot_accel,
                    normal_rot_accel_id, ARRAYSIZE(normal_rot_accel_id));

        conf_slider(id, _("Finesse-mode rotation accel."), CAMERA_FINESSE_ROT_ACCEL, finesse_rot_accel,
                    finesse_rot_accel_id, ARRAYSIZE(finesse_rot_accel_id));

        gui_space(id);

        conf_toggle(id, _("Reversed camera rotation"), CAMERA_REVERSED_CAMERA_ROTATION,
                config_get_d(CONFIG_REVERSED_CAMERA_ROTATION), _("On"), 1, _("Off"), 0);

        conf_toggle(id, _("Draw floor tilt"),   CAMERA_SCREEN_TILT_ENABLED,
                    config_get_d(CONFIG_SCREEN_TILT_ENABLED), _("On"), 1, _("Off"), 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int camera_enter(struct state *st, struct state *prev)
{
    if (!camera_back)
        camera_back = prev;

    conf_common_init(camera_action);
    return camera_gui();
}

/*---------------------------------------------------------------------------*/

struct state st_camera = {
    camera_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

/*---------------------------------------------------------------------------*/
