#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "chip8.h"
#include "texture.h"
#include "sound.h"
#include "cimgui.h"

// Lista dinâmica de ROMs
typedef struct
{
    char path[512];
    char name[128];
} RomEntry;

// Key-Mapping: associoa SDL_Keycode ao Keypad CHIP-8 (0x0 - 0xF)
typedef struct
{
    SDL_Keycode key; //
    uint8_t chip8;   //
} KeyMap;

/*
 * Layout QWERTY:    Keypad CHIP-8:
 * 1 2 3 4           1 2 3 C
 * Q W E R    ->     4 5 6 D
 * A S D F           7 8 9 E
 * Z X C V           A 0 B F
 */
static KeyMap key_map[16] = {
    {SDLK_1, 0x1},
    {SDLK_2, 0x2},
    {SDLK_3, 0x3},
    {SDLK_4, 0xC},
    {SDLK_q, 0x4},
    {SDLK_w, 0x5},
    {SDLK_e, 0x6},
    {SDLK_r, 0xD},
    {SDLK_a, 0x7},
    {SDLK_s, 0x8},
    {SDLK_d, 0x9},
    {SDLK_f, 0xE},
    {SDLK_z, 0xA},
    {SDLK_x, 0x0},
    {SDLK_c, 0xB},
    {SDLK_v, 0xF},
};

// Lista de ROMs
static RomEntry *roms = NULL;
static size_t rom_count = 0;
static size_t rom_capacity = 0;

// Slot de keys do chip8 aguardanda remapeamento
static int remapping_slot = -1;

// Retorna o slot CHIP-8 para uma tecla SDL.
// Retorna 0xFF se não mapeado
static uint8_t map_sdl_key(SDL_Keycode key)
{
    for (int i = 0; i < 16; i++)
        if (key_map[i].key == key)
            return key_map[i].chip8;
    return 0xFF;
}

// Lista de ROMs
static bool add_rom_entry(const char *path, const char *name)
{
    if (!path || !name)
        return false;
    if (rom_count == rom_capacity)
    {
        size_t new_cap = rom_capacity ? rom_capacity * 2 : 8;
        if (new_cap > SIZE_MAX / sizeof(RomEntry))
            return false;
        RomEntry *tmp = (RomEntry *)realloc(roms, new_cap * sizeof(RomEntry));
        if (!tmp)
            return false;
        roms = tmp;
        rom_capacity = new_cap;
    }
    //
    strncpy(roms[rom_count].path, path, sizeof(roms[rom_count].path) - 1);
    roms[rom_count].path[sizeof(roms[rom_count].path) - 1] = '\0';
    //
    strncpy(roms[rom_count].name, name, sizeof(roms[rom_count].name) - 1);
    roms[rom_count].name[sizeof(roms[rom_count].name) - 1] = '\0';

    rom_count++;
    return true;
}

// Libera memória
static void free_rom_list(void)
{
    free(roms);
    roms = NULL;
    rom_count = rom_capacity = 0;
}

// Scaneia o diretório "roms"
static bool scan_roms(const char *directory)
{
    free_rom_list();
    DIR *dir = opendir(directory);
    if (!dir)
        return false;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        size_t len = strlen(entry->d_name);
        if (len <= 4)
            continue;
        if (strcasecmp(entry->d_name + len - 4, ".ch8") == 0)
        {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
            add_rom_entry(full_path, entry->d_name);
        }
    }
    closedir(dir);
    return rom_count > 0;
}

// Carregamento de ROMs
static bool load_rom_to_chip(Chip8 *chip8, const char *path, char *current_path, size_t max_len)
{
    uint8_t *buffer = NULL;
    int32_t size = 0;
    if (!chip8_load_rom(path, &buffer, &size))
        return false;
    chip8_boot(chip8, buffer, size);
    if (size > (4096 - 512)) // 3,584 bytes for the actual ROM
    {
        fprintf(stderr, "ROM too large for CHIP-8 memory\n");
        free(buffer);
        return false;
    }
    if (current_path)
    {
        strncpy(current_path, path, max_len - 1);
        current_path[max_len - 1] = '\0';
    }
    return true;
}

// Inicialização do SDL
static SDL_Window *initialize_sdl(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_Window *window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        660, 940,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!window)
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return window;
}

// Função MAIN
int main(int argc, char *argv[])
{
    // Carregando a Janela SDL
    SDL_Window *window = initialize_sdl();
    if (!window)
        return false;

    // Cria um contexto para uma janela com openGL
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Estrutura pricipal da CPU
    Chip8 chip8;
    chip8_init(&chip8);

    // SOM
    Sound sound;
    sound_init(&sound);

    // ROMs
    const char *rom_directory = "../Debug/roms";
    scan_roms(rom_directory);

    /* Carregamento incial da ROM */
    char current_rom_path[512] = "";
    int selected_rom = 0;

    if (argc > 1)
    {
        strncpy(current_rom_path, argv[1], sizeof(current_rom_path) - 1);
        selected_rom = -1;
    }
    else if (rom_count > 0)
    {
        strncpy(current_rom_path, roms[0].path, sizeof(current_rom_path) - 1);
        selected_rom = 0;
    }

    if (current_rom_path[0] == '\0' ||
        !load_rom_to_chip(&chip8, current_rom_path,
                          current_rom_path, sizeof(current_rom_path)))
    {
        fprintf(stderr, "Erro ao carregar ROM.\n");
    }

    // Textura com o openGL
    Texture texture;
    texture_init(&texture, CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT, chip8.display);

    // Inicia o imgui
    cimgui_init(window, gl_context);

    /* Variáveis do Loop */
    Uint32 last_timer_tick = SDL_GetTicks();
    const Uint32 timer_interval = 1000 / 60;

    bool running = true;
    bool paused = false;
    bool step_requested = false;
    bool show_keymap = false;
    float step_accum = 0.0f;
    int emulation_speed = 600;

    /* Loop Principal */

    while (running)
    {
        // Eventos
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;

            // Quando aguardando remapeamento, captura a próxima tecla
            // ANTES de repassar ao ImGui, para não acionar atalhos.

            if (remapping_slot >= 0 && event.type == SDL_KEYDOWN)
            {
                SDL_Keycode new_key = event.key.keysym.sym;

                // Remove conflito: limpa o mesmo keycode em outro slot
                for (int i = 0; i < 16; i++)
                    if (i != remapping_slot && key_map[i].key == new_key)
                        key_map[i].key = SDLK_UNKNOWN;

                key_map[remapping_slot].key = new_key;
                remapping_slot = -1;
                continue; // não envia ao emulador nem ao ImGui
            }

            // Repassa o evento ao ImGui (mouse, scroll, texto, etc.)
            // mas NÃO usa continue: as teclas do CHIP-8 devem ser
            // processadas independente de o ImGui ter "capturado" o evento.
            cimgui_process_event(&event);

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                else
                {
                    uint8_t mapped = map_sdl_key(event.key.keysym.sym);
                    if (mapped != 0xFF)
                        chip8.keypad[mapped] = 1;
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                uint8_t mapped = map_sdl_key(event.key.keysym.sym);
                if (mapped != 0xFF)
                    chip8.keypad[mapped] = 0;
            }
        }

        /* Emulação */
        if (!paused)
        {
            step_accum += emulation_speed / 60.0f;
            while (step_accum >= 1.0f)
            {
                chip8_step(&chip8);
                step_accum -= 1.0f;
            }
        }
        else if (step_requested)
        {
            chip8_step(&chip8);
            step_requested = false;
        }

        /* Timers 60Hz */
        Uint32 current_ticks = SDL_GetTicks();
        if (current_ticks - last_timer_tick >= timer_interval)
        {
            if (chip8.delay_timer > 0)
                chip8.delay_timer--;
            if (chip8.sound_timer > 0)
                chip8.sound_timer--;
            last_timer_tick += timer_interval;
        }
        sound_play(&sound, chip8.sound_timer > 0);

        /* Renderização */
        texture_update(&texture, chip8.display);

        cimgui_new_frame();
        cimgui_begin("CHIP-8 Emulator", 0.0f, 0.0f, 660.0f, 940.0f);

        /* ===================================================== */
        // SEÇÃO 1 — ROMs disponíveis
        cimgui_separator();
        cimgui_text("[ ROMs disponíveis ]");
        cimgui_separator();

        for (size_t i = 0; i < rom_count; i++)
        {
            /* Botão "Load" com ID único por índice */
            char btn_lbl[32];
            snprintf(btn_lbl, sizeof(btn_lbl), "Load##%zu", i);

            if (cimgui_button(btn_lbl))
            {
                selected_rom = (int)i;
                load_rom_to_chip(&chip8, roms[i].path,
                                 current_rom_path, sizeof(current_rom_path));
            }
            cimgui_same_line();

            cimgui_text("%s%s",
                        roms[i].name,
                        selected_rom == (int)i ? "  <-- atual" : "");
        }

        /* ===================================================== */
        // SEÇÃO 2 — Controles de emulação
        cimgui_separator();
        cimgui_text("[ Controles ]");
        cimgui_separator();

        if (cimgui_button("Reset"))
            load_rom_to_chip(&chip8, current_rom_path,
                             current_rom_path, sizeof(current_rom_path));

        cimgui_same_line();

        if (cimgui_button(paused ? "Resume" : "Pause"))
            paused = !paused;

        cimgui_same_line();

        /* Step só tem efeito enquanto pausado */
        if (cimgui_button("Step") && paused)
            step_requested = true;

        cimgui_separator();
        cimgui_text("Velocidade (inst/s):");
        cimgui_slider_int("##speed", &emulation_speed, 60, 3000);

        cimgui_separator();
        cimgui_text("Quirks de compatibilidade:");
        cimgui_checkbox("Shift usa VY (CHIP-48/S-CHIP)", &chip8.shift_using_VY);
        cimgui_checkbox("Incrementa I em LD Vx/Vy", &chip8.increment_I_on_ld);

        /* =====================================================*/
        // SEÇÃO 3 — Estado da CPU
        cimgui_separator();
        cimgui_text("[ CPU  |  %s ]", paused ? "PAUSADO" : "RODANDO");
        cimgui_separator();

        cimgui_text("PC: 0x%04X    I: 0x%04X    SP: %u",
                    chip8.pc, chip8.I, chip8.sp);
        cimgui_text("DT: %-3u    ST: %-3u",
                    chip8.delay_timer, chip8.sound_timer);

        /* Registradores V0-VF em duas linhas de 8 */
        cimgui_text("V0:%02X V1:%02X V2:%02X V3:%02X V4:%02X V5:%02X V6:%02X V7:%02X",
                    chip8.V[0], chip8.V[1], chip8.V[2], chip8.V[3],
                    chip8.V[4], chip8.V[5], chip8.V[6], chip8.V[7]);
        cimgui_text("V8:%02X V9:%02X VA:%02X VB:%02X VC:%02X VD:%02X VE:%02X VF:%02X",
                    chip8.V[8], chip8.V[9], chip8.V[10], chip8.V[11],
                    chip8.V[12], chip8.V[13], chip8.V[14], chip8.V[15]);

        /* Pilha em duas linhas de 8 slots */
        cimgui_text("Pilha (SP=%u):", chip8.sp);
        cimgui_text("[0]%04X [1]%04X [2]%04X [3]%04X [4]%04X [5]%04X [6]%04X [7]%04X",
                    chip8.stack[0], chip8.stack[1], chip8.stack[2], chip8.stack[3],
                    chip8.stack[4], chip8.stack[5], chip8.stack[6], chip8.stack[7]);
        cimgui_text("[8]%04X [9]%04X [A]%04X [B]%04X [C]%04X [D]%04X [E]%04X [F]%04X",
                    chip8.stack[8], chip8.stack[9], chip8.stack[10], chip8.stack[11],
                    chip8.stack[12], chip8.stack[13], chip8.stack[14], chip8.stack[15]);

        /* ===================================================== */
        // SEÇÃO 4 — Key Mapping
        cimgui_separator();

        if (cimgui_button(show_keymap ? "[ Key Mapping - ocultar ]"
                                      : "[ Key Mapping - exibir  ]"))
            show_keymap = !show_keymap;

        if (show_keymap)
        {
            cimgui_separator();

            if (remapping_slot >= 0)
                cimgui_text(">> Pressione a nova tecla para o slot CHIP-8 [%X] <<",
                            key_map[remapping_slot].chip8);
            else
                cimgui_text("Clique em um slot para remapear a tecla:");

            /*
             * Grade 4×4 — espelha o keypad fisico do CHIP-8:
             *   [1][2][3][C]
             *   [4][5][6][D]
             *   [7][8][9][E]
             *   [A][0][B][F]
             *
             * grid_order[i] = índice em key_map[] para a célula i
             * (key_map[] já está na mesma ordem, então é 0..15 direto)
             */
            static const int grid_order[16] = {
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                9,
                10,
                11,
                12,
                13,
                14,
                15,
            };

            for (int row = 0; row < 4; row++)
            {
                for (int col = 0; col < 4; col++)
                {
                    int idx = grid_order[row * 4 + col];
                    KeyMap *km = &key_map[idx];
                    bool waiting = (remapping_slot == idx);

                    /*
                     * Rótulo: "C8:X | KeyName" (ou "..." ao aguardar)
                     * O sufixo "##kmN" garante IDs únicos ao ImGui.
                     */
                    char btn_lbl[48];
                    snprintf(btn_lbl, sizeof(btn_lbl),
                             "C8:%X | %s##km%d",
                             km->chip8,
                             waiting ? "..." : SDL_GetKeyName(km->key),
                             idx);

                    if (cimgui_button(btn_lbl))
                        remapping_slot = (remapping_slot == idx) ? -1 : idx;

                    if (col < 3)
                        cimgui_same_line();
                }
            }

            cimgui_separator();

            if (cimgui_button("Restaurar mapeamento padrao"))
            {
                static const SDL_Keycode defaults[16] = {
                    SDLK_1,
                    SDLK_2,
                    SDLK_3,
                    SDLK_4,
                    SDLK_q,
                    SDLK_w,
                    SDLK_e,
                    SDLK_r,
                    SDLK_a,
                    SDLK_s,
                    SDLK_d,
                    SDLK_f,
                    SDLK_z,
                    SDLK_x,
                    SDLK_c,
                    SDLK_v,
                };
                for (int i = 0; i < 16; i++)
                    key_map[i].key = defaults[i];
                remapping_slot = -1;
            }
        }

        /* ===================================================== */
        // SEÇÃO 5 — Display CHIP-8
        cimgui_separator();
        cimgui_text("[ Display %dx%d ]",
                    CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT);
        cimgui_separator();

        cimgui_image((void *)(intptr_t)texture.tex_id,
                     CHIP8_DISPLAY_WIDTH * 10.0f,
                     CHIP8_DISPLAY_HEIGHT * 10.0f);

        cimgui_end();
        cimgui_render();

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    // Limpeza
    texture_destroy(&texture);
    sound_destroy(&sound);
    chip8_destroy(&chip8);
    free_rom_list();
    cimgui_shutdown();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
