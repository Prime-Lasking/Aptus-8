package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"
)

//
// ---------------------------
// Helpers
// ---------------------------
//

func strcasecmp(a, b string) bool {
	return strings.EqualFold(a, b)
}

func parseInt(s string) (int64, error) {
	s = strings.TrimSpace(s)
	base := 10
	if strings.HasPrefix(s, "0x") || strings.HasPrefix(s, "0X") {
		base = 16
		s = s[2:]
	}
	return strconv.ParseInt(s, base, 64)
}

//
// ---------------------------
// Instruction Definitions
// ---------------------------
//

type Instruction struct {
	Name     string
	Opcode   byte
	Operands int
}

var instructionSet = []Instruction{
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
	{"cmp", 0x18, 2},
	{"print", 0x40, 1},
	{"jmp", 0x20, 1},
	{"jz", 0x21, 1},
	{"jnz", 0x22, 1},
	{"halt", 0xFF, 0},
}

const (
	REG_A   = 0
	REG_B   = 1
	REG_C   = 2
	REG_CMP = 3
)

//
// ---------------------------
// CPU & Memory
// ---------------------------
//

type CPU struct {
	A, B, C, CMP byte
	PC           uint16
	Cycles       uint64
}

var RAM [65536]byte

func memRead(addr uint16) byte {
	return RAM[addr]
}

//
// ---------------------------
// Assembler
// ---------------------------
//

func loadProgram(filename string, start uint16) (int, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return 0, err
	}

	lines := strings.Split(string(data), "\n")
	labels := make(map[string]byte)

	// ---------- First pass (labels) ----------
	addr := byte(start)
	for _, line := range lines {
		line = strings.TrimSpace(line)
		if line == "" || strings.HasPrefix(line, "//") {
			continue
		}
		if strings.HasSuffix(line, ":") {
			labels[strings.ToLower(strings.TrimSuffix(line, ":"))] = addr
			continue
		}

		parts := strings.FieldsFunc(line, func(r rune) bool {
			return r == ' ' || r == '\t' || r == ',' || r == ';'
		})

		for _, ins := range instructionSet {
			if strcasecmp(parts[0], ins.Name) {
				addr += byte(1 + ins.Operands)
				break
			}
		}
	}

	// ---------- Second pass (emit) ----------
	var output []byte

	for _, line := range lines {
		line = strings.TrimSpace(line)
		if line == "" || strings.HasPrefix(line, "//") || strings.HasSuffix(line, ":") {
			continue
		}

		parts := strings.FieldsFunc(line, func(r rune) bool {
			return r == ' ' || r == '\t' || r == ',' || r == ';'
		})

		var instr *Instruction
		for i := range instructionSet {
			if strcasecmp(parts[0], instructionSet[i].Name) {
				instr = &instructionSet[i]
				break
			}
		}
		if instr == nil {
			return 0, fmt.Errorf("unknown instruction: %s", parts[0])
		}

		output = append(output, instr.Opcode)

		for i := 0; i < instr.Operands; i++ {
			op := strings.ToLower(parts[i+1])
			switch op {
			case "a":
				output = append(output, REG_A)
			case "b":
				output = append(output, REG_B)
			case "c":
				output = append(output, REG_C)
			default:
				if lbl, ok := labels[op]; ok {
					output = append(output, lbl)
				} else {
					v, err := parseInt(op)
					if err != nil {
						return 0, err
					}
					output = append(output, byte(v)|0x80)
				}
			}
		}
	}

	copy(RAM[start:], output)
	return len(output), nil
}

//
// ---------------------------
// Execution
// ---------------------------
//

func readSrc(cpu *CPU, src byte) byte {
	if src&0x80 != 0 {
		return src & 0x7F
	}
	switch src {
	case REG_A:
		return cpu.A
	case REG_B:
		return cpu.B
	case REG_C:
		return cpu.C
	case REG_CMP:
		return cpu.CMP
	}
	return src
}

func writeReg(cpu *CPU, r byte, v byte) {
	switch r {
	case REG_A:
		cpu.A = v
	case REG_B:
		cpu.B = v
	case REG_C:
		cpu.C = v
	case REG_CMP:
		if v != 0 {
			cpu.CMP = 1
		} else {
			cpu.CMP = 0
		}
	}
}

func execute(cpu *CPU) bool {
	op := memRead(cpu.PC)
	cpu.PC++

	switch op {

	case 0x01: // mov
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, s))

	case 0x10: // add
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)+readSrc(cpu, s))

	case 0x11: // sub
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)-readSrc(cpu, s))

	case 0x18: // cmp
		a, b := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		if readSrc(cpu, a) == readSrc(cpu, b) {
			cpu.CMP = 0
		} else {
			cpu.CMP = 1
		}

	case 0x20: // jmp
		cpu.PC = uint16(memRead(cpu.PC))

	case 0x21: // jz
		addr := memRead(cpu.PC)
		if cpu.CMP == 0 {
			cpu.PC = uint16(addr)
		} else {
			cpu.PC++
		}

	case 0x22: // jnz
		addr := memRead(cpu.PC)
		if cpu.CMP != 0 {
			cpu.PC = uint16(addr)
		} else {
			cpu.PC++
		}

	case 0x40: // print
		o := memRead(cpu.PC)
		cpu.PC++
		fmt.Println(readSrc(cpu, o))

	case 0xFF: // halt
		fmt.Printf("Total cycles: %d\n", cpu.Cycles)
		return false

	default:
		panic(fmt.Sprintf("bad opcode %02X", op))
	}

	cpu.Cycles++
	return true
}

//
// ---------------------------
// Main
// ---------------------------
//

func main() {
	if len(os.Args) < 2 {
		fmt.Println("usage: vm [-S] program.asm")
		return
	}

	dump := false
	file := os.Args[1]
	if os.Args[1] == "-S" {
		dump = true
		file = os.Args[2]
	}

	size, err := loadProgram(file, 0)
	if err != nil {
		panic(err)
	}

	fmt.Printf("Loaded %d bytes\n", size)

	if dump {
		for i := 0; i < size; i++ {
			fmt.Printf("%02X ", RAM[i])
		}
		fmt.Println()
		return
	}

	cpu := CPU{}
	for execute(&cpu) {
	}
}
