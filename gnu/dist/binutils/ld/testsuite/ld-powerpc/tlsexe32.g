#source: tls32.s
#as: -a32
#ld: -melf32ppc tmpdir/libtlslib32.so
#objdump: -sj.got
#target: powerpc*-*-*

.*: +file format elf32-powerpc

Contents of section \.got:
.* 4e800021 018102d0 00000000 00000000  .*
.* 00000000 00000000 00000000           .*
