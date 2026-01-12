package main

type CPU struct {
	A, B, C, CMP byte
	PC           uint16
	Cycles       uint64
}

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
