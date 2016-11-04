#source: tls.s
#as: -a64
#ld: tmpdir/libtlslib.so
#objdump: -sj.got
#target: powerpc64*-*-*

.*

Contents of section \.got:
 10010700 (00000000|00870110) (10018700|00000000) (ffffffff|1880ffff) (ffff8018|ffffffff)  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
