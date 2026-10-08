#include "events.h"
#include <SDL3/SDL.h>

static bool keys[SDL_SCANCODE_COUNT] = {false};

int process_events(TransformState *state, float dt)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return 0;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode < SDL_SCANCODE_COUNT)
                    keys[event.key.scancode] = true;
                break;
            case SDL_EVENT_KEY_UP:
                if (event.key.scancode < SDL_SCANCODE_COUNT)
                    keys[event.key.scancode] = false;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                state->tz += event.wheel.y * 0.1f;
                break;
        }
    }

    float tspeed = 5.0f * dt;
    if (keys[SDL_SCANCODE_W]) state->ty += tspeed;
    if (keys[SDL_SCANCODE_A]) state->tx -= tspeed;
    if (keys[SDL_SCANCODE_S]) state->ty -= tspeed;
    if (keys[SDL_SCANCODE_D]) state->tx += tspeed;
    if (keys[SDL_SCANCODE_Q]) state->tz += tspeed;
    if (keys[SDL_SCANCODE_E]) state->tz -= tspeed;

    float rspeed = 20.0f * dt;
    if (keys[SDL_SCANCODE_I]) state->rot_x += rspeed;
    if (keys[SDL_SCANCODE_K]) state->rot_x -= rspeed;
    if (keys[SDL_SCANCODE_L]) state->rot_y += rspeed;
    if (keys[SDL_SCANCODE_J]) state->rot_y -= rspeed;
    if (keys[SDL_SCANCODE_U]) state->rot_z += rspeed;
    if (keys[SDL_SCANCODE_O]) state->rot_z -= rspeed;

    return 1;
}
