#as: --no-underscore --pic --em=criself
#source: expdyn1.s
#source: locref1.s
#ld: -m crislinux
#objdump: -dt

# Referencing a "normal" (non-hidden) symbol with a local-only PIC
# relocation is ok when building an executable.

.*:     file format elf32-cris

SYMBOL TABLE:
#...
0+80076 g     F \.text	0+2 expfn
0+820ac g     O \.data	0+ expobj
#...
0+820a0 g     O \.got	0+ \.hidden _GLOBAL_OFFSET_TABLE_
#...
Disassembly of section \.text:
#...
0+80078 <y>:
   80078:	6fae d6df ffff  .*
   8007e:	6fbe 0c00 0000  .*
