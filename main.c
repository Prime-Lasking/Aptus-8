#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>

// Instruction mnemonics to opcode mapping
typedef struct {
    const char *name;
    uint8_t opcode;
    int operands;  // Number of operands this instruction expects
} InstructionDef;

static const InstructionDef instructions[] = {
    {"mov", 0x01, 2},  // mov reg, imm
    {"add", 0x10, 0},  // ADD
    {"sub", 0x11, 0},  // SUB
    {"jmp", 0x20, 2},  // JMP addr (2-byte address)
    {"sta", 0x30, 1},  // STA addr
    {"stb", 0x31, 1},  // STB addr
    {"stc", 0x32, 1},  // STC addr
    {"print", 0x40, 1}, // PRINT addr
    {"halt", 0xFF, 0},  // HALT
    {NULL, 0, 0}  // Terminator
};

// Register codes
enum {
    REG_A = 0,
    REG_B = 1,
    REG_C = 2
};

// ---------------------------
// CPU Definition
// ---------------------------

typedef struct {
    uint8_t  A;         // accumulator
    uint8_t  B;
    uint8_t  C;
    uint16_t PC;        // program counter
    uint64_t cycles;    // cycle counter for timing
} CPU;

// Global 64 KB RAM
uint8_t RAM[65536];

// ---------------------------
// Memory Helpers
// ---------------------------

uint8_t mem_read(uint16_t addr) {
    return RAM[addr];
}

void mem_write(uint16_t addr, uint8_t val) {
    RAM[addr] = val;
}

// ---------------------------
// Program Loader & Assembler
// ---------------------------

bool load_program_from_file(const char *path, uint16_t start, size_t *out_size) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Unable to open '%s': %s\n", path, strerror(errno));
        return false;
    }
    
    size_t max_bytes = (start < sizeof(RAM)) ? sizeof(RAM) - start : 0;
    if (max_bytes == 0) {
        fprintf(stderr, "Error: Start address 0x%04X is outside RAM range\n", start);
        fclose(file);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size_l = ftell(file);
    if (file_size_l < 0) {
        fclose(file);
        return false;
    }
    size_t file_size = (size_t)file_size_l;
    rewind(file);

    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return false;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = 0;
    fclose(file);

    size_t token_count = 0;
    uint8_t *text_bytes = malloc(max_bytes);
    bool parsed_as_text = true;

    char *cursor = buffer;
    int in_comment = 0;

    while (*cursor) {
        while (*cursor && isspace((unsigned char)*cursor)) cursor++;
        if (!*cursor) break;

        // Comments
        if (!in_comment && cursor[0]=='/' && cursor[1]=='/') {
            while (*cursor && *cursor!='\n') cursor++;
            continue;
        }
        if (!in_comment && cursor[0]=='/' && cursor[1]=='*') {
            in_comment = 1;
            cursor += 2;
            continue;
        }
        if (in_comment && cursor[0]=='*' && cursor[1]=='/') {
            in_comment = 0;
            cursor += 2;
            continue;
        }
        if (in_comment) {
            cursor++;
            continue;
        }

        // Parse mnemonic
        char mnemonic[16] = {0};
        const char *start_tok = cursor;
        while (*cursor && !isspace((unsigned char)*cursor) && (cursor - start_tok) < 15) {
            mnemonic[cursor - start_tok] = *cursor;
            cursor++;
        }

        // Lookup instruction
        const InstructionDef *instr = NULL;
        for (const InstructionDef *i = instructions; i->name; i++)
            if (strcasecmp(mnemonic, i->name) == 0) { instr = i; break; }

        if (instr) {
            text_bytes[token_count++] = instr->opcode;

            while (*cursor && isspace((unsigned char)*cursor)) cursor++;

            // ---- operand handling (patched) ----
            for (int op = 0; op < instr->operands; op++) {
                if (!*cursor) { parsed_as_text = false; break; }

                if (isalpha((unsigned char)*cursor)) {
                    char r = tolower(*cursor);
                    uint8_t regcode;
                    if (r == 'a') regcode = REG_A;
                    else if (r == 'b') regcode = REG_B;
                    else if (r == 'c') regcode = REG_C;
                    else { parsed_as_text = false; break; }

                    text_bytes[token_count++] = regcode;
                    cursor++;
                } else {
                    char *next;
                    long val = strtol(cursor, &next, 0);
                    if (next == cursor || val < 0 || val > 0xFF) { parsed_as_text = false; break; }
                    text_bytes[token_count++] = (uint8_t)val;
                    cursor = next;
                }

                while (*cursor && isspace((unsigned char)*cursor)) cursor++;
                if (*cursor == ',') { cursor++; while (isspace((unsigned char)*cursor)) cursor++; }
            }
        } else {
            parsed_as_text = false;
            break;
        }
    }

    size_t loaded = 0;

    if (parsed_as_text) {
        memcpy(&RAM[start], text_bytes, token_count);
        loaded = token_count;
    } else {
        memcpy(&RAM[start], buffer, file_size);
        loaded = file_size;
    }

    free(buffer);
    free(text_bytes);

    if (out_size) *out_size = loaded;
    return true;
}

// ---------------------------
// Instruction Execution
// ---------------------------

void execute(CPU *cpu) {
    uint8_t opcode = mem_read(cpu->PC++);

    switch (opcode) {

    // MOV reg, imm  (patched)
    case 0x01: {
        uint8_t reg = mem_read(cpu->PC++);
        uint8_t value = mem_read(cpu->PC++);
        switch (reg) {
            case REG_A: cpu->A = value; break;
            case REG_B: cpu->B = value; break;
            case REG_C: cpu->C = value; break;
            default:
                fprintf(stderr, "Invalid register %u\n", reg);
                break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x10: cpu->A += cpu->B; cpu->cycles++; break;
    case 0x11: cpu->A -= cpu->B; cpu->cycles++; break;

    case 0x20: {
        uint8_t hi = mem_read(cpu->PC++);
        uint8_t lo = mem_read(cpu->PC++);
        cpu->PC = (hi << 8) | lo;
        cpu->cycles += 3;
        break;
    }

    case 0x30: mem_write(mem_read(cpu->PC++), cpu->A); cpu->cycles += 3; break;
    case 0x31: mem_write(mem_read(cpu->PC++), cpu->B); cpu->cycles += 3; break;
    case 0x32: mem_write(mem_read(cpu->PC++), cpu->C); cpu->cycles += 3; break;

    case 0x40: {
        uint8_t addr = mem_read(cpu->PC++);
        printf("%u\n", mem_read(addr));
        cpu->cycles += 4;
        break;
    }

    case 0xFF:
        printf("System exited at PC=0x%04X after %" PRIu64 " cycles\n",
            cpu->PC - 1, cpu->cycles);
        exit(0);

    default:
        printf("Unknown opcode: 0x%02X at PC=0x%04X\n", opcode, cpu->PC - 1);
        exit(1);
    }
}

// ---------------------------
// Main
// ---------------------------

int main(int argc, char **argv) {
    CPU cpu = {0};
    cpu.PC = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.asm|program.bin>\n", argv[0]);
        return 1;
    }

    size_t program_size = 0;
    if (!load_program_from_file(argv[1], 0x0000, &program_size))
        return 1;

    printf("Loaded '%s' (%zu bytes) into RAM\n", argv[1], program_size);

    while (1)
        execute(&cpu);

    return 0;
}
