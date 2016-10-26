#source: tls128.s
#source: tls-ld-7.s
#source: tls-gd-2.s
#source: tls-ldgd-15.s
#source: tls-hx1x2.s
#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux tmpdir/tls-dso-xz-1.so
#objdump: -s -h -t -T -R -r -p

# Check that we have proper NPTL/TLS markings and GOT for two
# R_CRIS_32_GOT_GD and two R_CRIS_32_DTPRELs against different
# variables in a DSO, GD symbols defined elsewhere.

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
  7 .got .*
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
00002344 R_CRIS_DTPMOD     \*ABS\*
0000234c R_CRIS_DTP        x
00002354 R_CRIS_DTP        z

Contents of section \.hash:
#...
Contents of section \.text:
 021c 6fae8000 00006fbe 84000000 6fae1400  .*
 022c 0+ 6fae1c00 0+           .*
Contents of section .tdata:
#...
Contents of section \.got:
 2338 c0220+ 0+ 0+ 0+  .*
 2348 0+ 0+ 0+ 0+  .*
 2358 0+                             .*
