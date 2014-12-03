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
   CONTROLS_FINESSE_MODE_INDICATOR,
   CONTROLS_FINESSE_SCALE,
   CONTROLS_FINESSE_MODE_AFFECTS,
};

static int gsensor_sensitivity_id[11];
static int analog_sensitivity_id[11];
static int gsensor_deadzone_id[11];
static int analog_deadzone_id[11];
static int finesse_scale_id[11];

static struct state *controls_back;

static int controls_action(int tok, int val)
{
    int r = 1;

    int new_gsensor_centerx = 0, new_gsensor_centery = 0;
    int gsensor_sensitivity = config_get_d(CONFIG_GSENSOR_SENSITIVITY);
    int analog_sensitivity = config_get_d(CONFIG_ANALOG_SENSITIVITY);
    int gsensor_deadzone = config_get_d(CONFIG_GSENSOR_DEADZONE);
    int analog_deadzone = config_get_d(CONFIG_ANALOG_DEADZONE);
    int finesse_scale = config_get_d(CONFIG_FINESSE_SCALE);
    int finesse_mode_affects = config_get_d(CONFIG_FINESSE_MODE_AFFECTS);

    audio_play(AUD_MENU, 1.0f);

    switch (tok) {
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

       case CONTROLS_FINESSE_MODE_INDICATOR:
          goto_state(&st_null);
          config_set_d(CONFIG_FINESSE_MODE_INDICATOR, val);
          goto_state(&st_controls);
          break;

       case CONTROLS_FINESSE_SCALE:
          config_set_d(CONFIG_FINESSE_SCALE, val);
          gui_toggle(finesse_scale_id[val]);
          gui_toggle(finesse_scale_id[finesse_scale]);
          break;

       case CONTROLS_FINESSE_MODE_AFFECTS:
          goto_state(&st_null);
          finesse_mode_affects++;
          if (finesse_mode_affects > FINESSE_MODE_AFFECTS_MAX) {
             finesse_mode_affects = FINESSE_MODE_AFFECTS_MIN;
          }
          config_set_d(CONFIG_FINESSE_MODE_AFFECTS, finesse_mode_affects);
          goto_state(&st_controls);
          break;

       default:
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

      char finesse_mode_affects_txt[sizeof ("DPAD, Analog, G-Sensor")];
      int finesse_mode_affects = config_get_d(CONFIG_FINESSE_MODE_AFFECTS);
      if (finesse_mode_affects == FINESSE_DPAD) {
         sprintf(finesse_mode_affects_txt, "DPAD");
      } else if (finesse_mode_affects == FINESSE_ANALOG) {
         sprintf(finesse_mode_affects_txt, "Analog");
      } else if (finesse_mode_affects == FINESSE_GSENSOR) {
         sprintf(finesse_mode_affects_txt, "G-Sensor");
      } else if (finesse_mode_affects == (FINESSE_DPAD | FINESSE_ANALOG)) {
         sprintf(finesse_mode_affects_txt, "DPAD, Analog");
      } else if (finesse_mode_affects == (FINESSE_DPAD | FINESSE_GSENSOR)) {
         sprintf(finesse_mode_affects_txt, "DPAD, G-Sensor");
      } else if (finesse_mode_affects == (FINESSE_ANALOG | FINESSE_GSENSOR)) {
         sprintf(finesse_mode_affects_txt, "Analog, G-Sensor");
      } else if (finesse_mode_affects == (FINESSE_ANALOG | FINESSE_GSENSOR | FINESSE_DPAD)) {
         sprintf(finesse_mode_affects_txt, "DPAD, Analog, G-Sensor");
      } else {
         sprintf(finesse_mode_affects_txt, "ERROR");
      }

      int gsensor_sensitivity = config_get_d(CONFIG_GSENSOR_SENSITIVITY);
      int analog_sensitivity = config_get_d(CONFIG_ANALOG_SENSITIVITY);
      int gsensor_deadzone = config_get_d(CONFIG_GSENSOR_DEADZONE);
      int analog_deadzone = config_get_d(CONFIG_ANALOG_DEADZONE);
      int finesse_scale = config_get_d(CONFIG_FINESSE_SCALE);

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

      gui_label(id, _("Warning! Sensitivity left of middle lessens range of motion:"), GUI_SML, 0, 0);

      conf_slider(id, _("Analog sensitivity"), CONTROLS_ANALOG_SENSITIVITY, analog_sensitivity,
            analog_sensitivity_id, ARRAYSIZE(analog_sensitivity_id));


      conf_slider(id, _("Analog deadzone"), CONTROLS_ANALOG_DEADZONE, analog_deadzone,
            analog_deadzone_id, ARRAYSIZE(analog_deadzone_id));

      gui_space(id);

      conf_toggle(id, _("Finesse mode indicator"), CONTROLS_FINESSE_MODE_INDICATOR, 
            config_get_d(CONFIG_FINESSE_MODE_INDICATOR), _("On"), 1, _("Off"), 0);

      conf_slider(id, _("Finesse speed scale"), CONTROLS_FINESSE_SCALE, finesse_scale,
            finesse_scale_id, ARRAYSIZE(finesse_scale_id));

      jd = conf_state (id, _("Finesse mode affects"), finesse_mode_affects_txt, CONTROLS_FINESSE_MODE_AFFECTS);

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
