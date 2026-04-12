#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL2/SDL_opengl.h>
#include <stdbool.h>

typedef struct
{
    GLuint tex_id;
    int width;
    int height;
} Texture;

void texture_init(Texture *texture, int width, int height, const void *pixels);
bool texture_update(Texture *texture, const void *pixels);
void texture_render(const Texture *texture, float x, float y, float scale);
void texture_destroy(Texture *texture);

#endif // TEXTURE_H
