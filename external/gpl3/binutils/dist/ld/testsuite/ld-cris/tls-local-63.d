#source: tls-ie-8.s --pic
#source: tls-hx.s
#as: --no-underscore --em=criself
#ld: -m crislinux --shared
#readelf: -a -x 6 -x 8 -x 5

# A R_CRIS_16_GOT_TPREL in a DSO against a hidden symbol.  Make sure
# the relocation, GOT, .text and .tdata have the right contents.

#...
Relocation section '.rela.dyn' at offset 0x.* contains 1 entries:
 Offset     Info    Type            Sym.Value  Sym. Name \+ Addend
00002210  0000001c R_CRIS_32_TPREL[ ]+00+

There are no unwind sections in this file.

Symbol table '.dynsym' contains 7 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
#...
Symbol table '.symtab' contains 16 entries:
#...
     .: 00000000 +4 +TLS +LOCAL +DEFAULT +6 x
#...
Hex dump of section '.text':
  0x00000184 5fae0c00                            .*
#...
Hex dump of section '.tdata':
  0x00002188 280+                            .*
#...
Hex dump of section '.got':
  0x0+2204 8c210000 0+ 0+ 0+ .*
