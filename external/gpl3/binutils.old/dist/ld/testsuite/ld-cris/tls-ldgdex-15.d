#source: start1.s
#source: tls128.s
#source: tls-ld-7.s
#source: tls-gd-2.s
#source: tls-ldgd-15.s
#source: tls-hx1x2.s
#as: --pic --no-underscore --em=criself
#ld: -m crislinux tmpdir/tls-dso-xz-1.so
#objdump: -s -h -t -T -R -r -p

# Check that we have proper NPTL/TLS markings and GOT for two
# R_CRIS_32_GOT_GD and two R_CRIS_32_DTPRELs against different
# variables, for an executable, GD symbols defined elsewhere.

.*:     file format elf32-cris

Program Header:
#...
     TLS off  .*
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
00082318 R_CRIS_DTP        x
00082320 R_CRIS_DTP        z

Contents of section \.interp:
#...
Contents of section \.text:
 801dc 41b20000 6fae8000 00006fbe 84000000  .*
 801ec 6fae1400 00000000 6fae1c00 00000000  .*
Contents of section \.tdata:
#...
Contents of section \.got:
 82304 84220800 0+ 0+ 010+  .*
 82314 0+ 0+ 0+ 0+  .*
 82324 0+                             .*
