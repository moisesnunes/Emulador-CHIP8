#include "texture.h"
#include <SDL2/SDL_opengl.h>
#include <stdlib.h>

void texture_init(Texture *texture, int width, int height, const void *pixels)
{
    if (!texture)
        return;

    texture->width = width;
    texture->height = height;
    texture->tex_id = 0;

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture->tex_id);
    glBindTexture(GL_TEXTURE_2D, texture->tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool texture_update(Texture *texture, const void *pixels)
{
    if (!texture || texture->tex_id == 0)
    {
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture->tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture->width, texture->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void texture_render(const Texture *texture, float x, float y, float scale)
{
    if (!texture || texture->tex_id == 0)
    {
        return;
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    glBindTexture(GL_TEXTURE_2D, texture->tex_id);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f((float)texture->width, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f((float)texture->width, (float)texture->height);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, (float)texture->height);
    glEnd();
}

void texture_destroy(Texture *texture)
{
    if (!texture || texture->tex_id == 0)
    {
        return;
    }
    glDeleteTextures(1, &texture->tex_id);
    texture->tex_id = 0;
}
