#source: start1.s
#source: expdref3.s
#as: --no-underscore --em=criself
#ld: -m crislinux
#ld_after_inputfiles: tmpdir/libdso-15.so
#readelf: -a -x 10

# Like libdso-15b.d, but referencing the weak symbol and function from
# a program.  At some time we broke emitting a copy reloc for the
# object, instead yielding NULL.

#...
 +\[[0-9]+\] .got +PROGBITS +0+82314 000314 000010 04 +WA +0 +0 +4
#...
 +\[[0-9]+\] .bss +NOBITS +0+82324 .*
#...
Relocation section '.rela.dyn' at offset 0x... contains 1 entries:
 Offset +Info +Type +Sym.Value +Sym. Name \+ Addend
00082324 +00000109 R_CRIS_COPY +00082324 +__expobj2 \+ 0

Relocation section '.rela.plt' at offset 0x... contains 1 entries:
 Offset +Info +Type +Sym.Value +Sym. Name \+ Addend
00082320 +0000030b R_CRIS_JUMP_SLOT +00080238 +expfn2 \+ 0

There are no unwind sections in this file.

Symbol table '.dynsym' contains . entries:
#...
 +.: 00082324 +4 +OBJECT +GLOBAL +DEFAULT +13 __expobj2@TST3 \(2\)
#...
 +.: 00080238 +0 +FUNC +GLOBAL +DEFAULT +UND expfn2@TST3 \(2\)
#...
Symbol table '.symtab' contains .. entries:
#...
Hex dump of section '\.text':
  0x0008024c 41b20000 6fae2423 08006fae 38020800 .*
