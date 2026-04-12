#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

// Definições do Hardware do CHIP8
#define CHIP8_MEMORY_SIZE 4096  // Memória RAM total de 4kb
#define CHIP8_DISPLAY_WIDTH 64  // Largura da Tela
#define CHIP8_DISPLAY_HEIGHT 32 // Altura da Tela
#define CHIP8_ROM_START 0x200   // Endereço de memória onde os programas (ROMs) começam

/* --- Macros para Decodificação de Instruções(Opcodes) --- */
// O CHIP-8 usa instruções de 16 bits (ex: 0xANNN). Estas macros extraem partes específicas:
#define OPCODE_NNN(op) ((op) & 0x0fff)      // Extrai os 12 bits inferiores (endereço de memória)
#define OPCODE_NN(op) ((op) & 0x00ff)       // Extrai os 8 bits inferiores (valor constante)
#define OPCODE_N(op) ((op) & 0x000f)        // Extrai o último nibble (4 bits)
#define OPCODE_X(op) (((op) & 0x0f00) >> 8) // Extrai o nibble X (registrador destino)
#define OPCODE_Y(op) (((op) & 0x00f0) >> 4) // Extrai o nibble Y (registrador origem)

/* --- Definições de Cores (Formato ARGB/RGBA) --- */
#define CHIP8_PIXEL_OFF 0xFFFFFFFFU // Cor de fundo (pixel desligado)
#define CHIP8_PIXEL_ON 0x0a0a0a0aU  // Cor do desenho (pixel ligado)

// Estrutura principal da CPU
typedef struct
{
    uint8_t memory[CHIP8_MEMORY_SIZE]; // Memória RAM Original (4096 bytes)
    uint8_t V[16];                     // 16 registradores de uso geral de 8 bits (V0 a VF)
    int16_t I;                         // Registrador de índice (usado para armazenar endereços de memória)
    int16_t stack[16];                 // Pilha para chamadas de sub-rotinas
    int16_t sp;                        // Stack Pointer (ponteiro da pilha), aponta para o nivel mais alto na pilha
    int16_t pc;                        // Program Counter (aponta para a instrução atualemente em uso)

    uint8_t delay_timer; // Timer de atroso (drecementa a 60Hz), quando o valor é non-zero
    uint8_t sound_timer; // Timer de som (emite som quando > 0)

    uint16_t keypad[16]; // Teclado Hexadecimal com 16-key
    uint32_t *display;   // Buffer de renderização da tela (pixels 32-bit)

    uint8_t key_pressed_flag;  /* 1 while we're in Fx0A wait */
    uint8_t key_pressed_value; /* which key we detected */

    // Flags de Comportamento (Quirks)
    // Estas flags garantem compatibilidade com diferentes versões históricas do CHIP-8
    bool shift_using_VY;    // Define se as instruções de SHIFT usam o registrador VY
    bool increment_I_on_ld; // Define se o registrador I deve ser incrementado em operações de carga

} Chip8;

/* --- Funções de Controle --- */

// Inicializa a estrutura, limpa memória e define valores padrão
void chip8_init(Chip8 *chip8);

// Carrega o programa (ROM) para a memória do sistema
void chip8_boot(Chip8 *chip8, const uint8_t *program, int32_t length);

// Executa um único ciclo de instrução (Fetch, Decode, Execute)
void chip8_step(Chip8 *chip8);

// Libera memória alocada (como o buffer do display)
void chip8_destroy(Chip8 *chip8);

// Função auxiliar para carregar o arquivo da ROM do disco para um buffer
int chip8_load_rom(const char *fname, uint8_t **buffer, int32_t *size);

// TODO: Ainda não está implementado de forma correta
// Gerenciamento de teclas:
void chip8_set_key(Chip8 *chip8, uint8_t key);
void chip8_clear_key(Chip8 *chip8, uint8_t key);

#endif
