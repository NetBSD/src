#source: dso-1.s
#as: --pic --no-underscore --march=v32 --em=criself
#ld: --shared -m crislinux
#objdump: -p -h

# Sanity check; just an empty GOT.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+ paddr 0x0+ align 2\*\*13
         filesz 0x0+188 memsz 0x0+188 flags r-x
    LOAD off    0x0+1a0 vaddr 0x0+21a0 paddr 0x0+21a0 align 2\*\*13
         filesz 0x0+64 memsz 0x0+80 flags rw-
 DYNAMIC off    0x0+1a0 vaddr 0x0+21a0 paddr 0x0+21a0 align 2\*\*2
         filesz 0x0+58 memsz 0x0+58 flags rw-
Dynamic Section:
  HASH        0x94
  STRTAB      0x15c
  SYMTAB      0xcc
  STRSZ       0x28
  SYMENT      0x10
private flags = 2: \[v32\]
Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 \.hash         0+38  0+94  0+94  0+94  2\*\*2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  1 \.dynsym       0+90  0+cc  0+cc  0+cc  2\*\*2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 \.dynstr       0+28  0+15c  0+15c  0+15c  2\*\*0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 \.text         0+4  0+184  0+184  0+184  2\*\*0
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  4 \.dynamic      0+58  0+21a0  0+21a0  0+1a0  2\*\*2
                  CONTENTS, ALLOC, LOAD, DATA
  5 \.got          0+c  0+21f8  0+21f8  0+1f8  2\*\*2
                  CONTENTS, ALLOC, LOAD, DATA
  6 \.data         0+  0+2204  0+2204  0+204  2\*\*0
                  CONTENTS, ALLOC, LOAD, DATA
  7 \.bss          0+1c  0+2204  0+2204  0+204  2\*\*0
                  ALLOC
