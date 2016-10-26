#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux
#source: tls-ld-5.s
#source: tls128g.s
#source: tls-hx1x2.s
#objdump: -s -t -R -p

# DSO with two R_CRIS_16_DTPRELs against different hidden symbols.
# Check that we have proper NPTL/TLS markings and GOT.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+ paddr 0x0+ align 2\*\*13
         filesz 0x0+1a8 memsz 0x0+1a8 flags r-x
    LOAD off    0x0+1a8 vaddr 0x0+21a8 paddr 0x0+21a8 align 2\*\*13
         filesz 0x0+10c memsz 0x0+10c flags rw-
 DYNAMIC off    0x0+230 vaddr 0x0+2230 paddr 0x0+2230 align 2\*\*2
         filesz 0x0+70 memsz 0x0+70 flags rw-
     TLS off    0x0+1a8 vaddr 0x0+21a8 paddr 0x0+21a8 align 2\*\*2
         filesz 0x0+88 memsz 0x0+88 flags r--

Dynamic Section:
  HASH                 0x0+b4
  STRTAB               0x0+168
  SYMTAB               0x0+e8
  STRSZ                0x0+29
  SYMENT               0x0+10
  RELA                 0x0+194
  RELASZ               0x0+c
  RELAENT              0x0+c
private flags = 0:

SYMBOL TABLE:
#...
0+84 l       \.tdata	0+4 x2
0+80 l       \.tdata	0+4 x1
#...

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
0+22ac R_CRIS_DTPMOD     \*ABS\*

Contents of section \.hash:
#...
Contents of section \.text:
 01a0 5fae8000 5fbe8400              .*
#...
Contents of section \.got:
 22a0 30220+ 0+ 0+ 0+  .*
 22b0 0+                             .*
