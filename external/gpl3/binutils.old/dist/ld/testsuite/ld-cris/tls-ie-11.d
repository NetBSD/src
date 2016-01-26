#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux
#source: tls-ie-11.s
#source: tls128g.s
#source: tls-x1x2.s
#objdump: -s -t -R -p -T

# DSO with two R_CRIS_32_GOT_TPREL against different symbols.  Check
# that we have proper NPTL/TLS markings and GOT.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+ paddr 0x0+ align 2\*\*13
         filesz 0x0+1e4 memsz 0x0+1e4 flags r-x
    LOAD off    0x0+1e4 vaddr 0x0+21e4 paddr 0x0+21e4 align 2\*\*13
         filesz 0x0+114 memsz 0x0+114 flags rw-
 DYNAMIC off    0x0+26c vaddr 0x0+226c paddr 0x0+226c align 2\*\*2
         filesz 0x0+78 memsz 0x0+78 flags rw-
     TLS off    0x0+1e4 vaddr 0x0+21e4 paddr 0x0+21e4 align 2\*\*2
         filesz 0x0+88 memsz 0x0+88 flags r--

Dynamic Section:
  HASH                 0x0+b4
  STRTAB               0x0+190
  SYMTAB               0x0+f0
  STRSZ                0x0+2f
  SYMENT               0x0+10
  RELA                 0x0+1c0
  RELASZ               0x0+18
  RELAENT              0x0+c
  FLAGS                0x0+10
private flags = 0:

SYMBOL TABLE:
#...
0+84 g       \.tdata	0+4 x2
#...
0+80 g       \.tdata	0+4 x1
#...
DYNAMIC SYMBOL TABLE:
#...
0+84 g    D  \.tdata	0+4 x2
#...
0+80 g    D  \.tdata	0+4 x1
#...

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
0+22f0 R_CRIS_32_TPREL   x2
0+22f4 R_CRIS_32_TPREL   x1

Contents of section \.hash:
#...
Contents of section \.text:
 01d8 6fae1000 00006fbe 0c000000           .*
#...
Contents of section \.got:
 22e4 6c220+ 0+ 0+ 0+  .*
 22f4 00000000                             .*
