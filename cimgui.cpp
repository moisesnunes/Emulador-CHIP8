#include "cimgui.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl2.h"
#include <stdarg.h>
#include <stdio.h>

void cimgui_init(SDL_Window *window, SDL_GLContext gl_context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsClassic();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();
}

void cimgui_shutdown(void)
{
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void cimgui_new_frame(void)
{
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void cimgui_render(void)
{
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void cimgui_begin(const char *name, float x, float y, float width, float height)
{
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);
    ImGui::Begin(name, NULL,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
}

void cimgui_end(void)
{
    ImGui::End();
}

bool cimgui_button(const char *label)
{
    return ImGui::Button(label);
}

bool cimgui_checkbox(const char *label, bool *value)
{
    return ImGui::Checkbox(label, value);
}

bool cimgui_slider_int(const char *label, int *value, int min_val, int max_val)
{
    return ImGui::SliderInt(label, value, min_val, max_val);
}

bool cimgui_process_event(const SDL_Event *event)
{
    return ImGui_ImplSDL2_ProcessEvent(event);
}

void cimgui_text(const char *fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ImGui::Text("%s", buffer);
}

void cimgui_separator(void)
{
    ImGui::Separator();
}

void cimgui_same_line(void)
{
    ImGui::SameLine();
}

void cimgui_image(void *texture_id, float width, float height)
{
    ImGui::Image(texture_id, ImVec2(width, height));
}
