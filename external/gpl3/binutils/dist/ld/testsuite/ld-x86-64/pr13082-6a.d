#source: pr13082-6.s
#name: PR ld/13082-6 (a)
#as: --x32
#ld: -shared -melf32_x86_64
#readelf: -r --wide

Relocation section '.rela.dyn' at offset 0x[0-9a-f]+ contains 1 entries:
 Offset     Info    Type                Sym. Value  Symbol's Name \+ Addend
[0-9a-f]+ +[0-9a-f]+ +R_X86_64_IRELATIVE +[0-9a-f]+

Relocation section '.rela.plt' at offset 0x[0-9a-f]+ contains 1 entries:
 Offset     Info    Type                Sym. Value  Symbol's Name \+ Addend
[0-9a-f]+ +[0-9a-f]+ +R_X86_64_IRELATIVE +[0-9a-f]+
