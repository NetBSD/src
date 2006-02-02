#name: MIPS rel32 n32
#source: rel32.s
#as: -KPIC -EB -n32
#readelf: -x 5 -r
#ld: -shared -melf32btsmipn32

Relocation section '.rel.dyn' at offset 0x304 contains 2 entries:
 Offset     Info    Type            Sym.Value  Sym. Name
00000000  00000000 R_MIPS_NONE      
00000330  00000003 R_MIPS_REL32     

Hex dump of section '.text':
  0x00000320 00000000 00000000 00000000 00000000 ................
  0x00000330 00000330 00000000 00000000 00000000 ................
  0x00000340 00000000 00000000 00000000 00000000 ................
