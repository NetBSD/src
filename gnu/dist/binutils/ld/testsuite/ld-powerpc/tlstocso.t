#source: tlstoc.s
#as: -a64
#ld: -shared -melf64ppc
#objdump: -sj.tdata
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Contents of section \.tdata:
 10758 12345678 9abcdef0 23456789 abcdef01  .*
 10768 3456789a bcdef012 456789ab cdef0123  .*
 10778 56789abc def01234 6789abcd ef012345  .*
 10788 789abcde f0123456                    .*
