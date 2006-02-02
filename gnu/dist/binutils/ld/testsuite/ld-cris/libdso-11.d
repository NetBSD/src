#source: dso-1.s
#source: dsov32-1.s
#as: --pic --no-underscore --march=v32 --em=criself
#ld: --shared -m crislinux
#objdump: -s -T

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
#...
0+1e4 g    DF \.text	0+8 dsofn3
#...
0+1e0 g    DF \.text	0+ dsofn
#...
Contents of section \.rela\.plt:
 01a0 84220000 0b090000 00000000           .*
Contents of section \.plt:
 01ac 84e20401 7e7a3f7a 04f26ffa bf09b005  .*
 01bc 00000000 00000000 00006f0d 0c000000  .*
 01cc 6ffabf09 b0053f7e 00000000 bf0ed4ff  .*
 01dc ffffb005                             .*
Contents of section \.text:
 01e0 b0050000 bfbee2ff ffffb005           .*
Contents of section \.dynamic:
#...
Contents of section \.got:
 2278 00220000 00000000 00000000 d2010000  .*
