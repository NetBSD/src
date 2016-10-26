#source: start1.s
#source: tls128.s
#source: tls-le-13s.s
#source: tls-gd-2.s --pic
#source: tls-ldgd-14.s --pic
#source: tls-x1x2.s
#as: --no-underscore --em=criself
#ld: -m crislinux tmpdir/tls-dso-xz-1.so
#objdump: -s -h -t -T -R -r -p

# Check that we have proper NPTL/TLS markings and GOT for an
# executable with two R_CRIS_16_TPREL a R_CRIS_32_GOT_GD and a
# R_CRIS_16_GOT_GD, different symbols, GD symbols defined elsewhere.

.*:     file format elf32-cris

Program Header:
#...
     TLS off .*
         filesz 0x0+88 memsz 0x0+88 flags r--
Dynamic Section:
  NEEDED               tmpdir/tls-dso-xz-1.so
#...
private flags = 0:

#...
  8 .got .*
                  CONTENTS.*
SYMBOL TABLE:
#...
0+         \*UND\*	0+ x
#...
0+         \*UND\*	0+ z
#...
DYNAMIC SYMBOL TABLE:
#...
0+      D  \*UND\*	0+ x
#...
0+      D  \*UND\*	0+ z
#...
DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
00082308 R_CRIS_DTP        x
00082310 R_CRIS_DTP        z

Contents of section .interp:
#...
Contents of section \.text:
 801dc 41b20000 5faef8ff 5faefcff 6fae0c00  .*
 801ec 00000000 5fae1400                    .*
#...
Contents of section \.got:
 822fc 7c220800 0+ 0+ 0+  .*
 8230c 0+ 0+ 0+           .*
