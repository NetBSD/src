#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS relaxed macro with branch swapping
#as: -32
#source: relax-swap3.s

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> 0a00      	la	v0,[0-9a-f]+ <[^>]*>
[0-9a-f]+ <[^>]*> eb00      	jr	v1
[0-9a-f]+ <[^>]*> 6500      	nop
[0-9a-f]+ <[^>]*> f7ff 0a1c 	la	v0,[0-9a-f]+ <[^>]*>
[0-9a-f]+ <[^>]*> 2300      	beqz	v1,[0-9a-f]+ <[^>]*>
	\.\.\.
#pass
