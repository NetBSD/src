#source: ifunc-13a-i386.s
#source: ifunc-13b-i386.s
#ld: -shared -m elf_i386 -z nocombreloc
#as: --32
#readelf: -r --wide
#target: x86_64-*-* i?86-*-*

Relocation section '.rel.got' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
#...
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_386_GLOB_DAT[ ]+ifunc\(\)[ ]+ifunc
#...
Relocation section '.rel.ifunc' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_386_32[ ]+ifunc\(\)[ ]+ifunc
#...
Relocation section '.rel.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_386_JUMP_SLOT[ ]+ifunc\(\)[ ]+ifunc
