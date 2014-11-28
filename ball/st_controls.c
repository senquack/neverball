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
#include "st_controls.h"
#include "gsensor.h"    // New GCW-Zero g-sensor controls code

#define AUD_MENU "snd/menu.ogg"

//senquack - added two options to control camera rotation speeds
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
   CONTROLS_GSENSOR_ENABLED = GUI_LAST,
   CONTROLS_GSENSOR_RECALIBRATE,
   CONTROLS_GSENSOR_SENSITIVITY,
   CONTROLS_GSENSOR_DEADZONE,
   CONTROLS_GSENSOR_NONLINEAR,
   CONTROLS_ANALOG_ENABLED,
   CONTROLS_ANALOG_SENSITIVITY,
   CONTROLS_ANALOG_DEADZONE,
   CONTROLS_DEFAULT_ROT_SPEED,
   CONTROLS_MODIFIED_ROT_SPEED,
   CONTROLS_SCREEN_TILT_ENABLED,
};

static int gsensor_sensitivity_id[11];
static int analog_sensitivity_id[11];
static int gsensor_deadzone_id[11];
static int analog_deadzone_id[11];
static int default_rot_speed_id[11];
static int modified_rot_speed_id[11];

static struct state *controls_back;

static int controls_action(int tok, int val)
{
    int r = 1;

    int new_gsensor_centerx = 0, new_gsensor_centery = 0;
    int gsensor_sensitivity = config_get_d(CONFIG_GSENSOR_SENSITIVITY);
    int analog_sensitivity = config_get_d(CONFIG_ANALOG_SENSITIVITY);
    int gsensor_deadzone = config_get_d(CONFIG_GSENSOR_DEADZONE);
    int analog_deadzone = config_get_d(CONFIG_ANALOG_DEADZONE);
    int default_rot_speed = ROT_SPEED_RANGE_MAP(config_get_d(CONFIG_ROTATE_SLOW));  
    int modified_rot_speed = ROT_SPEED_RANGE_MAP(config_get_d(CONFIG_ROTATE_FAST));

    audio_play(AUD_MENU, 1.0f);

    switch (tok)
    {
    case GUI_BACK:
        goto_state(controls_back);
        controls_back = NULL;
        break;

    case CONTROLS_GSENSOR_ENABLED:
        goto_state(&st_null);
        config_set_d(CONFIG_GSENSOR_ENABLED, val);
        goto_state(&st_controls);
        break;

    case CONTROLS_GSENSOR_RECALIBRATE:
        goto_state(&st_null);
        new_gsensor_centerx = config_get_d(CONFIG_GSENSOR_CENTERX);
        new_gsensor_centery = config_get_d(CONFIG_GSENSOR_CENTERY);
        recalibrate_gsensor(&new_gsensor_centerx, &new_gsensor_centery);
        config_set_d(CONFIG_GSENSOR_CENTERX, new_gsensor_centerx);
        config_set_d(CONFIG_GSENSOR_CENTERY, new_gsensor_centery);
        goto_state(&st_controls);
        break;

    case CONTROLS_GSENSOR_SENSITIVITY:
        config_set_d(CONFIG_GSENSOR_SENSITIVITY, val);
        gui_toggle(gsensor_sensitivity_id[val]);
        gui_toggle(gsensor_sensitivity_id[gsensor_sensitivity]);
        break;

    case CONTROLS_GSENSOR_DEADZONE:
        config_set_d(CONFIG_GSENSOR_DEADZONE, val);
        gui_toggle(gsensor_deadzone_id[val]);
        gui_toggle(gsensor_deadzone_id[gsensor_deadzone]);
        break;

    case CONTROLS_GSENSOR_NONLINEAR:
        goto_state(&st_null);
        config_set_d(CONFIG_GSENSOR_NONLINEAR, val);
        goto_state(&st_controls);
        break;

    case CONTROLS_ANALOG_ENABLED:
        goto_state(&st_null);
        config_set_d(CONFIG_ANALOG_ENABLED, val);
        goto_state(&st_controls);
        break;

    case CONTROLS_ANALOG_SENSITIVITY:
        config_set_d(CONFIG_ANALOG_SENSITIVITY, val);
        gui_toggle(analog_sensitivity_id[val]);
        gui_toggle(analog_sensitivity_id[analog_sensitivity]);
        break;

    case CONTROLS_ANALOG_DEADZONE:
        config_set_d(CONFIG_ANALOG_DEADZONE, val);
        gui_toggle(analog_deadzone_id[val]);
        gui_toggle(analog_deadzone_id[analog_deadzone]);
        break;

    case CONTROLS_DEFAULT_ROT_SPEED:
        config_set_d(CONFIG_ROTATE_SLOW, ROT_SPEED_RANGE_UNMAP(val));
        gui_toggle(default_rot_speed_id[val]);
        gui_toggle(default_rot_speed_id[default_rot_speed]);
        break;

    case CONTROLS_MODIFIED_ROT_SPEED:
        config_set_d(CONFIG_ROTATE_FAST, ROT_SPEED_RANGE_UNMAP(val));
        gui_toggle(modified_rot_speed_id[val]);
        gui_toggle(modified_rot_speed_id[modified_rot_speed]);
        break;

    case CONTROLS_SCREEN_TILT_ENABLED:
        goto_state(&st_null);
        config_set_d(CONFIG_SCREEN_TILT_ENABLED, val);
        goto_state(&st_controls);
        break;
    }

    return r;
}

static int controls_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
       char gsensor_calibration[40];
       sprintf(gsensor_calibration, "X:%d Y:%d",
             config_get_d(CONFIG_GSENSOR_CENTERX),
             config_get_d(CONFIG_GSENSOR_CENTERY));

       conf_header(id, _("Controls"), GUI_BACK);

       int gsensor_sensitivity = config_get_d(CONFIG_GSENSOR_SENSITIVITY);
       int analog_sensitivity = config_get_d(CONFIG_ANALOG_SENSITIVITY);
       int gsensor_deadzone = config_get_d(CONFIG_GSENSOR_DEADZONE);
       int analog_deadzone = config_get_d(CONFIG_ANALOG_DEADZONE);
       int default_rot_speed = ROT_SPEED_RANGE_MAP(config_get_d(CONFIG_ROTATE_SLOW));
       int modified_rot_speed = ROT_SPEED_RANGE_MAP(config_get_d(CONFIG_ROTATE_FAST));

        conf_toggle(id, _("G-Sensor"),   CONTROLS_GSENSOR_ENABLED,
                    config_get_d(CONFIG_GSENSOR_ENABLED), _("On"), 1, _("Off"), 0);

        if ((jd = conf_state (id, _("Calibrate G-Sensor"), gsensor_calibration, CONTROLS_GSENSOR_RECALIBRATE))) {
           // TODO: if gsensor wasn't initialized, gray out this button
        }

        conf_slider(id, _("G-Sensor sensitivity"), CONTROLS_GSENSOR_SENSITIVITY, gsensor_sensitivity,
                    gsensor_sensitivity_id, ARRAYSIZE(gsensor_sensitivity_id));

        conf_slider(id, _("G-Sensor deadzone"), CONTROLS_GSENSOR_DEADZONE, gsensor_deadzone,
                    gsensor_deadzone_id, ARRAYSIZE(gsensor_deadzone_id));

        conf_toggle(id, _("Non-linear G-Sensor sensitivity"),   CONTROLS_GSENSOR_NONLINEAR,
                    config_get_d(CONFIG_GSENSOR_NONLINEAR), _("On"), 1, _("Off"), 0);

        gui_space(id);

        conf_toggle(id, _("Analog stick"),   CONTROLS_ANALOG_ENABLED,
                    config_get_d(CONFIG_ANALOG_ENABLED), _("On"), 1, _("Off"), 0);

        conf_slider(id, _("Analog sensitivity"), CONTROLS_ANALOG_SENSITIVITY, analog_sensitivity,
                    analog_sensitivity_id, ARRAYSIZE(analog_sensitivity_id));

        conf_slider(id, _("Analog deadzone"), CONTROLS_ANALOG_DEADZONE, analog_deadzone,
                    analog_deadzone_id, ARRAYSIZE(analog_deadzone_id));

        gui_space(id);
        
        conf_slider(id, _("Default camera rotation speed"), CONTROLS_DEFAULT_ROT_SPEED, default_rot_speed,
                    default_rot_speed_id, ARRAYSIZE(default_rot_speed_id));

        conf_slider(id, _("Modified camera rotation speed"), CONTROLS_MODIFIED_ROT_SPEED, modified_rot_speed,
                    modified_rot_speed_id, ARRAYSIZE(modified_rot_speed_id));

        conf_toggle(id, _("Draw floor tilt"),   CONTROLS_SCREEN_TILT_ENABLED,
                    config_get_d(CONFIG_SCREEN_TILT_ENABLED), _("On"), 1, _("Off"), 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int controls_enter(struct state *st, struct state *prev)
{
    if (!controls_back)
        controls_back = prev;

    conf_common_init(controls_action);
    return controls_gui();
}

/*---------------------------------------------------------------------------*/

struct state st_controls = {
    controls_enter,
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
