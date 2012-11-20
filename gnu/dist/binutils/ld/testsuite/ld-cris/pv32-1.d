#source: pv32.s
#as: --march=v32 --no-underscore --em=criself
#ld: -e here -m crislinux tmpdir/libdso-12.so
#objdump: -s -T

# Trivial test of linking a program to a v32 DSO.

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
0+822a0 g    DO \*ABS\*	0+ _DYNAMIC
0+8023a      DF \*UND\*	0+2 expfn
0+82354 g    DO \.bss	0+ expobj
0+82354 g    D  \*ABS\*	0+ __bss_start
0+80254      DF \*UND\*	0+8 dsofn3
0+82354 g    D  \*ABS\*	0+ _edata
0+82360 g    D  \*ABS\*	0+ _end
0+80280 g    DF \.text	0+8 dsofn

Contents of section \.interp:
 800d4 2f6c6962 2f6c642e 736f2e31 00        .*
#...
Contents of section \.rela\.dyn:
 801fc 54230800 09030000 00000000           .*
Contents of section \.rela\.plt:
 80208 4c230800 0b020000 00000000 50230800  .*
 80218 0b050000 00000000                    .*
Contents of section \.plt:
 80220 84e26ffe 44230800 7e7a3f7a 04f26ffa  .*
 80230 bf09b005 00000000 00006ffe 4c230800  .*
 80240 6ffabf09 b0053f7e 00000000 bf0ed4ff  .*
 80250 ffffb005 6ffe5023 08006ffa bf09b005  .*
 80260 3f7e0c00 0000bf0e baffffff b005      .*
Contents of section \.text:
 8026e b005bfbe caffffff b005bfbe dcffffff  .*
 8027e b0056fae 54230800 b0050000           .*
Contents of section \.dynamic:
#...
Contents of section \.got:
 82340 a0220800 00000000 00000000 46020800  .*
 82350 60020800                             .*
