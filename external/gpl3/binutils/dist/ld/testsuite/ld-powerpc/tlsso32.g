#source: tls32.s
#as: -a32
#ld: -shared
#objdump: -sj.got
#target: powerpc*-*-*

.*

Contents of section \.got:
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 (4e800021|2100804e) (000103ec|ec030100) 00000000  .*
.* 00000000                             .*
