#include "sound.h"
#include <string.h>
#include <stdlib.h>

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    Sound *sound = (Sound *)userdata;
    if (!sound)
    {
        return;
    }

    int16_t *buffer = (int16_t *)stream;
    int samples = len / sizeof(int16_t);

    for (int i = 0; i < samples; ++i)
    {
        int16_t value = 0;
        if (sound->playing)
        {
            int period = 48000 / 440;
            int half = period / 2;
            value = ((sound->sample_index++ / half) % 2) ? 3000 : -3000;
        }
        buffer[i] = value;
    }
}

void sound_init(Sound *sound)
{
    if (!sound)
    {
        return;
    }

    memset(sound, 0, sizeof(*sound));

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = 48000;
    desired.format = AUDIO_S16LSB;
    desired.channels = 1;
    desired.samples = 1024;
    desired.callback = audio_callback;
    desired.userdata = sound;

    sound->device_id = SDL_OpenAudioDevice(NULL, 0, &desired, &sound->spec, 0);
    if (sound->device_id == 0)
    {
        return;
    }

    SDL_PauseAudioDevice(sound->device_id, 0);
}

void sound_play(Sound *sound, bool play_note)
{
    if (!sound || sound->device_id == 0)
    {
        return;
    }

    sound->playing = play_note;
}

void sound_destroy(Sound *sound)
{
    if (!sound || sound->device_id == 0)
    {
        return;
    }
    SDL_CloseAudioDevice(sound->device_id);
    sound->device_id = 0;
}
