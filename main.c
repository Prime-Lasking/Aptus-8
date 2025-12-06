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
    {"mov", 0x01, 2},  // mov reg, reg/imm8
    {"add", 0x10, 2},  // add reg, reg/imm8
    {"sub", 0x11, 2},  // sub reg reg/imm8
    {"mul", 0x09, 2},  // mul reg reg/imm8
    {"div", 0x08, 2},  // div reg reg/imm8
    {"and", 0x12, 2},  // and reg reg/imm8
    {"or",  0x13, 2},  // or reg reg/imm8
    {"xor", 0x14, 2},  // xor reg reg/imm8
    {"not", 0x15, 1},  // not reg
    {"nand",0x16, 2},  // nand reg reg/imm8
    {"nor", 0x17, 2},  // nor reg reg/imm8
    {"print", 0x40, 1},// print reg/imm8
    {"halt", 0xFF, 0}, // halt
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
    uint8_t  A;
    uint8_t  B;
    uint8_t  C;
    uint16_t PC;
    uint64_t cycles;
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

void dump_hex(const uint8_t *data, size_t len) {
    printf("Bytecode (%zu bytes):\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len % 16 != 0) printf("\n");
}
int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++; s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}
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

        char mnemonic[16] = {0};
        const char *start_tok = cursor;
        while (*cursor && !isspace((unsigned char)*cursor) && (cursor - start_tok) < 15) {
            mnemonic[cursor - start_tok] = *cursor;
            cursor++;
        }

        const InstructionDef *instr = NULL;
        for (const InstructionDef *i = instructions; i->name; i++)
            if (strcasecmp(mnemonic, i->name) == 0) { instr = i; break; }

        if (instr) {
            text_bytes[token_count++] = instr->opcode;

            while (*cursor && isspace((unsigned char)*cursor)) cursor++;

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
static inline uint8_t read_src_value(CPU *cpu, uint8_t src) {
    switch (src) {
        case REG_A: return cpu->A;
        case REG_B: return cpu->B;
        case REG_C: return cpu->C;
        default:    return src;   // immediate
    }
}
void execute(CPU *cpu) {
    uint8_t opcode = mem_read(cpu->PC++);

    switch (opcode) {

    case 0x01: { // mov
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t value = read_src_value(cpu, src);

        switch (dest_reg) {
            case REG_A: cpu->A = value; break;
            case REG_B: cpu->B = value; break;
            case REG_C: cpu->C = value; break;
            default: fprintf(stderr, "Invalid destination register %u\n", dest_reg); break;
        }
        cpu->cycles += 3;
        break;
    }

    case 0x08: { // div
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A = cpu->A / val; break;
            case REG_B: cpu->B = cpu->B / val; break;
            case REG_C: cpu->C = cpu->C / val; break;
            default: fprintf(stderr, "Invalid dest reg %u for div\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x09: { // mul
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A = cpu->A * val; break;
            case REG_B: cpu->B = cpu->B * val; break;
            case REG_C: cpu->C = cpu->C * val; break;
            default: fprintf(stderr, "Invalid dest reg %u for mul\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x10: { // add
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A = cpu->A + val; break;
            case REG_B: cpu->B = cpu->B + val; break;
            case REG_C: cpu->C = cpu->C + val; break;
            default: fprintf(stderr, "Invalid dest reg %u for ADD\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x11: { // sub
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A = cpu->A - val; break;
            case REG_B: cpu->B = cpu->B - val; break;
            case REG_C: cpu->C = cpu->C - val; break;
            default: fprintf(stderr, "Invalid dest reg %u for SUB\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x12: { // and
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A &= val; break;
            case REG_B: cpu->B &= val; break;
            case REG_C: cpu->C &= val; break;
            default: fprintf(stderr, "Invalid dest reg %u for AND\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x13: { // or
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A |= val; break;
            case REG_B: cpu->B |= val; break;
            case REG_C: cpu->C |= val; break;
            default: fprintf(stderr, "Invalid dest reg %u for OR\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x14: { // xor
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A ^= val; break;
            case REG_B: cpu->B ^= val; break;
            case REG_C: cpu->C ^= val; break;
            default: fprintf(stderr, "Invalid dest reg %u for XOR\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x15: { // not
        uint8_t dest = mem_read(cpu->PC++);
        switch (dest) {
            case REG_A: cpu->A = ~cpu->A; break;
            case REG_B: cpu->B = ~cpu->B; break;
            case REG_C: cpu->C = ~cpu->C; break;
            default: fprintf(stderr, "Invalid dest reg %u for NOT\n", dest); break;
        }
        cpu->cycles += 1;
        break;
    }

    case 0x16: { // nand
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A = ~(cpu->A & val); break;
            case REG_B: cpu->B = ~(cpu->B & val); break;
            case REG_C: cpu->C = ~(cpu->C & val); break;
            default: fprintf(stderr, "Invalid dest reg %u for NAND\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x17: { // nor
        uint8_t dest = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, src);

        switch (dest) {
            case REG_A: cpu->A = ~(cpu->A | val); break;
            case REG_B: cpu->B = ~(cpu->B | val); break;
            case REG_C: cpu->C = ~(cpu->C | val); break;
            default: fprintf(stderr, "Invalid dest reg %u for NOR\n", dest); break;
        }
        cpu->cycles += 2;
        break;
    }

    case 0x30: { // STORE
        uint8_t reg = mem_read(cpu->PC++);
        uint8_t addr = mem_read(cpu->PC++);
        switch (reg) {
            case REG_A: mem_write(addr, cpu->A); break;
            case REG_B: mem_write(addr, cpu->B); break;
            case REG_C: mem_write(addr, cpu->C); break;
            default:
                fprintf(stderr, "Invalid register %u for STORE\n", reg);
                break;
        }
        cpu->cycles += 3;
        break;
    }

    case 0x40: { // PRINT
        uint8_t op = mem_read(cpu->PC++);
        if (op == REG_A || op == REG_B || op == REG_C) {
            uint8_t val = (op == REG_A) ? cpu->A : (op == REG_B) ? cpu->B : cpu->C;
            printf("%u\n", val);
        } else {
            printf("%u\n", mem_read(op));
        }
        cpu->cycles += 4;
        break;
    }

    case 0xFF: { // HALT
        printf("System exited at PC=0x%04X after %" PRIu64 " cycles\n",
               cpu->PC - 1, cpu->cycles);
        exit(0);
    }

    default:
        printf("Unknown opcode: 0x%02X at PC=0x%04X\n", opcode, cpu->PC - 1);
        exit(1);
    }
}

// ---------------------------
// Main
// ---------------------------

int main(int argc, char **argv) {
    bool dump_only = false;
    const char *filename = NULL;

    if (argc == 2) {
        filename = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "-S") == 0) {
        dump_only = true;
        filename = argv[2];
    } else {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s program.asm         (assemble + run)\n", argv[0]);
        fprintf(stderr, "  %s -S program.asm      (assemble only, dump bytecode)\n",
                argv[0]);
        return 1;
    }

    size_t program_size = 0;
    if (!load_program_from_file(filename, 0x0000, &program_size))
        return 1;

    printf("Loaded '%s' (%zu bytes) into RAM\n", filename, program_size);

    if (dump_only) {
        dump_hex(RAM, program_size);
        return 0;
    }

    CPU cpu = {0};
    cpu.PC = 0;

    while (1)
        execute(&cpu);

    return 0;
}
