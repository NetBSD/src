#source: start1.s
#source: tls128.s
#source: tls-le-13.s
#source: tls-gd-3.s
#source: tls-legd-16.s
#source: tls-x1x2.s
#as: --no-underscore --em=criself
#ld: -m crislinux tmpdir/tls-dso-xz-1.so
#objdump: -s -h -t -T -R -r -p

# Check that we have proper NPTL/TLS markings and GOT for an
# executable with two R_CRIS_32_TPREL and two R_CRIS_32_GD, different
# symbols, GD symbols defined elsewhere.

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
0+      D  \*UND\*	0+ x
0+      D  \*UND\*	0+ z
#...
DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
00082310 R_CRIS_DTP        x
00082318 R_CRIS_DTP        z

Contents of section .interp:
#...
Contents of section \.text:
 801dc 41b20000 6faef8ff ffff6fae fcffffff  .*
 801ec 6fae1023 08000000 6fae1823 08000000  .*
#...
Contents of section \.got:
 82304 84220800 0+ 0+ 0+  .*
 82314 0+ 0+ 0+           .*
