#source: tls.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#objdump: -sj.tdata
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Contents of section \.tdata:
 10010188 12345678 9abcdef0 23456789 abcdef01  .*
 10010198 3456789a bcdef012 456789ab cdef0123  .*
 100101a8 56789abc def01234 6789abcd ef012345  .*
 100101b8 789abcde f0123456 00c0ffee           .*
