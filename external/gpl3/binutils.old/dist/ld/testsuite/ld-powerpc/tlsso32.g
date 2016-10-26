#source: tls32.s
#as: -a32
#ld: -shared
#objdump: -sj.got
#target: powerpc*-*-*

.*

Contents of section \.got:
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 (0001044c|4c040100) 00000000 00000000  .*
