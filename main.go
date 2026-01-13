package main

import (
	"fmt"
	"os"
)

//
// ---------------------------
// Main
// ---------------------------
//

func main() {
	if len(os.Args) < 2 {
		fmt.Println("usage: vm [-S|-trace] program.asm")
		return
	}

	dump := false
	trace := false
	file := os.Args[1]
	if os.Args[1] == "-S" {
		dump = true
		file = os.Args[2]
	}
	if os.Args[1] == "-trace" {
		trace = true
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
	for execute(&cpu, trace) {
	}
}
