#source: gotop32.s
#as: --32 -K PIC
#ld: -shared -melf32_sparc
#objdump: -sj.data
#target: sparc*-*-*

.*: +file format elf32-sparc

Contents of section .data:
 13000 12345678 deadbeef 00000000 00000000  .*
 13010 00000000 00000000 00000000 00000000  .*
#pass
