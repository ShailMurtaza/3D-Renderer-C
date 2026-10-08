#ifndef EVENTS_H
#define EVENTS_H

typedef struct {
    float rot_x, rot_y, rot_z;
    float tx, ty, tz;
} TransformState;

int process_events(TransformState *state, float dt);

#endif
