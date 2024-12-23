#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

// 6502 CPU structure
typedef struct {
    uint16_t pc;    // Program Counter
    uint8_t sp;     // Stack Pointer
    uint8_t a;      // Accumulator
    uint8_t x;      // X Register
    uint8_t y;      // Y Register
    uint8_t status; // Processor Status
} CPU6502;

// Memory and CPU instance
#define MEMORY_SIZE 0x10000 // 64KB memory
uint8_t memory[MEMORY_SIZE];
CPU6502 cpu;

// Flags in the status register
#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_UNUSED    0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_NEGATIVE  0x80

// SDL variables
SDL_Surface *screen;

// Apple II text mode constants
#define TEXT_COLS 40
#define TEXT_ROWS 24
#define CHAR_WIDTH 7
#define CHAR_HEIGHT 8

// Basic 7x8 bitmap font (simplified ASCII, characters 0x20 to 0x5F)
uint8_t font[96][8] = {
    // Space (0x20)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // !
    {0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00},
    // "
    {0x28, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00},
    // #
    {0x28, 0x28, 0x7C, 0x28, 0x7C, 0x28, 0x28, 0x00},
    // Letters A-Z
    {0x38, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x00}, // A
    {0x78, 0x44, 0x44, 0x78, 0x44, 0x44, 0x78, 0x00}, // B
    {0x38, 0x44, 0x40, 0x40, 0x40, 0x44, 0x38, 0x00}, // C
    {0x70, 0x48, 0x44, 0x44, 0x44, 0x48, 0x70, 0x00}, // D
    {0x7C, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7C, 0x00}, // E
    {0x7C, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40, 0x00}, // F
    {0x38, 0x44, 0x40, 0x5C, 0x44, 0x44, 0x38, 0x00}, // G
    {0x44, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x00}, // H
    {0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00}, // I
    {0x1C, 0x08, 0x08, 0x08, 0x08, 0x48, 0x30, 0x00}, // J
    {0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44, 0x00}, // K
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7C, 0x00}, // L
    {0x44, 0x6C, 0x6C, 0x54, 0x44, 0x44, 0x44, 0x00}, // M
    {0x44, 0x64, 0x64, 0x54, 0x4C, 0x4C, 0x44, 0x00}, // N
    {0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00}, // O
    {0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x40, 0x00}, // P
    {0x38, 0x44, 0x44, 0x44, 0x54, 0x48, 0x34, 0x00}, // Q
    {0x78, 0x44, 0x44, 0x78, 0x48, 0x44, 0x44, 0x00}, // R
    {0x38, 0x44, 0x40, 0x38, 0x04, 0x44, 0x38, 0x00}, // S
    {0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00}, // T
    {0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00}, // U
    {0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x10, 0x00}, // V
    {0x44, 0x44, 0x44, 0x54, 0x54, 0x6C, 0x44, 0x00}, // W
    {0x44, 0x44, 0x28, 0x10, 0x28, 0x44, 0x44, 0x00}, // X
    {0x44, 0x44, 0x28, 0x10, 0x10, 0x10, 0x10, 0x00}, // Y
    {0x7C, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7C, 0x00}, // Z
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // whatever this is..
    // ~
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

// Floppy disk structure
#define SECTOR_SIZE 0x100
#define TRACKS 35
#define SECTORS_PER_TRACK 16

typedef struct {
    uint8_t *data; // Raw floppy disk data
    size_t size;   // Size of the disk image
} FloppyDisk;

FloppyDisk floppy;

// Initialize SDL
void init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    screen = SDL_SetVideoMode(TEXT_COLS * CHAR_WIDTH, TEXT_ROWS * CHAR_HEIGHT, 32, SDL_SWSURFACE);
    if (!screen) {
        fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_WM_SetCaption("Apple II Emulator", NULL);
}

// Draw a single character
void draw_char(uint8_t ch, int x, int y) {
    if (ch < 0x20 || ch > 0x7F) return;
    ch -= 0x20; // Adjust index to match font array

    for (int row = 0; row < CHAR_HEIGHT; row++) {
        for (int col = 0; col < CHAR_WIDTH; col++) {
            uint8_t pixel_on = font[ch][row] & (1 << (6 - col));
            uint32_t color = pixel_on ? 0xFFFFFFFF : 0x00000000;
            int px = x + col;
            int py = y + row;

            if (px >= 0 && px < TEXT_COLS * CHAR_WIDTH && py >= 0 && py < TEXT_ROWS * CHAR_HEIGHT) {
                ((uint32_t *)screen->pixels)[py * screen->w + px] = color;
            }
        }
    }
}

// Render the screen for text mode
void render_screen() {
    if (!screen) {
        fprintf(stderr, "Screen is not initialized.\n");
        return;
    }

    if (SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0) {
        fprintf(stderr, "Unable to lock screen: %s\n", SDL_GetError());
        return;
    }

    memset(screen->pixels, 0, screen->w * screen->h * sizeof(uint32_t));

    for (int row = 0; row < TEXT_ROWS; row++) {
        for (int col = 0; col < TEXT_COLS; col++) {
            uint16_t addr = 0x0400 + row * TEXT_COLS + col;
            uint8_t ch = memory[addr];

            if (ch < 0x20 || ch > 0x7F) {
                ch = 0x41; // Default to 'A' for invalid characters
            }

            draw_char(ch, col * CHAR_WIDTH, row * CHAR_HEIGHT);
        }
    }

    if (SDL_MUSTLOCK(screen)) {
        SDL_UnlockSurface(screen);
    }

    if (SDL_Flip(screen) < 0) {
        fprintf(stderr, "SDL_Flip failed: %s\n", SDL_GetError());
    }
}

// Handle input
void handle_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            printf("Quit event received. Exiting.\n");
            SDL_Quit();
            exit(0);
        }
    }
}

// Load ROM into memory
void load_rom(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open ROM file");
        exit(1);
    }
    fread(&memory[0xD000], 1, 0x3000, file); // Load ROM at $D000-$FFFF
    fclose(file);
    for (int i = 0xD000; i <= 0xD0FF; i++) {
    printf("ROM[%04X] = %02X\n", i, memory[i]);
        }
}

// Reset CPU
void reset_cpu() {
    cpu.pc = (memory[0xFFFC] | (memory[0xFFFD] << 8)); // Reset vector at $FFFC-$FFFD
    cpu.sp = 0xFF;
    cpu.a = 0;
    cpu.x = 0;
    cpu.y = 0;
    cpu.status = FLAG_UNUSED;
}

// Push a byte onto the stack
void push_byte(uint8_t value) {
    memory[0x0100 + cpu.sp--] = value;
}

// Fetch a byte from memory
uint8_t fetch_byte() {
    return memory[cpu.pc++];
}

// Fetch a word (2 bytes) from memory
uint16_t fetch_word() {
    uint16_t low = fetch_byte();
    uint16_t high = fetch_byte();
    return low | (high << 8);
}

// Execute a single instruction (VERY BASIC..)
void execute_instruction() {
    uint8_t opcode = fetch_byte();

    switch (opcode) {
        case 0xA9: // LDA Immediate
            cpu.a = fetch_byte();
            cpu.status = (cpu.a == 0 ? FLAG_ZERO : 0) | (cpu.a & 0x80 ? FLAG_NEGATIVE : 0);
            break;

        case 0x4C: // JMP Absolute
        {
            uint16_t address = fetch_word();
            cpu.pc = address;
            break;
        }

        case 0x00: // BRK
        {
            push_byte((cpu.pc >> 8) & 0xFF);
            push_byte(cpu.pc & 0xFF);
            push_byte(cpu.status | FLAG_BREAK);
            cpu.status |= FLAG_INTERRUPT;
            cpu.pc = memory[0xFFFE] | (memory[0xFFFF] << 8);
            break;
        }

        default:
            printf("Unsupported opcode: %02X\n", opcode);
            exit(1);
    }
}

// Run the emulator
void run() {
    while (1) {
        execute_instruction();
        handle_input();
        render_screen();
        SDL_Delay(16); // Limit to ~60 FPS
    }
}

// Function to load a floppy disk image
void load_floppy(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open floppy disk image");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    floppy.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    floppy.data = (uint8_t *)malloc(floppy.size);
    if (!floppy.data) {
        perror("Failed to allocate memory for floppy disk");
        exit(1);
    }

    fread(floppy.data, 1, floppy.size, file);
    fclose(file);
}

// Function to read a sector from the floppy disk
void read_sector(uint8_t *buffer, uint8_t track, uint8_t sector) {
    uint32_t sector_start = (track * SECTORS_PER_TRACK + sector) * SECTOR_SIZE;
    if (sector_start + SECTOR_SIZE > floppy.size) {
        printf("Error: Trying to read past the end of the floppy disk.\n");
        exit(1);
    }

    memcpy(buffer, floppy.data + sector_start, SECTOR_SIZE);
}

// Function to boot from floppy (simplified)
void boot_from_floppy() {
    uint8_t boot_sector[SECTOR_SIZE];
    read_sector(boot_sector, 0, 0); // Read the first sector (boot sector)
    uint16_t boot_address = boot_sector[0x1FF] | (boot_sector[0x1FE] << 8);
    cpu.pc = boot_address;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom-file> [floppy-image]\n", argv[0]);
        return 1;
    }

    init_sdl();
    load_rom(argv[1]);

    if (argc > 2) {
        load_floppy(argv[2]);
        boot_from_floppy(); // Boot from floppy if provided
    }

    reset_cpu();
    run();

    return 0;
}
