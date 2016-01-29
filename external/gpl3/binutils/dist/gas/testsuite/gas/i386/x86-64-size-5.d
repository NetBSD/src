#name: x86-64 size 5
#readelf: -r


Relocation section '.rela.text' at offset .* contains 3 entries:
  Offset          Info           Type           Sym. Value    Sym. Name \+ Addend
0+2  000400000021 R_X86_64_SIZE64   0000000000000000 xxx \+ 0
0+c  000400000021 R_X86_64_SIZE64   0000000000000000 xxx - 8
0+16  000400000021 R_X86_64_SIZE64   0000000000000000 xxx \+ 8

Relocation section '.rela.data' at offset .* contains 3 entries:
  Offset          Info           Type           Sym. Value    Sym. Name \+ Addend
0+50  000400000021 R_X86_64_SIZE64   0000000000000000 xxx - 1
0+58  000600000021 R_X86_64_SIZE64   0000000000000000 yyy \+ c8
0+60  000500000021 R_X86_64_SIZE64   0000000000000020 zzz \+ 0
#pass
