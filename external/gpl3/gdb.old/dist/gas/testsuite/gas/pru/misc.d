#objdump: -dr --prefix-addresses --show-raw-insn
#name: PRU misc

# Test the miscellaneous instruction

.*: +file format elf32-pru

Disassembly of section .text:
0+0000 <[^>]*> 2a000000 	halt
0+0004 <[^>]*> 3e800000 	slp	1
0+0008 <[^>]*> 3e000000 	slp	0
