#source: tls128g.s
#source: tls-ld-5.s
#source: tls-gd-1.s
#source: tls-ldgd-14.s
#source: tls-x.s
#source: tls-z.s
#source: tls-hx1x2.s
#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux
#objdump: -s -t -R -p -T

# Check that we have proper NPTL/TLS markings and GOT for two
# R_CRIS_16_GOT_GD and two R_CRIS_16_DTPRELs against different
# variables in a DSO.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+ paddr 0x0+ align 2\*\*13
         filesz 0x0+230 memsz 0x0+230 flags r-x
    LOAD off    0x0+230 vaddr 0x0+2230 paddr 0x0+2230 align 2\*\*13
         filesz 0x0+124 memsz 0x0+124 flags rw-
 DYNAMIC off    0x0+2c0 vaddr 0x0+22c0 paddr 0x0+22c0 align 2\*\*2
         filesz 0x0+70 memsz 0x0+70 flags rw-
     TLS off    0x0+230 vaddr 0x0+2230 paddr 0x0+2230 align 2\*\*2
         filesz 0x0+90 memsz 0x0+90 flags r--

Dynamic Section:
  HASH                 0x0+b4
  STRTAB               0x0+1b8
  SYMTAB               0x0+f8
  STRSZ                0x0+42
  SYMENT               0x0+10
  RELA                 0x0+1fc
  RELASZ               0x0+24
  RELAENT              0x0+c
private flags = 0:

SYMBOL TABLE:
#...
0+8c l       \.tdata	0+4 x2
#...
0+88 l       \.tdata	0+4 x1
#...
0+80 g       \.tdata	0+4 x
#...
0+84 g       \.tdata	0+4 z
#...
DYNAMIC SYMBOL TABLE:
#...
0+80 g    D  \.tdata	0+4 x
#...
0+84 g    D  \.tdata	0+4 z
#...

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
0+233c R_CRIS_DTPMOD     \*ABS\*
0+2344 R_CRIS_DTP        x
0+234c R_CRIS_DTP        z

Contents of section \.hash:
#...
Contents of section \.text:
 0220 5fae8800 5fbe8c00 5fae1400 5fae1c00  .*
Contents of section .tdata:
#...
Contents of section \.got:
 2330 c0220+ 0+ 0+ 0+  .*
 2340 0+ 0+ 0+ 0+  .*
 2350 0+                             .*
