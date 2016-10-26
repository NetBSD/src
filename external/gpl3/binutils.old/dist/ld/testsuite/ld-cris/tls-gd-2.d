#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux
#source: tls-gd-2.s
#source: tls128g.s
#source: tls-x.s
#objdump: -s -t -R -p -T

# DSO with a single R_CRIS_32_GOT_GD.  Check that we have proper
# NPTL/TLS markings and GOT.

.*:     file format elf32-cris

Program Header:
    LOAD off    0x0+ vaddr 0x0+ paddr 0x0+ align 2\*\*13
         filesz 0x0+1bc memsz 0x0+1bc flags r-x
    LOAD off    0x0+1bc vaddr 0x0+21bc paddr 0x0+21bc align 2\*\*13
         filesz 0x0+108 memsz 0x0+108 flags rw-
 DYNAMIC off    0x0+240 vaddr 0x0+2240 paddr 0x0+2240 align 2\*\*2
         filesz 0x0+70 memsz 0x0+70 flags rw-
     TLS off    0x0+1bc vaddr 0x0+21bc paddr 0x0+21bc align 2\*\*2
         filesz 0x0+84 memsz 0x0+84 flags r--

Dynamic Section:
  HASH                 0x0+b4
  STRTAB               0x0+17c
  SYMTAB               0x0+ec
  STRSZ                0x0+2c
  SYMENT               0x0+10
  RELA                 0x0+1a8
  RELASZ               0x0+c
  RELAENT              0x0+c
private flags = 0:

SYMBOL TABLE:
#...
0+80 g       \.tdata	0+4 x
#...
DYNAMIC SYMBOL TABLE:
#...
0+80 g    D  \.tdata	0+4 x
#...

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
0+22bc R_CRIS_DTP        x

Contents of section \.hash:
#...
Contents of section \.text:
 01b4 6fae0c00 00000000              .*
#...
Contents of section \.got:
 22b0 40220+ 0+ 0+ 0+  .*
 22c0 0+                             .*
