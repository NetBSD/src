#readelf: -rs
#name: x86-64 local PIC

Relocation section '.rela.text' at offset 0x[0-9a-f]+ contains 1 entries:
  Offset          Info           Type           Sym. Value    Sym. Name \+ Addend
[0-9a-f]+ +[0-9a-f]+ R_X86_64_GOTPCREL +[0-9a-f]+ +foo - 4
#...
 +[0-9]+: +[0-9a-f]+ +[0-9a-f]+ +NOTYPE +LOCAL +DEFAULT +[0-9]+ +foo
#pass
