#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS branch-misc-4-64
#as: -64
#source: branch-misc-4.s

# Verify PC-relative relocations do not overflow (microMIPS).

.*: +file format .*mips.*

Disassembly of section \.text:
	\.\.\.
[0-9a-f]+ <[^>]*> 9400 0000 	b	[0-9a-f]+ <foo\+0x[0-9a-f]+>
[ 	]*[0-9a-f]+: R_MICROMIPS_PC16_S1	bar\-0x4
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\-0x4
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\-0x4
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9400 0000 	b	[0-9a-f]+ <foo\+0x[0-9a-f]+>
[ 	]*[0-9a-f]+: R_MICROMIPS_PC16_S1	\.init\+0x2
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\+0x2
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\+0x2
[0-9a-f]+ <[^>]*> 0c00      	nop
	\.\.\.

Disassembly of section \.init:
[0-9a-f]+ <[^>]*> 9400 0000 	b	[0-9a-f]+ <bar\+0x[0-9a-f]+>
[ 	]*[0-9a-f]+: R_MICROMIPS_PC16_S1	foo\-0x4
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\-0x4
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\-0x4
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9400 0000 	b	[0-9a-f]+ <bar\+0x[0-9a-f]+>
[ 	]*[0-9a-f]+: R_MICROMIPS_PC16_S1	\.text\+0x40002
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\+0x40002
[ 	]*[0-9a-f]+: R_MIPS_NONE	\*ABS\*\+0x40002
[0-9a-f]+ <[^>]*> 0c00      	nop
	\.\.\.
