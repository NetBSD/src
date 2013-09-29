#source: tls32.s
#as: -a32
#ld: tmpdir/libtlslib32.so
#objdump: -sj.got
#target: powerpc*-*-*

.*

Contents of section \.got:
.* 00000000 00000000 00000000 (4e800021|2100804e)  .*
.* (018102b8|b8028101) 00000000 00000000           .*
