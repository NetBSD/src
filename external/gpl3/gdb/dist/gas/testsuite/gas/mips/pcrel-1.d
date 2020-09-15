#objdump: -dr
#name: Locally-resolvable PC-relative code references

.*:     file format .*

Disassembly of section .text:

00000000 <func>:
       0:	3c040001 	lui	a0,0x1
       4:	2484800c 	addiu	a0,a0,-32756
	...

00008010 <foo>:
#pass
