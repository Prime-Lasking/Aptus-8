#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>

// Custom strcasecmp implementation (keeps behavior but avoids redefining the standard one)
int custom_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return (unsigned char)c1 - (unsigned char)c2;
        s1++; s2++;
    }
    return (unsigned char)tolower((unsigned char)*s1) - (unsigned char)tolower((unsigned char)*s2);
}

long custom_strtol(const char *nptr, char **endptr, int base) {
    const char *p = nptr;
    long result = 0;
    int negative = 0;

    // skip leading whitespace
    while (isspace((unsigned char)*p)) p++;

    // sign
    if (*p == '+' || *p == '-') {
        if (*p == '-') negative = 1;
        p++;
    }

    // auto-detect base
    if (base == 0) {
        if (p[0] == '0') {
            if (p[1] == 'x' || p[1] == 'X') {
                base = 16;
                p += 2;
            } else if (isdigit((unsigned char)p[1])) {
                base = 8; // legacy octal if needed (rare)
                p++;
            } else {
                base = 10;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    }

    // parse digits
    while (*p) {
        int digit;
        if (isdigit((unsigned char)*p)) digit = *p - '0';
        else if (isalpha((unsigned char)*p)) {
            digit = tolower((unsigned char)*p) - 'a' + 10;
        } else break;

        if (digit >= base) break;
        result = result * base + digit;
        p++;
    }

    if (endptr) *endptr = (char *)p;
    return negative ? -result : result;
}

// Instruction mnemonics to opcode mapping
typedef struct {
    const char *name;
    uint8_t opcode;
    int operands;
} InstructionDef;

static const InstructionDef instructions[] = {
    {"mov", 0x01, 2},
    {"add", 0x10, 2},
    {"sub", 0x11, 2},
    {"mul", 0x09, 2},
    {"div", 0x08, 2},
    {"and", 0x12, 2},
    {"or", 0x13, 2},
    {"xor", 0x14, 2},
    {"not", 0x15, 1},
    {"nand", 0x16, 2},
    {"nor", 0x17, 2},
    {"print", 0x40, 1},
    {"halt", 0xFF, 0},
    {NULL, 0, 0}
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
    uint8_t A;
    uint8_t B;
    uint8_t C;
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

bool load_program_from_file(const char *path, uint16_t start, size_t *out_size) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Unable to open '%s': %s\n", path, strerror(errno));
        return false;
    }

    size_t max_bytes = sizeof(RAM);
    if (start >= sizeof(RAM)) {
        fprintf(stderr, "Error: Start address 0x%04X is outside RAM range\n", start);
        fclose(file);
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
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

    size_t nread = fread(buffer, 1, file_size, file);
    if (nread != file_size) {
        // still continue but note mismatch
    }
    buffer[file_size] = '\0';
    fclose(file);

    size_t token_count = 0;
    uint8_t *text_bytes = malloc(max_bytes ? max_bytes : 1); // at least 1 for safe pointer
    if (!text_bytes) {
        free(buffer);
        return false;
    }
    bool parsed_as_text = true;

    char *cursor = buffer;
    int in_comment = 0;

    while (*cursor) {
        while (*cursor && isspace((unsigned char)*cursor)) cursor++;
        if (!*cursor) break;

        // single-line comment //
        if (!in_comment && cursor[0] == '/' && cursor[1] == '/') {
            while (*cursor && *cursor != '\n') cursor++;
            continue;
        }
        // start comment /*
        if (!in_comment && cursor[0] == '/' && cursor[1] == '*') {
            in_comment = 1;
            cursor += 2;
            continue;
        }
        // end comment */
        if (in_comment && cursor[0] == '*' && cursor[1] == '/') {
            in_comment = 0;
            cursor += 2;
            continue;
        }
        if (in_comment) {
            cursor++;
            continue;
        }

        // Read mnemonic/token
        char mnemonic[16] = {0};
        const char *start_tok = cursor;
        int tok_len = 0;
        while (*cursor && !isspace((unsigned char)*cursor) && (tok_len < 15)) {
            mnemonic[tok_len++] = *cursor;
            cursor++;
        }
        mnemonic[tok_len] = '\0';
        if (tok_len == 0) { parsed_as_text = false; break; }

        // Find instruction
        const InstructionDef *instr = NULL;
        for (const InstructionDef *i = instructions; i->name; i++)
            if (custom_strcasecmp(mnemonic, i->name) == 0) { instr = i; break; }

        if (instr) {
            if (token_count >= max_bytes) { parsed_as_text = false; break; }
            text_bytes[token_count++] = instr->opcode;

            // skip whitespace
            while (*cursor && isspace((unsigned char)*cursor)) cursor++;

            for (int op = 0; op < instr->operands; op++) {
                if (!*cursor) { parsed_as_text = false; break; }

                // Register?
                if (isalpha((unsigned char)*cursor)) {
                    char r = tolower((unsigned char)*cursor);
                    uint8_t regcode;
                    if (r == 'a') regcode = REG_A;
                    else if (r == 'b') regcode = REG_B;
                    else if (r == 'c') regcode = REG_C;
                    else { parsed_as_text = false; break; }

                    if (token_count >= max_bytes) { parsed_as_text = false; break; }
                    text_bytes[token_count++] = regcode;
                    cursor++;
                } else {
                    // immediate numeric
                    char *next = NULL;
                    long val = custom_strtol(cursor, &next, 0);
                    if (next == cursor) { parsed_as_text = false; break; }
                    if (val < 0 || val > 0xFF) { parsed_as_text = false; break; }

                    if (token_count >= max_bytes) { parsed_as_text = false; break; }
                    text_bytes[token_count++] = (uint8_t)val;
                    cursor = next;
                }

                // skip space and optional comma
                while (*cursor && isspace((unsigned char)*cursor)) cursor++;
                if (*cursor == ',') { cursor++; while (*cursor && isspace((unsigned char)*cursor)) cursor++; }
            }
        } else {
            // couldn't parse mnemonic -> fallback to binary mode
            parsed_as_text = false;
            break;
        }
    }

    size_t loaded = 0;

    if (parsed_as_text) {
        if (token_count > max_bytes) {
            fprintf(stderr, "Error: assembled program too large for RAM area\n");
            free(buffer);
            free(text_bytes);
            return false;
        }
        memcpy(&RAM[start], text_bytes, token_count);
        loaded = token_count;
    } else {
        // treat file as raw binary blob
        size_t to_copy = (file_size <= max_bytes) ? file_size : max_bytes;
        memcpy(&RAM[start], buffer, to_copy);
        loaded = to_copy;
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
        default:    return src; // immediate
    }
}

void execute(CPU *cpu) {
    uint8_t opcode = mem_read(cpu->PC++);

    switch (opcode) {
    case 0x01: { // mov dest, src
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
    case 0x10: { // add dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t lhs = read_src_value(cpu, dest_reg);
        uint8_t rhs = read_src_value(cpu, src);
        uint8_t res = lhs + rhs;
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in add\n", dest_reg); break;
        }
        cpu->cycles += 3;
        break;
    }
    case 0x11: { // sub dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t lhs = read_src_value(cpu, dest_reg);
        uint8_t rhs = read_src_value(cpu, src);
        uint8_t res = lhs - rhs;
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in sub\n", dest_reg); break;
        }
        cpu->cycles += 3;
        break;
    }
    case 0x09: { // mul dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint16_t res16 = (uint16_t)read_src_value(cpu, dest_reg) * (uint16_t)read_src_value(cpu, src);
        uint8_t res = (uint8_t)(res16 & 0xFF);
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in mul\n", dest_reg); break;
        }
        cpu->cycles += 5;
        break;
    }
    case 0x08: { // div dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t divisor = read_src_value(cpu, src);
        if (divisor == 0) {
            fprintf(stderr, "Runtime error: division by zero\n");
            exit(1);
        }
        uint8_t res = read_src_value(cpu, dest_reg) / divisor;
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in div\n", dest_reg); break;
        }
        cpu->cycles += 10;
        break;
    }
    case 0x12: { // and dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t res = read_src_value(cpu, dest_reg) & read_src_value(cpu, src);
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in and\n", dest_reg); break;
        }
        cpu->cycles += 1;
        break;
    }
    case 0x13: { // or dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t res = read_src_value(cpu, dest_reg) | read_src_value(cpu, src);
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in or\n", dest_reg); break;
        }
        cpu->cycles += 1;
        break;
    }
    case 0x14: { // xor dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t res = read_src_value(cpu, dest_reg) ^ read_src_value(cpu, src);
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in xor\n", dest_reg); break;
        }
        cpu->cycles += 1;
        break;
    }
    case 0x15: { // not dest
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t res = ~read_src_value(cpu, dest_reg);
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in not\n", dest_reg); break;
        }
        cpu->cycles += 1;
        break;
    }
    case 0x16: { // nand dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t res = ~(read_src_value(cpu, dest_reg) & read_src_value(cpu, src));
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in nand\n", dest_reg); break;
        }
        cpu->cycles += 2;
        break;
    }
    case 0x17: { // nor dest, src
        uint8_t dest_reg = mem_read(cpu->PC++);
        uint8_t src = mem_read(cpu->PC++);
        uint8_t res = ~(read_src_value(cpu, dest_reg) | read_src_value(cpu, src));
        switch (dest_reg) {
            case REG_A: cpu->A = res; break;
            case REG_B: cpu->B = res; break;
            case REG_C: cpu->C = res; break;
            default: fprintf(stderr, "Invalid dest reg %u in nor\n", dest_reg); break;
        }
        cpu->cycles += 2;
        break;
    }
    case 0x40: { // print operand
        uint8_t op = mem_read(cpu->PC++);
        uint8_t val = read_src_value(cpu, op);
        // print value as unsigned integer
        printf("%u\n", (unsigned)val);
        cpu->cycles += 2;
        break;
    }
    case 0xFF: { // halt
        cpu->cycles += 1;
        exit(0);
        break;
    }
    default:
        printf("Unknown opcode: 0x%02X at PC=0x%04X\n", opcode, (unsigned)(cpu->PC - 1));
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
        fprintf(stderr, "  %s -S program.asm      (assemble only, dump bytecode)\n", argv[0]);
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

    // run until halt (or error)
    bool halted = false;
    while (!halted) {
        execute(&cpu);
        if (cpu.cycles % 1000000 == 0) {  // Print cycles periodically to avoid flooding output
            printf("Total cycles: %llu\n", cpu.cycles);
        }
        // Check if CPU has halted
        if (mem_read(cpu.PC) == 0xFF) {  // Check if next instruction is halt
            halted = true;
        }
    }

    printf("Total cycles: %llu\n", cpu.cycles);
    return 0;
}
