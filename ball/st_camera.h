#ifndef ST_CAMERA_H
#define ST_CAMERA_H

//senquack - st_camera.c and st_camera.h define our new camera settings screen

#include "state.h"

/*---------------------------------------------------------------------------*/


/*
 * This is only a common declaration, this module does not implement
 * this state. Check out ball/st_conf.c and putt/st_conf.c instead.
 */
extern struct state st_null;

/*
 * These are actually implemented by this module.
 */
extern struct state st_camera;

/*---------------------------------------------------------------------------*/

#endif
