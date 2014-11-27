#ifndef ST_CONTROLS_H
#define ST_CONTROLS_H

//senquack - st_controls.c and st_controls.h define our new controls settings screen

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
extern struct state st_controls;

/*---------------------------------------------------------------------------*/

#endif
