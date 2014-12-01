/* Contains GCW Zero gsensor-related code from senquack */
#ifdef GCWZERO

#ifndef GSENSOR_H
#define GSENSOR_H
extern SDL_Joystick *joy_gsensor;
extern int gsensor_dev_idx;    //SDL device index of the gsensor so we can filter SDL JOYAXIS events

void recalibrate_gsensor(int *x, int *y);

#endif //GSENSOR_H
#endif //GCWZERO

