#source: ifunc-6-i386.s
#ld: -m elf_i386
#as: --32
#readelf: -r --wide
#target: x86_64-*-* i?86-*-*

Relocation section '.rel.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_386_IRELATIVE[ ]*
