#as: --64
#ld: --oformat elf32-i386 -m elf_x86_64
#objdump: -s -j .rodata
#notarget: x86_64-*-nacl*

#...
 [0-9a-f]+ 00000203 00414243 4400 +.....ABCD. +
#pass
