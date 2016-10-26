#objdump: -dr --prefix-addresses
#name: MIPS add
#source: add.s
#as: -32

# Test the add macro.

.*: +file format .*mips.*

Disassembly of section .text:
0+0000 <[^>]*> addiu	a0,a0,1
	\.\.\.
