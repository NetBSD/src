#as: -32
#objdump: -M reg-names=numeric -dr
#name: MIPS1 FP instructions

.*:     file format .*

Disassembly of section .text:

[0-9a-f]+ <foo>:
.*:	46041000 	add.s	\$f0,\$f2,\$f4
.*:	44420000 	cfc1	\$2,\$0
#pass
