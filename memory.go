package main

var RAM [65536]byte

func memRead(addr uint16) byte {
	return RAM[addr]
}
