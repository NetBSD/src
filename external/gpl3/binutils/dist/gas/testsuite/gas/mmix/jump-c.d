#as: -x
#objdump: -tdr

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  .text	0+ (|\.text)
0+ l    d  .data	0+ (|\.data)
0+ l    d  .bss	0+ (|\.bss)
ffff0000ffff0000 l       \*ABS\*	0+ i1
ffff0000ffff0000 l       \*ABS\*	0+ i2
0+ g     F .text	0+ Main

Disassembly of section .text:

0+ <Main>:
   0:	f0000000 	jmp 0 <Main>
			0: R_MMIX_JMP	\*ABS\*-0xffff00010000
   4:	fd000000 	swym 0,0,0
   8:	fd000000 	swym 0,0,0
   c:	fd000000 	swym 0,0,0
  10:	fd000000 	swym 0,0,0
  14:	f0000000 	jmp 14 <Main\+0x14>
			14: R_MMIX_JMP	i2
  18:	fd000000 	swym 0,0,0
  1c:	fd000000 	swym 0,0,0
  20:	fd000000 	swym 0,0,0
  24:	fd000000 	swym 0,0,0
