#name: MIPS multi-got-no-shared
#as: -EB -32 -KPIC -mno-shared
#source: multi-got-no-shared-1.s
#source: multi-got-no-shared-2.s
#ld: -melf32btsmip --entry func1
#objdump: -D -j .text --prefix-addresses --show-raw-insn

.*: +file format.*

Disassembly of section \.text:
004000b0 <[^>]*> 3c1c1000 	lui	gp,0x1000
004000b4 <[^>]*> 279c7ff0 	addiu	gp,gp,32752
004000b8 <[^>]*> afbc0008 	sw	gp,8\(sp\)
#...
00408d60 <[^>]*> 3c1c1002 	lui	gp,0x1002
00408d64 <[^>]*> 279c9960 	addiu	gp,gp,-26272
00408d68 <[^>]*> afbc0008 	sw	gp,8\(sp\)
#pass
