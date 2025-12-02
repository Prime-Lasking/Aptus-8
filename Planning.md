# Aptus-8

## Current Implementation

### CPU Architecture
- **Registers**:
  - `A`: 8-bit Accumulator
  - `B`, `C`: 8-bit General Purpose Registers
  - `PC`: 16-bit Program Counter
- **Memory**: 64KB addressable space

### Instruction Set

|| Opcode | Mnemonic | Description | Example | Cycles |
|--------|----------|-------------|---------|--------|
| 0x01   | mov reg, reg/imm8 | Copy 2nd operand to the 1st | `0x01 0x00 0x01` → A = B | 2-3 |
| 0x08   | div reg/imm8 | A = A / src | `0x08 0x01` → A = A / B | 1-2 |
| 0x09   | mul reg/imm8 | A = A * src | `0x09 0x01` → A = A * B | 1-2 |
| 0x10   | add reg/imm8 | A = A + src | `0x10 0x01` → A = A + B | 1-2 |
| 0x11   | sub reg/imm8 | A = A - src | `0x11 0x01` → A = A - B | 1-2 |
| 0x12   | and reg/imm8 | A = A & src | `0x12 0x01` → A = A & B | 1-2 |
| 0x13   | or reg/imm8  | A = A | src | `0x13 0x01` → A = A | B | 1-2 |
| 0x14   | xor reg/imm8 | A = A ^ src | `0x14 0x01` → A = A ^ B | 1-2 |
| 0x15   | not          | A = ~A      | `0x15` → A = ~A    | 1 |
| 0x16   | nand reg/imm8| A = ~(A & src) | `0x16 0x01` → A = ~(A & B) | 1-2 |
| 0x17   | nor reg/imm8 | A = ~(A | src) | `0x17 0x01` → A = ~(A | B) | 1-2 |
| 0x40   | print reg/imm8 | Print value | `0x40 0x01` → prints B | 3-4 |
| 0xFF   | halt     | Stop execution | `0xFF` → stops CPU | 1 |

### Timing Model

Each instruction's cycle count is determined by:
- **1 cycle** for the opcode fetch
- **1 cycle** for each additional byte read/write
- **1 cycle** for ALU operations
- **1 cycle** for I/O operations (PRINT)
- **1 cycle** for memory access (load/store)

Example timing breakdown:
- `mov A, B`: 2 cycles (1 opcode + 1 register)
- `mov A, 0x42`: 3 cycles (1 opcode + 1 register + 1 immediate)
- `add A, B`: 1 cycle (1 opcode, register operation)
- `add A, 0x42`: 2 cycles (1 opcode + 1 immediate)
- `print A`: 3 cycles (1 opcode + 1 register + 1 I/O)
- `print 0x10`: 4 cycles (1 opcode + 1 immediate + 1 memory read + 1 I/O)
### Assembly Syntax

#### Comments
- Single-line comments start with `//` and continue to the end of the line
- Multi-line comments are enclosed in `/*` and `*/`

Example:
```asm
// This is a single-line comment
mov a 0x0A    // Load 10 into A

/* This is a
   multi-line
   comment */
mov b 0x14    // Load 20 into B
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
  - [x] AND, OR, XOR, NOT, NAND, NOR
  - [ ] Bit shifts and rotates
  - [ ] Bit test instructions

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
