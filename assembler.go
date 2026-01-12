package main

import (
	"fmt"
	"os"
	"strings"
)

func loadProgram(filename string, start uint16) (int, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return 0, err
	}

	lines := strings.Split(string(data), "\n")
	labels := make(map[string]byte)

	// First pass: labels
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

	// Second pass: emit
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
