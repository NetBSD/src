#source: start1.s
#source: expdref4.s
#as: --no-underscore --em=criself
#ld: -m crislinux
#ld_after_inputfiles: tmpdir/libdso-15.so
#readelf: -a -x 11

# Like weakref3.d, but just the expobj2 referenced from .data.  We
# should avoid a copy reloc (instead emitting a R_CRIS_GLOB_DAT or
# R_CRIS_32 against the weak symbol), but for the time being, make
# sure we get a valid reloc.

#...
 +\[[0-9]+\] .data +PROGBITS +0+822a4 0002a4 000004 00 +WA +0 +0 +1
#...
 +\[[0-9]+\] .bss +NOBITS +0+822a8 .*
#...
Relocation section '.rela.dyn' at offset 0x... contains 1 entries:
#...
000822a8 +00000109 R_CRIS_COPY +000822a8 +__expobj2 \+ 0

There are no unwind sections in this file.

Symbol table '.dynsym' contains . entries:
#...
 +.: 000822a8 +4 +OBJECT +GLOBAL +DEFAULT +12 __expobj2@TST3 \(2\)
#...
Symbol table '.symtab' contains .. entries:
#...
Hex dump of section '.data':
  0x000822a4 a8220800                            .*

