#source: pr20244-2.s
#as: --32
#ld: -m elf_i386
#objdump: -s -j .got.plt
#notarget: i?86-*-nacl* x86_64-*-nacl*

.*: +file format .*

Contents of section .got.plt:
 80490d4 00000000 00000000 00000000 b0800408  ................
 80490e4 b1800408                             ....            
