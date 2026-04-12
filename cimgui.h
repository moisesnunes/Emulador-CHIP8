#ifndef CIMGUI_H
#define CIMGUI_H

#include <SDL2/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void cimgui_init(SDL_Window *window, SDL_GLContext gl_context);
    void cimgui_shutdown(void);
    void cimgui_new_frame(void);
    void cimgui_render(void);
    void cimgui_begin(const char *name, float x, float y, float width, float height);
    void cimgui_end(void);
    bool cimgui_button(const char *label);
    bool cimgui_process_event(const SDL_Event *event);
    bool cimgui_checkbox(const char *label, bool *value);
    bool cimgui_slider_int(const char *label, int *value, int min_val, int max_val);
    void cimgui_text(const char *fmt, ...);
    void cimgui_separator(void);
    void cimgui_same_line(void);
    void cimgui_image(void *texture_id, float width, float height);

#ifdef __cplusplus
}
#endif

#endif // CIMGUI_H
