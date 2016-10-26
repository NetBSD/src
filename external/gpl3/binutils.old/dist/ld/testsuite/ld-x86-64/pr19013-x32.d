#source: pr19013.s
#as: --x32
#ld: --oformat elf32-i386 -m elf32_x86_64
#objdump: -s -j .rodata
#notarget: x86_64-*-nacl*

#...
 [0-9a-f]+ 02030041 42434400 +...ABCD. +
#pass
