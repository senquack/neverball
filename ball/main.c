/*
 * Copyright (C) 2003 Robert Kooima
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

/*---------------------------------------------------------------------------*/

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "version.h"
#include "glext.h"
#include "config.h"
#include "video.h"
#include "image.h"
#include "audio.h"
#include "demo.h"
#include "progress.h"
#include "gui.h"
#include "set.h"
#include "tilt.h"
#include "hmd.h"
#include "fs.h"
#include "common.h"
#include "text.h"
#include "mtrl.h"
#include "geom.h"

#include "st_conf.h"
#include "st_title.h"
#include "st_demo.h"
#include "st_level.h"
#include "st_pause.h"


//senquack:
#ifdef GCWZERO
//For new code in make_dirs_and_migrate():
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

//For new code in gsensor.c
#include "gsensor.h"

//For visual indication of gsensor toggling:
#include "hud.h"

// This is also defined within the Makefile, but placed here to doubly-ensure symlink() is correctly declared:
#define POSIX_C_SOURCE 200112L
#endif //GCWZERO

const char TITLE[] = "Neverball " VERSION;
const char ICON[] = "icon/neverball.png";

//senquack - added gsensor support - these allow us to filter how the game responds to the two joysticks
#ifdef GCWZERO
extern struct state st_play_loop; //added so I know whether to submit gsensor inputs to the game (defined in st_play.c)
static int joy_dev_idx = 0;         //SDL device index of the gcw controls (so we can filter inputs)
static int a_pressed = 0;     // Following four used to assist with G-sensor enabled/disable hotkey & A/B/X/Y movement
static int b_pressed = 0;      
static int x_pressed = 0;
static int y_pressed = 0;
extern int ticks_when_hotkey_pressed;     // Defined in st_play.c
extern int hotkey_pressed; //Defined in st_play.c
extern int l_pressed;   // Defined in st_play.c
extern int r_pressed;   // Defined in st_play.c
#endif

/*---------------------------------------------------------------------------*/

static void shot(void)
{
    static char filename[MAXSTR];
    sprintf(filename, "Screenshots/screen%05d.png", config_screenshot());
    video_snap(filename);
}

/*---------------------------------------------------------------------------*/

static void toggle_wire(void)
{
#if !ENABLE_OPENGLES
    static int wire = 0;

    if (wire)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        wire = 0;
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        wire = 1;
    }
#endif
}

/*---------------------------------------------------------------------------*/

static int handle_key_dn(SDL_Event *e)
{
    int d = 1;
    int c = e->key.keysym.sym;

    /* SDL made me do it. */
#ifdef __APPLE__
    if (c == SDLK_q && e->key.keysym.mod & KMOD_GUI)
        return 0;
#endif
#ifdef _WIN32
    if (c == SDLK_F4 && e->key.keysym.mod & KMOD_ALT)
        return 0;
#endif

    switch (c)
    {
    case KEY_SCREENSHOT:
        shot();
        break;
    case KEY_FPS:
        config_tgl_d(CONFIG_FPS);
        break;
    case KEY_WIREFRAME:
        if (config_cheat())
            toggle_wire();
        break;
    case KEY_RESOURCES:
        if (config_cheat())
        {
            light_load();
            mtrl_reload();
        }
        break;
    case SDLK_RETURN:
        d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);
        break;
    case KEY_EXIT:
        d = st_keybd(KEY_EXIT, 1);
        break;

    default:
        if (config_tst_d(CONFIG_KEY_FORWARD, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), -1.0f);
        else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), +1.0f);
        else if (config_tst_d(CONFIG_KEY_LEFT, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), -1.0f);
        else if (config_tst_d(CONFIG_KEY_RIGHT, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), +1.0f);
        else
            d = st_keybd(e->key.keysym.sym, 1);
    }

    return d;
}

static int handle_key_up(SDL_Event *e)
{
    int d = 1;
    int c = e->key.keysym.sym;

    switch (c)
    {
    case SDLK_RETURN:
        d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 0);
        break;
    case KEY_EXIT:
        d = st_keybd(KEY_EXIT, 0);
        break;
    default:
        if (config_tst_d(CONFIG_KEY_FORWARD, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0);
        else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0);
        else if (config_tst_d(CONFIG_KEY_LEFT, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0);
        else if (config_tst_d(CONFIG_KEY_RIGHT, c))
            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0);
        else
            d = st_keybd(e->key.keysym.sym, 0);
    }

    return d;
}

static int loop(void)
{
    SDL_Event e;
    int d = 1;

    int ax, ay, dx, dy;

#ifdef GCWZERO
    float val;
    //senquack - added this to determine if we're in the actual game or not:
    int in_actual_game;
    //senquack - holds value of new configuration option finesse-scale for 'finesse-mode'
    float finesse_scale;
#endif
    /* Process SDL events. */

    while (d && SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_QUIT:
            return 0;

        case SDL_MOUSEMOTION:
            /* Convert to OpenGL coordinates. */

            ax = +e.motion.x;
            ay = -e.motion.y + video.window_h;
            dx = +e.motion.xrel;
            dy = (config_get_d(CONFIG_MOUSE_INVERT) ?
                  +e.motion.yrel : -e.motion.yrel);

            /* Convert to pixels. */

            ax = ROUND(ax * video.device_scale);
            ay = ROUND(ay * video.device_scale);
            dx = ROUND(dx * video.device_scale);
            dy = ROUND(dy * video.device_scale);

            st_point(ax, ay, dx, dy);

            break;

        case SDL_MOUSEBUTTONDOWN:
            d = st_click(e.button.button, 1);
            break;

        case SDL_MOUSEBUTTONUP:
            d = st_click(e.button.button, 0);
            break;

//senquack - on GCW Zero, keyboard events are disabled (SDL2.0 does not disable them automatically)
#ifndef GCWZERO
        case SDL_KEYDOWN:
            d = handle_key_dn(&e);
            break;

        case SDL_KEYUP:
            d = handle_key_up(&e);
            break;
#endif //GCWZERO

        case SDL_WINDOWEVENT:
            switch (e.window.event)
            {
            case SDL_WINDOWEVENT_FOCUS_LOST:
                if (video_get_grab())
                    goto_state(&st_pause);
                break;

            case SDL_WINDOWEVENT_MOVED:
                if (config_get_d(CONFIG_DISPLAY) != video_display())
                    config_set_d(CONFIG_DISPLAY, video_display());
                break;

            case SDL_WINDOWEVENT_RESIZED:
                log_printf("Resize event (%u, %dx%d)\n",
                           e.window.windowID,
                           e.window.data1,
                           e.window.data2);
                break;

            case SDL_WINDOWEVENT_SIZE_CHANGED:
                log_printf("Size change event (%u, %dx%d)\n",
                           e.window.windowID,
                           e.window.data1,
                           e.window.data2);
                break;
            }
            break;

        case SDL_TEXTINPUT:
            text_input_str(e.text.text, 1);
            break;

        case SDL_JOYAXISMOTION:
#ifdef GCWZERO
            //senquack - added gsensor & analog stick support:
            // A/B/X/Y button movement override all other inputs:
            if (!(((y_pressed || b_pressed) && e.jaxis.axis == 1) || 
                     ((x_pressed || a_pressed) && e.jaxis.axis == 0))) {
               if (e.jaxis.which == gsensor_dev_idx) {
                  if (config_get_d(CONFIG_GSENSOR_ENABLED) && joy_gsensor && curr_state() == &st_play_loop ) {
                     // This is a gsensor input
                     int gsensor_centerx = config_get_d(CONFIG_GSENSOR_CENTERX);
                     int gsensor_centery = config_get_d(CONFIG_GSENSOR_CENTERY);
                     int gsensor_deadzone = config_get_d(CONFIG_GSENSOR_DEADZONE) * 500;
                     int gsensor_effective_max = gsensor_max - (config_get_d(CONFIG_GSENSOR_SENSITIVITY) * 1000);
                     int gsensor_val = e.jaxis.value;

                     if (abs(gsensor_centerx) > abs(gsensor_centery)) {
                        gsensor_effective_max -= abs(gsensor_centerx);
                     } else {
                        gsensor_effective_max -= abs(gsensor_centery);
                     }

                     if (e.jaxis.axis == 0) {
                        gsensor_val -= gsensor_centerx;
                        if (gsensor_val < -gsensor_effective_max) {
                           gsensor_val = -gsensor_effective_max;
                        } else if (gsensor_val > gsensor_effective_max) {
                           gsensor_val = gsensor_effective_max;
                        }
                     } else {
                        gsensor_val -= gsensor_centery;
                        if (gsensor_val < -gsensor_effective_max) {
                           gsensor_val = -gsensor_effective_max;
                        } else if (gsensor_val > gsensor_effective_max) {
                           gsensor_val = gsensor_effective_max;
                        }
                     }

                     float gsensor_fval;

                     if (abs(gsensor_val) < gsensor_deadzone) {
                        gsensor_fval = 0;
                     } else {
                        gsensor_fval = (float)gsensor_val;
                        gsensor_fval /= (float)gsensor_effective_max;   // Convert to number between -1.0 and 1.0

                        if (gsensor_fval > 1.0) {
                           gsensor_fval = 1.0;
                        } else if (gsensor_fval < -1.0) {
                           gsensor_fval = -1.0;
                        }

                        // Is non-linear sensitivity requested?
                        if (config_get_d(CONFIG_GSENSOR_NONLINEAR)) {
                           gsensor_fval = gsensor_fval * gsensor_fval * gsensor_fval;
                        }

                        if (config_get_d(CONFIG_FINESSE_MODE_ENABLED)) {
                           gsensor_fval *= (float)config_get_d(CONFIG_FINESSE_SCALE) / 100.0f;
                        }

                     }
                     st_stick(e.jaxis.axis, gsensor_fval);
                  }
               } else {
                  // This is an input from the GCW analog stick:
                  int analog_val = e.jaxis.value;
                  // Only accept inputs if we're in the actual game and the input is more than the deadzone:
                  if (config_get_d(CONFIG_ANALOG_ENABLED) && curr_state() == &st_play_loop) {
                     int analog_sensitivity = config_get_d(CONFIG_ANALOG_SENSITIVITY);
                     int analog_effective_max;
                     if (analog_sensitivity > 1) {
                        analog_effective_max = 32767 - ((analog_sensitivity-1) * 2000);
                     } else {
                        analog_effective_max = 32767;
                     }

                     int analog_deadzone = config_get_d(CONFIG_ANALOG_DEADZONE) * 1000;
                     float analog_fval;
                     if (abs(analog_val) < analog_deadzone) {
                        analog_fval = 0;
                     } else {
                        analog_fval = (float)analog_val;
                        analog_fval /= (float)analog_effective_max;  // Convert to value between -1.0 and 1.0
                        if (analog_fval > 1.0) {
                           analog_fval = 1.0;
                        } else if (analog_fval < -1.0) {
                           analog_fval = -1.0;
                        }
                        if (analog_sensitivity < 1) {
                           // if sensitivity is set very low, use non-linear sensitivity:
                           analog_fval = analog_fval * analog_fval * analog_fval;
                        }

                        if (config_get_d(CONFIG_FINESSE_MODE_ENABLED)) {
                           analog_fval *= (float)config_get_d(CONFIG_FINESSE_SCALE) / 100.0f;
                        }
                     }
                     st_stick(e.jaxis.axis, analog_fval);
                  }
               }
            }
#else
            st_stick(e.jaxis.axis, JOY_VALUE(e.jaxis.value));
#endif //GCWZERO
            break;

         //senquack - DPAD on GCWZERO is a HAT so we need to add support for those events:
#ifdef GCWZERO
        case SDL_JOYHATMOTION:
            in_actual_game = curr_state() == &st_play_loop;

            if (config_get_d(CONFIG_FINESSE_MODE_ENABLED)) {
               finesse_scale = (float)config_get_d(CONFIG_FINESSE_SCALE) / 100.0f;
            } else {
               finesse_scale = 1.0f;
            }

            switch (e.jhat.value) {
               case SDL_HAT_UP:
                  if (in_actual_game) {
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        val = -1.0f;
                        st_stick(1, val * finesse_scale);
                     }
                  } else {
                     st_stick(1, -1.0f);
                  }
                  break;
               case SDL_HAT_DOWN:
                  if (in_actual_game) {
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        val = 1.0f;
                        st_stick(1, val * finesse_scale);
                     }
                  } else {
                     st_stick(1, 1.0f);
                  }
                  break;
               case SDL_HAT_LEFT:
                  if (in_actual_game) {
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        val = -1.0f;
                        st_stick(0, val * finesse_scale);
                     }
                  } else {
                     st_stick(0, -1.0f);
                  }
                  break;
               case SDL_HAT_RIGHT:
                  if (in_actual_game) {
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        val = 1.0f;
                        st_stick(0, val * finesse_scale);
                     }
                  } else {
                     st_stick(0, 1.0f);
                  }
                  break;
               case SDL_HAT_LEFTUP:
                  if (in_actual_game) {
                     //First, handle the X-axis:
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        val = -1.0f;
                        st_stick(0, val * finesse_scale);
                     }
                     //Then, handle the Y-axis:
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        val = -1.0f;
                        st_stick(1, val * finesse_scale);
                     }
                  } else {
                     st_stick(0, -1.0f);
                     st_stick(1, -1.0f);
                  }
                  break;
               case SDL_HAT_RIGHTUP:
                  if (in_actual_game) {
                     //First, handle the X-axis:
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        val = 1.0f;
                        st_stick(0, val * finesse_scale);
                     }
                     //Then, handle the Y-axis:
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        val = -1.0f;
                        st_stick(1, val * finesse_scale);
                     }
                  } else {
                     st_stick(0, 1.0f);
                     st_stick(1, -1.0f);
                  }
                  break;
               case SDL_HAT_LEFTDOWN:
                  if (in_actual_game) {
                     //First, handle the X-axis:
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        val = -1.0f;
                        st_stick(0, val * finesse_scale);
                     }
                     //Then, handle the Y-axis:
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        val = 1.0f;
                        st_stick(1, val * finesse_scale);
                     }
                  } else {
                     st_stick(0, -1.0f);
                     st_stick(1, 1.0f);
                  }
                  break;
               case SDL_HAT_RIGHTDOWN:
                  if (in_actual_game) {
                     //First, handle the X-axis:
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        val = 1.0f;
                        st_stick(0, val * finesse_scale);
                     }
                     //Then, handle the Y-axis:
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        val = 1.0f;
                        st_stick(1, val * finesse_scale);
                     }
                  } else {
                     st_stick(0, 1.0f);
                     st_stick(1, 1.0f);
                  }
                  break;
               case SDL_HAT_CENTERED:
                  if (in_actual_game) {
                     //First, handle the X-axis:
                     if (!(x_pressed || a_pressed)) {    // X/A override any other X-axis input in finesse mode
                        st_stick(0, 0.0f);
                     }
                     //Then, handle the Y-axis:
                     if (!(y_pressed || b_pressed)) {    // Y/B override any other Y-axis input in-game in all modes
                        st_stick(1, 0.0f);
                     }
                  } else {
                     st_stick(0, 0.0f);
                     st_stick(1, 0.0f);
                  }
                  break;
               default:
                  break;
            }
            break;
#endif //GCWZERO
        case SDL_JOYBUTTONDOWN:
#ifdef GCWZERO
            if (curr_state() == &st_play_loop) {

               // In-Game, A/B/X/Y buttons always act as Up/Down/Left/Right and override all other inputs
               if (e.jbutton.button == GCWZERO_B) {           // is B pressed? (down)
                  b_pressed = 1;              
                  st_stick(1, 1.0f);
               } else if (e.jbutton.button == GCWZERO_Y) {    // is Y pressed? (up)
                  y_pressed= 1;
                  st_stick(1, -1.0f);
               } else if (e.jbutton.button == GCWZERO_X) {    // is X pressed? (left)
                  x_pressed = 1;
                  st_stick(0, -1.0f);
               } else if (e.jbutton.button == GCWZERO_A) {    // is A pressed? (right)
                  a_pressed = 1;
                  st_stick(0, 1.0f);
               } else {
                  d = st_buttn(e.jbutton.button, 1);
               }
            } else {
               d = st_buttn(e.jbutton.button, 1);
            }
#else
            d = st_buttn(e.jbutton.button, 1);
#endif
            break;

        case SDL_JOYBUTTONUP:
#ifdef GCWZERO
            // Always reset the tracker variables when A/B/X/Y buttons come up, even if not in-game:
            if (e.jbutton.button == GCWZERO_Y) {
               y_pressed = 0;
            } else if (e.jbutton.button == GCWZERO_B) {
               b_pressed = 0;
            } else if (e.jbutton.button == GCWZERO_X) {
               x_pressed = 0;
            } else if (e.jbutton.button == GCWZERO_A) {
               a_pressed = 0;
            } else if (e.jbutton.button == GCWZERO_SELECT) {
               // Always reset select-hotkey timer even when out of the actual game
               ticks_when_hotkey_pressed = 0;
               hotkey_pressed = 0;
            } else if (e.jbutton.button == GCWZERO_L) {
               l_pressed = 0;
            } else if (e.jbutton.button == GCWZERO_R) {
               r_pressed = 0;
            }
            
            if (curr_state() == &st_play_loop) {
               // In-game, A/B/X/Y buttons always act as Up/Down/Left/Right and override all other inputs
               if (e.jbutton.button == GCWZERO_Y || e.jbutton.button == GCWZERO_B) {           // is B/Y unpressed? (down/up)
                  st_stick(1, 0.0f);
               } else if (e.jbutton.button == GCWZERO_X || e.jbutton.button == GCWZERO_A) {    // is X/A unpressed? (left/right)
                  st_stick(0, 0.0f);
               } else {
                  d = st_buttn(e.jbutton.button, 0);
               }
            } else {
               d = st_buttn(e.jbutton.button, 0);
            }
#else
            d = st_buttn(e.jbutton.button, 0);
#endif
            break;

        case SDL_MOUSEWHEEL:
            st_wheel(e.wheel.x, e.wheel.y);
            break;
        }
    }

    /* Process events via the tilt sensor API. */
//senquack - not used on GCW zero:
#ifndef GCWZERO
    if (tilt_stat())
    {
        int b;
        int s;

        st_angle(tilt_get_x(),
                 tilt_get_z());

        while (tilt_get_button(&b, &s))
        {
            const int X = config_get_d(CONFIG_JOYSTICK_AXIS_X0);
            const int Y = config_get_d(CONFIG_JOYSTICK_AXIS_Y0);
            const int L = config_get_d(CONFIG_JOYSTICK_DPAD_L);
            const int R = config_get_d(CONFIG_JOYSTICK_DPAD_R);
            const int U = config_get_d(CONFIG_JOYSTICK_DPAD_U);
            const int D = config_get_d(CONFIG_JOYSTICK_DPAD_D);

            if (b == L || b == R || b == U || b == D)
            {
                static int pad[4] = { 0, 0, 0, 0 };

                /* Track the state of the D-pad buttons. */

                if      (b == L) pad[0] = s;
                else if (b == R) pad[1] = s;
                else if (b == U) pad[2] = s;
                else if (b == D) pad[3] = s;

                /* Convert D-pad button events into joystick axis motion. */

                if      (pad[0] && !pad[1]) st_stick(X, -1.0f);
                else if (pad[1] && !pad[0]) st_stick(X, +1.0f);
                else                        st_stick(X,  0.0f);

                if      (pad[2] && !pad[3]) st_stick(Y, -1.0f);
                else if (pad[3] && !pad[2]) st_stick(Y, +1.0f);
                else                        st_stick(Y,  0.0f);
            }
            else d = st_buttn(b, s);
        }
    }
#endif //GCWZERO

    return d;
}

/*---------------------------------------------------------------------------*/

static char *opt_data;
static char *opt_replay;
static char *opt_level;

#define opt_usage                                                     \
    "Usage: %s [options ...]\n"                                       \
    "Options:\n"                                                      \
    "  -h, --help                show this usage message.\n"          \
    "  -v, --version             show version.\n"                     \
    "  -d, --data <dir>          use 'dir' as game data directory.\n" \
    "  -r, --replay <file>       play the replay 'file'.\n"           \
    "  -l, --level <file>        load the level 'file'\n"

#define opt_error(option) \
    fprintf(stderr, "Option '%s' requires an argument.\n", option)

static void opt_parse(int argc, char **argv)
{
    int i;

    /* Scan argument list. */

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help")    == 0)
        {
            printf(opt_usage, argv[0]);
            exit(EXIT_SUCCESS);
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
        {
            printf("%s\n", VERSION);
            exit(EXIT_SUCCESS);
        }

        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data")    == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_data = argv[++i];
            continue;
        }

        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--replay")  == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_replay = argv[++i];
            continue;
        }

        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--level")  == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_level = argv[++i];
            continue;
        }

        /* Perform magic on a single unrecognized argument. */

        if (argc == 2)
        {
            size_t len = strlen(argv[i]);
            int level = 0;

            if (len > 4)
            {
                char *ext = argv[i] + len - 4;

                if (strcmp(ext, ".map") == 0)
                    strncpy(ext, ".sol", 4);

                if (strcmp(ext, ".sol") == 0)
                    level = 1;
            }

            if (level)
                opt_level = argv[i];
            else
                opt_replay = argv[i];

            break;
        }
    }
}

#undef opt_usage
#undef opt_error

/*---------------------------------------------------------------------------*/

static int is_replay(struct dir_item *item)
{
    return str_ends_with(item->path, ".nbr");
}

static int is_score_file(struct dir_item *item)
{
    return str_starts_with(item->path, "neverballhs-");
}

static void make_dirs_and_migrate(void)
{
    Array items;
    int i;

    const char *src;
    char *dst;

    if (fs_mkdir("Replays"))
    {
        if ((items = fs_dir_scan("", is_replay)))
        {
            for (i = 0; i < array_len(items); i++)
            {
                src = DIR_ITEM_GET(items, i)->path;
                dst = concat_string("Replays/", src, NULL);
                fs_rename(src, dst);
                free(dst);
            }

            fs_dir_free(items);
        }
    }

/* senquack - the working-replay file (Last.nbr) is now written to Replays/tmp/ which is a symlink to /tmp and this 
    should fix the random stutters and increased SD/SSD-wear coming from constantly writing to a file while playing. */
#ifdef GCWZERO
    int retval;
    retval = mkdir("/tmp/neverball", 0755);
    if (retval) {
        printf("Error in make_dirs_and_migrate() creating folder /tmp/neverball\n");
        printf("%s\n", strerror(errno));
    } else {
        retval = symlink("/tmp/neverball", "/media/data/local/home/.neverball/Replays/tmp");
        if (retval) {
            printf("Error in make_dirs_and_migrate() creating symlink Replays/tmp to /tmp/neverball\n");
            printf("%s\n", strerror(errno));
        }
    }
#endif

    if (fs_mkdir("Scores"))
    {
        if ((items = fs_dir_scan("", is_score_file)))
        {
            for (i = 0; i < array_len(items); i++)
            {
                src = DIR_ITEM_GET(items, i)->path;
                dst = concat_string("Scores/",
                                    src + sizeof ("neverballhs-") - 1,
                                    ".txt",
                                    NULL);
                fs_rename(src, dst);
                free(dst);
            }

            fs_dir_free(items);
        }
    }

    fs_mkdir("Screenshots");
}

/*---------------------------------------------------------------------------*/


int main(int argc, char *argv[])
{
    SDL_Joystick *joy = NULL;


    int t1, t0;

    if (!fs_init(argv[0]))
    {
        fprintf(stderr, "Failure to initialize virtual file system (%s)\n",
                fs_error());
        return 1;
    }

    opt_parse(argc, argv);

    config_paths(opt_data);
    log_init("Neverball", "neverball.log");
    make_dirs_and_migrate();

    /* Initialize SDL. */

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) == -1)
    {
        log_printf("Failure to initialize SDL (%s)\n", SDL_GetError());
        return 1;
    }

    /* Intitialize configuration. */

    config_init();
    config_load();

    /* Initialize localization. */

    lang_init();

    /* Initialize joystick. */

//senquack - added support for GCW Zero's tilt sensor (g-sensor):
//    if (config_get_d(CONFIG_JOYSTICK) && SDL_NumJoysticks() > 0)
//    {
//        joy = SDL_JoystickOpen(config_get_d(CONFIG_JOYSTICK_DEVICE));
//        if (joy)
//            SDL_JoystickEventState(SDL_ENABLE);
//    }
#ifdef GCWZERO
	printf ("Initializing joysticks for GCW Zero.. Total joysticks reported: %d\n", SDL_NumJoysticks());
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_Joystick *tmp_joy = NULL;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
        tmp_joy = SDL_JoystickOpen(i);
        if (tmp_joy) {
            printf("Joystick %u: \"%s\"\n", i, SDL_JoystickName(tmp_joy));
            if (strcmp(SDL_JoystickName(tmp_joy), "linkdev device (Analog 2-axis 8-button 2-hat)") == 0) {
                joy = tmp_joy;
                joy_dev_idx = i;
				printf("Recognized GCW Zero's built-in analog stick..\n");
            } else if (strcmp(SDL_JoystickName(tmp_joy), "mxc6225") == 0) {
                joy_gsensor = tmp_joy;
                gsensor_dev_idx = i;
                printf("Recognized GCW Zero's built-in g-sensor..\n");
            }
		}
	}

    if (!joy_gsensor) {
        printf("Error: failed to recognize GCW Zero's g-sensor\n");
    }

    if (!joy) {
        printf("Error: failed to recognize GCW Zero's joystick controls\n");
        if (SDL_NumJoysticks() > 0) {
            joy = SDL_JoystickOpen(0);
        }
        if (!joy) {
            printf("Error: failed to recognize any joysticks at all!\n");
        } else {
            printf("Using this joystick as default instead: %s\n", SDL_JoystickName(joy));
        }
    } 

#else
    if (config_get_d(CONFIG_JOYSTICK) && SDL_NumJoysticks() > 0)
    {
        joy = SDL_JoystickOpen(config_get_d(CONFIG_JOYSTICK_DEVICE));
        if (joy)
            SDL_JoystickEventState(SDL_ENABLE);
    }
#endif //GCWZERO

    

    /* Initialize audio. */

    audio_init();
    tilt_init();

    /* Initialize video. */

    if (!video_init())
        return 1;

    /* Material system. */

    mtrl_init();

    /* Screen states. */

    init_state(&st_null);

    /* Initialize demo playback or load the level. */

    if (opt_replay &&
        fs_add_path(dir_name(opt_replay)) &&
        progress_replay(base_name(opt_replay)))
    {
        demo_play_goto(1);
        goto_state(&st_demo_play);
    }
    else if (opt_level)
    {
        const char *path = fs_resolve(opt_level);
        int loaded = 0;

        if (path)
        {
            /* HACK: must be around for the duration of the game. */
            static struct level level;

            if (level_load(path, &level))
            {
                progress_init(MODE_STANDALONE);

                if (progress_play(&level))
                {
                    goto_state(&st_level);
                    loaded = 1;
                }
            }
        }
        else log_printf("File %s is not in game path\n", opt_level);

        if (!loaded)
            goto_state(&st_title);
    }
    else
        goto_state(&st_title);

    /* Run the main game loop. */

    t0 = SDL_GetTicks();

    while (loop())
    {
        if ((t1 = SDL_GetTicks()) > t0)
        {
            /* Step the game state. */

            st_timer(0.001f * (t1 - t0));

            t0 = t1;

            /* Render. */

            //senquack
#ifndef GCWZERO
            hmd_step();
#endif //GCWZERO

            st_paint(0.001f * t0);
            video_swap();

            if (config_get_d(CONFIG_NICE))
                SDL_Delay(1);
        }
    }

    config_save();

    mtrl_quit();

    if (joy)
        SDL_JoystickClose(joy);

//senquack - added support for GCW Zero's tilt sensor (g-sensor):
#ifdef GCWZERO
    if (joy_gsensor)
        SDL_JoystickClose(joy_gsensor);
#endif

    tilt_free();
    hmd_free();
    SDL_Quit();

    return 0;
}

/*---------------------------------------------------------------------------*/

