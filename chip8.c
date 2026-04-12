#include "chip8.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Conjunto de caracteres (fontset) padrão do CHIP-8.
// Cada caractere (0-F) tem 5 bytes de altura. Eles são carregados no início da memória.
static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/**
 *   Prepara a estrutura do sistema para o primeiro uso.
 * - Zera a memória e registradores.
 * - Define o PC (Program Counter) para o início padrão das ROMs (0x200).
 * - Carrega as fontes na memória e aloca o array do display.
 */

void chip8_init(Chip8 *chip8)
{
    memset(chip8, 0, sizeof(*chip8)); // Estrutura da memória comece do zero
    chip8->pc = CHIP8_ROM_START;      // Endereço de memório onde as roms começam

    chip8->shift_using_VY = false;
    chip8->increment_I_on_ld = false;

    // Copia o fontset para o início da memória RAM (0x000 em diante)
    memcpy(chip8->memory, chip8_fontset, sizeof(chip8_fontset));

    // Aloca memória dinâmica para os pixels do display
    chip8->display = (uint32_t *)malloc(CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT * sizeof(uint32_t));
    // chip8->display = calloc(CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT, sizeof(uint32_t));
    if (chip8->display)
    {
        for (int i = 0; i < CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT; ++i)
        {
            chip8->display[i] = CHIP8_PIXEL_OFF;
        }
    }

    // Inicializa a semente do gerador de números aleatórios
    srand((unsigned int)time(NULL));
}

/**
 *   Libera os recursos alocados dinamicamente.
 * - Evita vazamentos de memória (memory leaks) limpando o display.
 */
void chip8_destroy(Chip8 *chip8)
{
    free(chip8->display);
    chip8->display = NULL;
}

/**
 *   Reseta o estado interno e carrega o programa binário.
 * - Limpa registros, pilha e timers.
 * - Copia o conteúdo da ROM para o endereço de memória correto.
 */
void chip8_boot(Chip8 *chip8, const uint8_t *program, int32_t length)
{
    // Verfica se a ROM cabe na memória disponível
    if (length <= 0 || length > CHIP8_MEMORY_SIZE - CHIP8_ROM_START)
    {
        return;
    }

    // Carregador de ROM
    // Copia o programa(jogo) do buffer para a memória RAM a partir de 0x200
    memcpy(&chip8->memory[CHIP8_ROM_START], program, length);

    // Reseta o estado da CPU para o início da execução
    chip8->pc = CHIP8_ROM_START;
    chip8->sp = -1;         // Pilha vazia
    chip8->I = 0;           // Indexador de memória zerado
    chip8->delay_timer = 0; //
    chip8->sound_timer = 0; //

    memset(chip8->keypad, 0, sizeof(chip8->keypad));
    memset(chip8->V, 0, sizeof(chip8->V));         // Limpa registradores V0-VF
    memset(chip8->stack, 0, sizeof(chip8->stack)); // Limpa a pilha

    // Limpa a tela para começar o programa
    if (chip8->display)
    {
        for (int i = 0; i < CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT; ++i)
        {
            chip8->display[i] = CHIP8_PIXEL_OFF;
        }
    }
}

/**
 *   Executa um único ciclo de instrução (Fetch, Decode, Execute).
 * - Lê a instrução de 2 bytes na memória.
 * - Decodifica os parâmetros (X, Y, NNN, NN, N).
 * - Executa a lógica correspondente ao Opcode.
 */
void chip8_step(Chip8 *chip8)
{
    if (!chip8)
        return;

    // FETCH: Lê dois bytes da memória e combina em um opcode de 16 bits
    uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    chip8->pc += 2; // Avança o contador de programas para a próxima instrução

    // SHR com quirk:
    uint16_t nnn = OPCODE_NNN(opcode); // Endereço de 12 bits
    uint8_t nn = OPCODE_NN(opcode);    // Valor imediato de 8 bits
    uint8_t n = OPCODE_N(opcode);      // Valor imediato de 4 bits
    uint8_t x = OPCODE_X(opcode);      // Registro VX(4 bits)
    uint8_t y = OPCODE_Y(opcode);      // Registro VY(4 bits)

    // EXECUTE: Switch principal baseado no primeiro nibble do opcode
    switch ((opcode & 0xF000) >> 12)
    {
    case 0x0:
        switch (opcode)
        {
        case 0x00E0: // CLS: Limpa o display
            memset(chip8->display, 0, CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT * sizeof(uint32_t));
            break;

        case 0x00EE: // RET: Retorna de uma sub-rotina (desempilha o pc)
            chip8->pc = chip8->stack[chip8->sp];
            chip8->sp -= 1;
            break;
        default:
            printf("ERRO: OPCODE DESCONHECIDO 0x%04x NO ENDEREÇO 0x%08x\n", opcode, chip8->pc);
            break;
        }
        break;
    case 0x1: // JP addr: Salta para o endereço NNN
        chip8->pc = nnn;
        break;

    case 0x2: // CALL addr: Chama sub-rotina (empilha o pc atual e salta)
        if (chip8->sp >= 15)
            break;
        chip8->sp += 1;
        chip8->stack[chip8->sp] = chip8->pc;
        chip8->pc = nnn;
        break;

    case 0x3: // SE Vx: Pula próxima instrução se Vz == nn
        if (chip8->V[x] == nn)
            chip8->pc += 2;
        break;

    case 0x4: // SNE Vx: Pula próxima instrução se Vx != nn
        if (chip8->V[x] != nn)
            chip8->pc += 2;
        break;

    case 0x5: // SE Vx, Vy: Pula próxima instrução se Vx == Vy
        if (n == 0 && chip8->V[x] == chip8->V[y])
            chip8->pc += 2;
        break;

    case 0x6: // LD Vx: Define Vx = nn
        chip8->V[x] = nn;
        break;

    case 0x7: // ADD Vx: Define Vx = Vx + nn
        chip8->V[x] += nn;
        break;

    case 0x8: // Operações Aritméticas e Lógicas entre Vx e Vy
        switch (n)
        {
        case 0x0:
            chip8->V[x] = chip8->V[y];
            break; // LD
        case 0x1:
            chip8->V[x] |= chip8->V[y];
            chip8->V[0xF] = 0;
            break; // OR
        case 0x2:
            chip8->V[x] &= chip8->V[y];
            chip8->V[0xF] = 0;
            break; // AND
        case 0x3:
            chip8->V[x] ^= chip8->V[y];
            chip8->V[0xF] = 0;
            break; // XOR

        case 0x4: // ADD com Carry: VF indica se houve estouro (overflow)
        {
            uint16_t sum = chip8->V[x] + chip8->V[y];
            chip8->V[x] = sum & 0xFF; // (255)
            chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
            break;
        }
        case 0x5: // SUB: VF indica se NÃO houve borrow
        {
            uint8_t xv = chip8->V[x];
            uint8_t yv = chip8->V[y];
            chip8->V[x] = (xv - yv) & 0xFF;
            chip8->V[0xF] = (xv >= yv) ? 1 : 0;
        }
        break;
        case 0x6: // SHR (Shift Right): Desloca bits para direita
        {
            // uint8_t buf = chip8->V[x];
            // chip8->V[x] = buf >> 1;
            // chip8->V[0xF] = buf & 0x1;
            // SHR com quirk:
            uint8_t src = chip8->shift_using_VY ? chip8->V[x] : chip8->V[y];
            chip8->V[0xF] = src & 0x1;
            chip8->V[x] = src >> 1;
        }
        break;

        case 0x7: // SUBN: Vy - Vx, VF indica se NÃO houve borrow
        {
            uint8_t xv = chip8->V[x];
            uint8_t yv = chip8->V[y];
            chip8->V[0xF] = 0;
            chip8->V[x] = (yv - xv);
            chip8->V[0xF] = (yv >= xv) ? 1 : 0;
        }
        break;

        case 0xE: // SHL (Shift Left): Desloca bits para a esquerda
        {
            // uint8_t buf = chip8->V[x];
            // chip8->V[x] = buf << 1;
            // chip8->V[0xF] = (buf >> 7) & 0x1;

            // SHL com quirk:
            uint8_t src = chip8->shift_using_VY ? chip8->V[x] : chip8->V[y];
            chip8->V[0xF] = (src >> 7) & 0x1;
            chip8->V[x] = src << 1;
        }
        break;
        }
        break;

    case 0x9: // SNE Vx, Vy: Pula se Vx != Vy
        if (n == 0 && chip8->V[x] != chip8->V[y])
            chip8->pc += 2;
        break;

    case 0xA: // LD I, addr: Define o registrador de índice I como NNN
        chip8->I = nnn;
        break;

    case 0xB:                          // JP V0, addr: Salta para nnn + V0
        chip8->pc = nnn + chip8->V[0]; // & 0x0FFF;
        // chip8->pc = (uint16_t)(nnn + (uint16_t)chip8->V[0]);
        break;

    case 0xC: // RND Vx: Vx = (Número Aleatório ) AND nn
        chip8->V[x] = (uint8_t)(rand() % 256) & nn;
        break;

    case 0xD: // DRW Vx, Vy, nibble: Desenha sprite na tela
    {
        // Esta instrução é o coração gráfico do CHIP-8 (usa XOR para desenhar)
        chip8->V[0xF] = 0; // VF vira 1 se houver colisão de pixels
        for (uint8_t row = 0; row < n; ++row)
        {
            uint8_t sprite = chip8->memory[chip8->I + row];
            uint8_t posY = (chip8->V[y] + row);
            for (uint8_t col = 0; col < 8; col++)
            {
                uint8_t pixel = (sprite >> (7 - col)) & 0x1;
                uint8_t posX = (chip8->V[x] + col);
                uint32_t index = posY * CHIP8_DISPLAY_WIDTH + posX;
                if (posX >= CHIP8_DISPLAY_WIDTH || posY >= CHIP8_DISPLAY_HEIGHT)
                    continue;
                if (pixel)
                {
                    if (chip8->display[index] != CHIP8_PIXEL_OFF)
                    {
                        chip8->display[index] = CHIP8_PIXEL_OFF;
                        chip8->V[0xF] = 1; // Colisão detectada
                    }
                    else
                    {
                        chip8->display[index] = CHIP8_PIXEL_ON;
                    }
                }
            }
        }
        break;
    }

    // Precisa rever a implementação
    case 0xE:
        if (nn == 0x9E)
        { // SKP Vx: Pula se a tecla em Vx está pressionada
            if (chip8->keypad[chip8->V[x]])
                chip8->pc += 2;
        }
        else if (nn == 0xA1)
        {
            // SKNP Vx: Pula se a tecla em Vx não estiver pressionada
            if (!chip8->keypad[chip8->V[x]])
                chip8->pc += 2;
        }
        break;

    case 0xF: // Diversos (Timers, BCD, Memória)
        switch (nn)
        {
        case 0x07:
            chip8->V[x] = chip8->delay_timer;
            break; // Lê timer de atraso

        case 0x000A:
        { /* Fx0A — LD Vx, K */
            /* Phase 1: wait for a key to be pressed */
            if (!chip8->key_pressed_flag)
            {
                for (int k = 0; k < 16; k++)
                {
                    if (chip8->keypad[k])
                    {
                        chip8->key_pressed_flag = 1;
                        chip8->key_pressed_value = k;
                        break;
                    }
                }
                /* no key yet — re-run this instruction next cycle */
                if (!chip8->key_pressed_flag)
                {
                    chip8->pc -= 2;
                    return;
                }
            }

            /* Phase 2: wait for that same key to be released */
            if (chip8->keypad[chip8->key_pressed_value])
            {
                chip8->pc -= 2; /* still held — keep waiting */
                return;
            }

            /* Key was pressed and has now been released — store it */
            chip8->V[x] = chip8->key_pressed_value;
            chip8->key_pressed_flag = 0;
            chip8->key_pressed_value = 0;
            break;
        }

        case 0x15:
            chip8->delay_timer = chip8->V[x];
            break; // Define timer de atraso

        case 0x18:
            chip8->sound_timer = chip8->V[x];
            break; // Define timer de som

        case 0x1E: // ADD I, Vx: Soma VX ao índice I
            chip8->I += chip8->V[x];
            break;

        case 0x29: // LD F, Vx: Aponta I para o caractere da fonte em VX
            chip8->I = chip8->V[x] * 5;
            break;

        case 0x33: // LD B, Vx: Armazena representação BCD (Decimal) de VX na memória
        {
            uint8_t value = chip8->V[x];
            chip8->memory[chip8->I] = value / 100;           // Centenas
            chip8->memory[chip8->I + 1] = (value / 10) % 10; // Dezenas
            chip8->memory[chip8->I + 2] = value % 10;        // Unidades
            break;
        }
        case 0x55: // LD [I], Vx: Salva V0 até VX na memória a partir de I
            for (uint8_t i = 0; i <= x; i++)
                chip8->memory[i + chip8->I] = chip8->V[i];
            chip8->I += x + 1;
            if (chip8->increment_I_on_ld) // Adicione este toggle na UI ImGui
                chip8->I += x + 1;
            break;

        case 0x65: // LD Vx, [I]: Lê da memória para V0 até VX
            for (uint8_t i = 0; i <= x; i++)
                chip8->V[i] = chip8->memory[i + chip8->I];
            chip8->I += x + 1;
            if (chip8->increment_I_on_ld)
                chip8->I += x + 1;
            break;
        }
        break;
    }
}

/**
 *   Função utilitária para ler o arquivo físico da ROM.
 * - Abre o arquivo no modo binário.
 * - Aloca um buffer do tamanho exato do arquivo.
 * - Lê o conteúdo para o buffer e retorna o tamanho.
 */
int chip8_load_rom(const char *fname, uint8_t **buffer, int32_t *size)
{
    if (!fname || !buffer || !size)
        return 0;

    FILE *file = fopen(fname, "rb");
    if (!file)
    {
        printf("Erro durante o fopen, arquivo %s\n", fname);
        return 0;
    }
    // Vai até o final do arquivo para descobrir o seu tamanho
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET); // Volta ao começo do arquivo

    if (file_size <= 0)
    {
        fclose(file);
        return 0;
    }

    *buffer = (uint8_t *)malloc(file_size);
    if (!*buffer)
    {
        fclose(file);
        return 0;
    }

    // Lê os dados do arquivo para o buffer alocado
    size_t read = fread(*buffer, 1, file_size, file);
    if (read != (size_t)file_size)
    {
        free(*buffer);
        fclose(file);
        return 0;
    }
    *size = (int32_t)file_size;
    return 1;
}

/**
 * chip8_set_key: Atualiza o estado da tecla pressionada no sistema.
 */
void chip8_set_key(Chip8 *chip8, uint8_t key)
{
    if (chip8 && key < 16)
        chip8->keypad[key] = 1;
}

/**
 * chip8_clear_key: Reseta o estado do teclado (nenhuma tecla pressionada).
 */
void chip8_clear_key(Chip8 *chip8, uint8_t key)
{
    if (chip8 && key < 16)
        chip8->keypad[key] = 0;
}
