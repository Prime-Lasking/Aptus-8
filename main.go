package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"
	"unicode"
)

//
// ---------------------------
// Helpers
// ---------------------------
//

func strcasecmp(a, b string) bool {
	return strings.EqualFold(a, b)
}

func strtol(s string, base int) (int64, int) {
	i := 0
	for i < len(s) && unicode.IsSpace(rune(s[i])) {
		i++
	}

	start := i

	if base == 0 {
		if strings.HasPrefix(s[i:], "0x") || strings.HasPrefix(s[i:], "0X") {
			base = 16
			i += 2
		} else if strings.HasPrefix(s[i:], "0") && i+1 < len(s) {
			base = 8
			i++
		} else {
			base = 10
		}
	}

	for i < len(s) {
		c := s[i]
		if unicode.IsDigit(rune(c)) || unicode.IsLetter(rune(c)) || c == '-' || c == '+' {
			i++
		} else {
			break
		}
	}

	val, err := strconv.ParseInt(s[start:i], base, 64)
	if err != nil {
		return 0, start
	}
	return val, i
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
	{"print", 0x40, 1},
	{"halt", 0xFF, 0},
}

const (
	REG_A = 0
	REG_B = 1
	REG_C = 2
)

//
// ---------------------------
// CPU & Memory
// ---------------------------
//

type CPU struct {
	A, B, C byte
	PC      uint16
	Cycles  uint64
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

	src := string(data)
	pos := 0
	inComment := false
	var output []byte

	for pos < len(src) {
		for pos < len(src) && unicode.IsSpace(rune(src[pos])) {
			pos++
		}
		if pos >= len(src) {
			break
		}

		if !inComment && strings.HasPrefix(src[pos:], "//") {
			for pos < len(src) && src[pos] != '\n' {
				pos++
			}
			continue
		}

		if !inComment && strings.HasPrefix(src[pos:], "/*") {
			inComment = true
			pos += 2
			continue
		}

		if inComment && strings.HasPrefix(src[pos:], "*/") {
			inComment = false
			pos += 2
			continue
		}

		if inComment {
			pos++
			continue
		}

		startTok := pos
		for pos < len(src) && !unicode.IsSpace(rune(src[pos])) {
			pos++
		}
		mnemonic := src[startTok:pos]

		var instr *Instruction
		for i := range instructionSet {
			if strcasecmp(mnemonic, instructionSet[i].Name) {
				instr = &instructionSet[i]
				break
			}
		}

		if instr == nil {
			return 0, fmt.Errorf("unknown instruction: %s", mnemonic)
		}

		output = append(output, instr.Opcode)

		for i := 0; i < instr.Operands; i++ {
			for pos < len(src) && unicode.IsSpace(rune(src[pos])) {
				pos++
			}

			c := unicode.ToLower(rune(src[pos]))
			if c == 'a' || c == 'b' || c == 'c' {
				reg := map[rune]byte{'a': REG_A, 'b': REG_B, 'c': REG_C}[c]
				output = append(output, reg)
				pos++
			} else {
				val, next := strtol(src[pos:], 0)
				output = append(output, byte(val)|0x80)
				pos += next
			}

			for pos < len(src) && unicode.IsSpace(rune(src[pos])) {
				pos++
			}
			if pos < len(src) && src[pos] == ',' {
				pos++
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
	default:
		return src
	}
}

func writeReg(cpu *CPU, reg byte, val byte) {
	switch reg {
	case REG_A:
		cpu.A = val
	case REG_B:
		cpu.B = val
	case REG_C:
		cpu.C = val
	}
}

func execute(cpu *CPU) {
	op := memRead(cpu.PC)
	cpu.PC++

	switch op {

	case 0x01: // mov
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x10: // add
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)+readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x11: // sub
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)-readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x09: // mul
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		res := uint16(readSrc(cpu, d)) * uint16(readSrc(cpu, s))
		writeReg(cpu, d, byte(res))
		cpu.Cycles += 5

	case 0x08: // div
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		div := readSrc(cpu, s)
		if div == 0 {
			fmt.Println("division by zero")
			os.Exit(1)
		}
		writeReg(cpu, d, readSrc(cpu, d)/div)
		cpu.Cycles += 10

	case 0x12: // and
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)&readSrc(cpu, s))
		cpu.Cycles++

	case 0x13: // or
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)|readSrc(cpu, s))
		cpu.Cycles++

	case 0x14: // xor
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)^readSrc(cpu, s))
		cpu.Cycles++

	case 0x15: // not
		d := memRead(cpu.PC)
		cpu.PC++
		writeReg(cpu, d, ^readSrc(cpu, d))
		cpu.Cycles++

	case 0x16: // nand
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, ^(readSrc(cpu, d) & readSrc(cpu, s)))
		cpu.Cycles += 2

	case 0x17: // nor
		d := memRead(cpu.PC)
		s := memRead(cpu.PC + 1)
		cpu.PC += 2
		writeReg(cpu, d, ^(readSrc(cpu, d) | readSrc(cpu, s)))
		cpu.Cycles += 2

	case 0x40: // print
		o := memRead(cpu.PC)
		cpu.PC++
		fmt.Println(readSrc(cpu, o))
		cpu.Cycles += 2

	case 0xFF: // halt
		cpu.Cycles++
		fmt.Printf("Total cycles: %d\n", cpu.Cycles)
		os.Exit(0)

	default:
		fmt.Printf("Unknown opcode %02X at PC=%04X\n", op, cpu.PC-1)
		os.Exit(1)
	}
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
			if (i+1)%16 == 0 {
				fmt.Println()
			}
		}
		fmt.Println()
		return
	}

	cpu := CPU{}
	for {
		execute(&cpu)
	}
}
