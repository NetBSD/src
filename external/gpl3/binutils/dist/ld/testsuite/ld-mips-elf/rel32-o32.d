#name: MIPS rel32 o32
#source: rel32.s
#as: -KPIC -EB -32
#readelf: -x .text -r
#ld: -shared -melf32btsmip

Relocation section '.rel.dyn' at offset .* contains 2 entries:
 Offset     Info    Type            Sym.Value  Sym. Name
[0-9a-f ]+R_MIPS_NONE      
[0-9a-f ]+R_MIPS_REL32     

Hex dump of section '.text':
  0x000002c0 00000000 00000000 00000000 00000000 ................
  0x000002d0 000002d0 00000000 00000000 00000000 ................
  0x000002e0 00000000 00000000 00000000 00000000 ................
