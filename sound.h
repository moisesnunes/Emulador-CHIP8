#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct
{
    SDL_AudioDeviceID device_id;
    SDL_AudioSpec spec;
    bool playing;
    int sample_index;
} Sound;

void sound_init(Sound *sound);
void sound_play(Sound *sound, bool play_note);
void sound_destroy(Sound *sound);

#endif // SOUND_H
