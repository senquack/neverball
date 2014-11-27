/* Contains GCW Zero gsensor-related code from senquack */
#ifdef GCWZERO
#include <SDL.h>

SDL_Joystick *joy_gsensor = NULL;
int gsensor_dev_idx = -1;    //SDL device index of the gsensor so we can filter SDL JOYAXIS events
const int gsensor_max = 26000;	// Just barely under the maximum reading in any direction from the gsensor
                                             //   under normal use (outside of shaking or jerking the unit)

void recalibrate_gsensor(int *x, int *y)
{
    if (joy_gsensor) {
        *x = SDL_JoystickGetAxis(joy_gsensor, 0);
        *y = SDL_JoystickGetAxis(joy_gsensor, 1);
    }

    if ((abs(*x) > 0) && (abs(*x) < 800)) {
        // If x-axis value was very small, force it to zero because the user probably wants a 0 value.
        *x = 0;
    }
}

#endif //GCWZERO
