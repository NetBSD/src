#objdump: -r -j .text
#name: MIPS JALR reloc (n64)
#as: -64
#source: jalr3.s

.*: +file format .*mips.*

RELOCATION RECORDS FOR \[\.text\]:
OFFSET           TYPE              VALUE 
0000000000000000 R_MIPS_JALR       \$bar
0000000000000000 R_MIPS_NONE       \*ABS\*
0000000000000000 R_MIPS_NONE       \*ABS\*
0000000000000008 R_MIPS_JALR       \$bar
0000000000000008 R_MIPS_NONE       \*ABS\*
0000000000000008 R_MIPS_NONE       \*ABS\*
