#objdump: -dr
#name: MIPS align maximum 

# Test the align macro at maximum alignment.

.*:  +file format .*mips.*

Disassembly of section .text:

[0]*00000000 <foo>:
	...

[0]*10000000 <bar>:
	...
