# Aptus-8

## Current Implementation

### CPU Architecture
- **Registers**:
  - `A`: 8-bit Accumulator
  - `B`, `C`: 8-bit General Purpose Registers
  - `PC`: 16-bit Program Counter
- **Memory**: 64KB addressable space

### Instruction Set

| Opcode | Mnemonic | Description | Example | Cycles |
|--------|----------|-------------|---------|--------|
| 0x01   | mov reg imm  | Load immediate into reg | `0x01 0x42` → reg = 0x42 | 2 |
| 0x10   | ADD      | A = A + B    | `0x10` → A = A + B | 1 |
| 0x11   | SUB      | A = A - B    | `0x11` → A = A - B | 1 |
| 0x20   | JMP addr | Jump to address | `0x20 0x12 0x34` → PC = 0x1234 | 3 |
| 0x30   | STA addr | Store A to memory | `0x30 0x10` → [0x10] = A | 3 |
| 0x31   | STB addr | Store B to memory | `0x31 0x10` → [0x10] = B | 3 |
| 0x32   | STC addr | Store C to memory | `0x32 0x10` → [0x10] = C | 3 |
| 0x40   | PRINT addr| Print memory value | `0x40 0x10` → prints [0x10] | 4 |
| 0xFF   | HALT     | Stop execution | `0xFF` → stops CPU | 1 |

### Timing Model

Each instruction's cycle count is determined by:
- **1 cycle** for the opcode fetch
- **1 cycle** for each additional byte read/write
- **1 cycle** for ALU operations
- **1 cycle** for I/O operations (PRINT)

Example timing breakdown:
- `mov a imm`: 2 cycles (1 opcode + 1 immediate byte)
- `ADD`: 1 cycle (1 opcode, register operation)
- `STA addr`: 3 cycles (1 opcode + 1 address byte + 1 memory write)
- `PRINT addr`: 4 cycles (1 opcode + 1 address byte + 1 memory read + 1 I/O)

### Assembly Syntax

#### Comments
- Single-line comments start with `//` and continue to the end of the line
- Multi-line comments are enclosed in `/*` and `*/`

Example:
```asm
// This is a single-line comment
LDA 0x0A    // Load 10 into A

/* This is a
   multi-line
   comment */
LDB 0x14    // Load 20 into B
```

### Memory Map
- `0x0000 - 0xFFFF`: General purpose RAM

## Future Development

### Planned Features
- [ ] **I/O System**
  - Keyboard input handling
  - Display output
  - Sound system

- [ ] **Extended Registers**
  - Add D, E, H, L registers
  - 16-bit register pairs (e.g., HL, BC, DE)

- [ ] **Bit Operations**
  - AND, OR, XOR
  - Bit shifts and rotates
  - Bit test instructions

- [ ] **Control Flow**
  - Conditional jumps
  - Compare instructions
  - Loop control

### Optimization Ideas
- [ ] Instruction prefetch queue
- [x] Cycle-accurate timing
- [ ] Memory banking

### Debugging Tools
- [ ] Disassembler
- [ ] Step-by-step execution
- [ ] Memory viewer
- [ ] Register watch window

## Notes
- Add your notes and ideas here
- Document any design decisions
- Track progress and issues
