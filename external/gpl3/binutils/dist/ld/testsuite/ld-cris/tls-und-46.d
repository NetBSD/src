#source: tls-gd-1.s --pic
#as: --no-underscore --em=criself
#ld: -m crislinux --shared
#readelf: -a -x 7

# Undefined reference for a R_CRIS_16_GOT_GD in a DSO.  Not an error;
# it's ok for a DSO to have undefined references upon creation for
# global symbols that can be overridden.  Just make sure GOT, dynsyms
# and dynrelocs look right.

#...
Relocation section '.rela.dyn' at offset 0x.* contains 1 entries:
 Offset +Info +Type +Sym.Value +Sym. Name \+ Addend
000021e8 +00000217 R_CRIS_DTP +00000000 +x \+ 0

There are no unwind sections in this file.

Symbol table '.dynsym' contains . entries:
 +Num: +Value +Size +Type +Bind +Vis +Ndx +Name
#...
 +[0-9]: 00000000 +0 +TLS +GLOBAL +DEFAULT +UND x
#...
Symbol table '.symtab' contains .. entries:
#...
Hex dump of section '.got':
  0x0+21dc 6c210000 00000000 00000000 00000000 .*
  0x0+21ec 00000000                            .*
