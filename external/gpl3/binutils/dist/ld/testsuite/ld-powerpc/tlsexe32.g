#source: tls32.s
#as: -a32
#ld: tmpdir/libtlslib32.so
#objdump: -sj.got
#target: powerpc*-*-*

.*

Contents of section \.got:
.* 00000000 00000000 00000000 (0181034c|4c038101)  .*
.* 00000000 00000000  .*
