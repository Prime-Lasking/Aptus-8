package main

import "fmt"

func execute(cpu *CPU, trace bool) bool {
	op := memRead(cpu.PC)
	pc := cpu.PC
	cpu.PC++

	cyclesBefore := cpu.Cycles

	switch op {

	// --------------------
	// Data movement
	// --------------------

	case 0x01: // mov
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, s))
		cpu.Cycles += 2 // fetch + operands

	// --------------------
	// Arithmetic
	// --------------------

	case 0x10: // add
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)+readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x11: // sub
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)-readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x09: // mul
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		res := uint16(readSrc(cpu, d)) * uint16(readSrc(cpu, s))
		writeReg(cpu, d, byte(res))
		cpu.Cycles += 6 // multi-cycle ALU

	case 0x08: // div
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		div := readSrc(cpu, s)
		if div == 0 {
			panic("division by zero")
		}
		writeReg(cpu, d, readSrc(cpu, d)/div)
		cpu.Cycles += 10 // very slow op

	case 0x06: // inc
		d := memRead(cpu.PC)
		cpu.PC++
		writeReg(cpu, d, readSrc(cpu, d)+1)
		cpu.Cycles += 2

	case 0x07: // dec
		d := memRead(cpu.PC)
		cpu.PC++
		writeReg(cpu, d, readSrc(cpu, d)-1)
		cpu.Cycles += 2

	// --------------------
	// Bitwise
	// --------------------

	case 0x12: // and
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)&readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x13: // or
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)|readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x14: // xor
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, readSrc(cpu, d)^readSrc(cpu, s))
		cpu.Cycles += 3

	case 0x15: // not
		d := memRead(cpu.PC)
		cpu.PC++
		writeReg(cpu, d, ^readSrc(cpu, d))
		cpu.Cycles += 2

	case 0x16: // nand
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, ^(readSrc(cpu, d) & readSrc(cpu, s)))
		cpu.Cycles += 3

	case 0x17: // nor
		d, s := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		writeReg(cpu, d, ^(readSrc(cpu, d) | readSrc(cpu, s)))
		cpu.Cycles += 3

	// --------------------
	// Compare
	// --------------------

	case 0x18: // cmp
		a, b := memRead(cpu.PC), memRead(cpu.PC+1)
		cpu.PC += 2
		if readSrc(cpu, a) == readSrc(cpu, b) {
			cpu.CMP = 0
		} else {
			cpu.CMP = 1
		}
		cpu.Cycles += 3

	// --------------------
	// Control flow
	// --------------------

	case 0x20: // jmp
		addr := memRead(cpu.PC)
		cpu.PC = uint16(addr)
		cpu.Cycles += 3

	case 0x21: // jz
		addr := memRead(cpu.PC)
		cpu.PC++
		cpu.Cycles += 2
		if cpu.CMP == 0 {
			cpu.PC = uint16(addr)
			cpu.Cycles++ // branch taken penalty
		}

	case 0x22: // jnz
		addr := memRead(cpu.PC)
		cpu.PC++
		cpu.Cycles += 2
		if cpu.CMP != 0 {
			cpu.PC = uint16(addr)
			cpu.Cycles++
		}

	// --------------------
	// I/O
	// --------------------

	case 0x40: // print
		o := memRead(cpu.PC)
		cpu.PC++
		fmt.Println(readSrc(cpu, o))
		cpu.Cycles += 3

	// --------------------
	// Halt
	// --------------------

	case 0xFF: // halt
		cpu.Cycles++
		fmt.Printf("Total cycles: %d\n", cpu.Cycles)
		return false

	default:
		panic(fmt.Sprintf("unknown opcode %02X at PC=%04X", op, cpu.PC-1))
	}

	if trace {
		fmt.Printf(
			"PC=%04X OP=%02X +%d cycles (total=%d)\n",
			pc, op,
			cpu.Cycles-cyclesBefore,
			cpu.Cycles,
		)
	}

	return true
}
