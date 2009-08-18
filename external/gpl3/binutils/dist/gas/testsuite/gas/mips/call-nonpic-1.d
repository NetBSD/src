#as: -mabi=32 -mips2 -call_nonpic
#objdump: -pdr

.*
private flags = 10001004: .*


Disassembly of section \.text:

0+0 <\.text>:
.* 	lui	t9,0x0
.*: R_MIPS_HI16	foo
.* 	addiu	t9,t9,0
.*: R_MIPS_LO16	foo
.* 	jalr	t9
.* 	nop
