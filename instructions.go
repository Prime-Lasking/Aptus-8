package main

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
	{"inc", 0x06, 1},
	{"dec", 0x07, 1},
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
