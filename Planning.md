# Aptus-8

## Current Implementation

### CPU Architecture

* **Registers**:

  * `A`, `B`, `C`: 8-bit general purpose registers
  * `PC`: 16-bit program counter
* **Memory**: 64KB addressable space (0x0000–0xFFFF)

> Note: There is no dedicated accumulator; all ALU operations explicitly name a destination register.

---

### Instruction Encoding

* Instructions are encoded as:

  * **1 byte opcode**
  * **N operand bytes** (register or immediate)
* **Registers** are encoded as:

  * `A = 0`, `B = 1`, `C = 2`
* **Immediate values** are encoded as `0x80 | imm7`

  * High bit (`0x80`) distinguishes immediates from registers
  * Effective immediate range: `0–127`

---

### Instruction Set

| Opcode | Mnemonic    | Operands | Description              | Example     | Cycles     |   |
| -----: | ----------- | -------- | ------------------------ | ----------- | ---------- | - |
|   0x01 | `mov d, s`  | 2        | Copy `s` into `d`        | `mov a, b`  | 3          |   |
|   0x08 | `div d, s`  | 2        | `d = d / s`              | `div a, b`  | 10         |   |
|   0x09 | `mul d, s`  | 2        | `d = d * s` (low 8 bits) | `mul a, b`  | 5          |   |
|   0x10 | `add d, s`  | 2        | `d = d + s`              | `add a, b`  | 3          |   |
|   0x11 | `sub d, s`  | 2        | `d = d - s`              | `sub a, b`  | 3          |   |
|   0x12 | `and d, s`  | 2        | `d = d & s`              | `and a, b`  | 1          |   |
|   0x13 | `or d, s`   | 2        | `d = d                   | s`          | `or a, b`  | 1 |
|   0x14 | `xor d, s`  | 2        | `d = d ^ s`              | `xor a, b`  | 1          |   |
|   0x15 | `not d`     | 1        | `d = ~d`                 | `not a`     | 1          |   |
|   0x16 | `nand d, s` | 2        | `d = ~(d & s)`           | `nand a, b` | 2          |   |
|   0x17 | `nor d, s`  | 2        | `d = ~(d                 | s)`         | `nor a, b` | 2 |
|   0x40 | `print s`   | 1        | Print value              | `print a`   | 2          |   |
|   0xFF | `halt`      | 0        | Stop execution           | `halt`      | 1          |   |

---

### Timing Model (Actual)

Cycle counts are **explicitly implemented per instruction** in the VM and are **not derived** from a generic fetch/execute formula.

Examples:

* `mov a, b` → **3 cycles**
* `add a, b` → **3 cycles**
* `mul a, b` → **5 cycles**
* `div a, b` → **10 cycles**
* `print a` → **2 cycles**

> Timing is deterministic but not microarchitecturally modeled.

---

### Assembly Syntax

#### Operands

* Registers: `a`, `b`, `c` (case-insensitive)
* Immediates: decimal, hex (`0xNN`), or octal
* Commas between operands are optional

Examples:

```asm
mov a, 10
mov b, 0x14
add a, b
print a
```

#### Comments

* Single-line comments start with `//`
* Multi-line comments use `/* ... */`

Example:

```asm
// Single-line comment
mov a, 0x0A

/* Multi-line
   comment */
mov b, 20
```

---

### Memory Map

* `0x0000–0xFFFF`: General-purpose RAM

---

## Future Development

### Planned Features

* [ ] **I/O System**

  * Keyboard input
  * Display output
  * Sound

* [ ] **Extended Registers**

  * Additional 8-bit registers
  * 16-bit register pairs

* [ ] **Bit Operations**

  * [x] AND, OR, XOR, NOT, NAND, NOR
  * [ ] Shifts and rotates
  * [ ] Bit test instructions

* [ ] **Control Flow**

  * Jumps and branches
  * Compare instructions
  * Loops

### Optimization Ideas

* [ ] Instruction prefetch
* [x] Fixed per-instruction cycle timing
* [ ] Memory banking

### Debugging Tools

* [ ] Disassembler
* [ ] Single-step execution
* [ ] Memory viewer
* [ ] Register watch

---

## Notes

* Immediate values are limited to 7 bits due to encoding
* Arithmetic wraps on overflow
* Division by zero halts execution with an error
