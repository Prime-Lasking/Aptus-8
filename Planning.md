# Aptus-8

## Current Implementation

### CPU Architecture

* **Registers**:

  * `A`, `B`, `C`: 8-bit general purpose registers
  * `CMP`: 1-bit comparison register (`0 = equal`, `1 = not equal`)
  * `PC`: 16-bit program counter
* **Memory**: 64KB addressable space (0x0000–0xFFFF)

> Note: There is no dedicated accumulator; all ALU operations explicitly name a destination register.

---

### Instruction Encoding

* Instructions are encoded as:

  * **1 byte opcode**
  * **N operand bytes** (register, immediate, or address)
* **Registers** are encoded as:

  * `A = 0`, `B = 1`, `C = 2`, `CMP = 3`
* **Immediate values** are encoded as `0x80 | imm7`

  * High bit (`0x80`) distinguishes immediates from registers
  * Effective immediate range: `0–127`
* **Jump addresses** are encoded as **8-bit absolute addresses**

  * Valid jump range: `0x00–0xFF`

---

### Instruction Set

| Opcode | Mnemonic    | Operands | Description                 | Example     | Cycles     |   |
| -----: | ----------- | -------- | --------------------------- | ----------- | ---------- | - |
|   0x01 | `mov d, s`  | 2        | Copy `s` into `d`           | `mov a, b`  | 1          |   |
|   0x08 | `div d, s`  | 2        | `d = d / s`                 | `div a, b`  | 1          |   |
|   0x09 | `mul d, s`  | 2        | `d = d * s` (low 8 bits)    | `mul a, b`  | 1          |   |
|   0x10 | `add d, s`  | 2        | `d = d + s`                 | `add a, b`  | 1          |   |
|   0x11 | `sub d, s`  | 2        | `d = d - s`                 | `sub a, b`  | 1          |   |
|   0x12 | `and d, s`  | 2        | `d = d & s`                 | `and a, b`  | 1          |   |
|   0x13 | `or d, s`   | 2        | `d = d                      | s`          | `or a, b`  | 1 |
|   0x14 | `xor d, s`  | 2        | `d = d ^ s`                 | `xor a, b`  | 1          |   |
|   0x15 | `not d`     | 1        | `d = ~d`                    | `not a`     | 1          |   |
|   0x16 | `nand d, s` | 2        | `d = ~(d & s)`              | `nand a, b` | 1          |   |
|   0x17 | `nor d, s`  | 2        | `d = ~(d                    | s)`         | `nor a, b` | 1 |
|   0x18 | `cmp a, b`  | 2        | Set `CMP` based on equality | `cmp a, b`  | 1          |   |
|   0x20 | `jmp addr`  | 1        | Unconditional jump          | `jmp loop`  | 1          |   |
|   0x21 | `jz addr`   | 1        | Jump if `CMP == 0`          | `jz done`   | 1          |   |
|   0x22 | `jnz addr`  | 1        | Jump if `CMP != 0`          | `jnz loop`  | 1          |   |
|   0x40 | `print s`   | 1        | Print value                 | `print a`   | 1          |   |
|   0xFF | `halt`      | 0        | Stop execution              | `halt`      | 1          |   |

---

### Comparison Semantics

* `cmp a, b` sets the `CMP` register:

  * `CMP = 0` if `a == b`
  * `CMP = 1` if `a != b`
* `CMP` is only modified by `cmp`
* Conditional jumps (`jz`, `jnz`) test `CMP`

---

### Timing Model

* Each instruction consumes **exactly 1 cycle**
* Cycle counts are explicitly incremented per instruction
* Timing is deterministic and instruction-based

> Timing is simplified and not microarchitecturally modeled.

---

### Assembly Syntax

#### Operands

* Registers: `a`, `b`, `c` (case-insensitive)
* Immediates: decimal or hex (`0xNN`)
* Labels resolve to **8-bit absolute addresses**
* Commas between operands are optional

Example:

```asm
mov a, 10
mov b, 0x14
add a, b
print a
```

---

#### Labels

```asm
loop:
    add a, 1
    jnz loop
```

---

#### Comments

* Single-line comments start with `//`

Example:

```asm
// Increment and print
add a, 1
print a
```

---

### Memory Map

* `0x0000–0xFFFF`: General-purpose RAM

> Note: Control-flow instructions are limited to the lower 256 bytes.

---

## Notes

* Immediate values are limited to 7 bits due to encoding
* Arithmetic wraps on overflow
* Division by zero halts execution with an error
* Jump targets must reside within `0x00–0xFF`

---

## Future Development

### Planned Features

* **I/O System**

  * Keyboard input
  * Display output
  * Sound

* **Extended Registers**

  * Additional 8-bit registers
  * 16-bit register pairs

* **Bit Operations**

  * AND, OR, XOR, NOT, NAND, NOR
  * Shifts and rotates
  * Bit test instructions

* **Debugging Tools**

  * Disassembler
  * Single-step execution
  * Memory viewer
  * Register watch
