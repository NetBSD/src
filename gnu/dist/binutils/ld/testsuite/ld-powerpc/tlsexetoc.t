#source: tlstoc.s
#as: -a64
#ld: -melf64ppc tmpdir/libtlslib.so
#objdump: -sj.tdata
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Contents of section \.tdata:
 10010410 12345678 9abcdef0 23456789 abcdef01  .*
 10010420 3456789a bcdef012 456789ab cdef0123  .*
 10010430 56789abc def01234 6789abcd ef012345  .*
 10010440 789abcde f0123456                    .*
