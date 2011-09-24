#source: ifunc-13a-x86-64.s
#source: ifunc-13b-x86-64.s
#ld: -shared -m elf_x86_64 -z nocombreloc
#as: --64
#readelf: -r --wide
#target: x86_64-*-*

Relocation section '.rela.got' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_X86_64_GLOB_DAT[ ]+ifunc\(\)[ ]+ifunc \+ 0
#...
Relocation section '.rela.ifunc' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_X86_64_64[ ]+ifunc\(\)[ ]+ifunc \+ 0
#...
Relocation section '.rela.plt' at .*
[ ]+Offset[ ]+Info[ ]+Type[ ]+.*
[0-9a-f]+[ ]+[0-9a-f]+[ ]+R_X86_64_JUMP_SLOT[ ]+ifunc\(\)[ ]+ifunc \+ 0
