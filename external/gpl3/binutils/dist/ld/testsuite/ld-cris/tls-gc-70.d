#source: start1.s
#source: tls128.s
#source: tls-gd-3.s
#source: tls-x.s
#as: --no-underscore --em=criself
#ld: -m crislinux --gc-sections
#objdump: -s -t -r -p

# An executable with a single R_CRIS_32_GD, with gc.  Check that we
# have nothing left but the start symbol and its code.  Can't get rid
# of the GOT just yet.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+80000 paddr 0x0+80000 align 2\*\*13
         filesz 0x0+78 memsz 0x0+78 flags r-x
    LOAD off    0x0+78 vaddr 0x0+82078 paddr 0x0+82078 align 2\*\*13
         filesz 0x0+c memsz 0x0+c flags rw-
private flags = 0:

SYMBOL TABLE:
0+80074 l    d  \.text	0+ \.text
0+82078 l    d  \.got	0+ \.got
0+82078 l     O \.got	0+ _GLOBAL_OFFSET_TABLE_
0+80074 g       \.text	0+ _start
0+82084 g       \.got	0+ __bss_start
0+82084 g       \.got	0+ _edata
0+820a0 g       \.got	0+ _end

Contents of section \.text:
 80074 41b20+                             .*
Contents of section \.got:
 82078 0+ 0+ 0+           .*
