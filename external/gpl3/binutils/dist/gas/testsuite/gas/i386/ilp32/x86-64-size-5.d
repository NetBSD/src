#name: x32 size 5
#source: ../x86-64-size-5.s
#readelf: -r


Relocation section '.rela.text' at offset .* contains 3 entries:
 Offset     Info    Type            Sym.Value  Sym. Name \+ Addend
0+2  00000421 R_X86_64_SIZE64   00000000   xxx \+ 0
0+c  00000421 R_X86_64_SIZE64   00000000   xxx - 8
0+16  00000421 R_X86_64_SIZE64   00000000   xxx \+ 8

Relocation section '.rela.data' at offset .* contains 3 entries:
 Offset     Info    Type            Sym.Value  Sym. Name \+ Addend
0+50  00000421 R_X86_64_SIZE64   00000000   xxx - 1
0+58  00000621 R_X86_64_SIZE64   00000000   yyy \+ c8
0+60  00000521 R_X86_64_SIZE64   00000020   zzz \+ 0
#pass
