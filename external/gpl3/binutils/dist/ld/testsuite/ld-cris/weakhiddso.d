#source: weakhid.s
#as: --pic --no-underscore --em=criself
#ld: --shared -m crislinux -z nocombreloc
#objdump: -s -R -T

# Check that .weak and .weak .hidden object references are handled
# correctly when generating a DSO.  For now, we have to live with the
# R_CRIS_NONE entry.

.*:     file format elf32-cris

DYNAMIC SYMBOL TABLE:
0+2214 l    d  \.data	0+ .data
0+2214 g    DO \.data	0+c x
0+      D  \*UND\*	0+ xregobj
0+2220 g    D  \*ABS\*	0+ __bss_start
0+  w   D  \*UND\*	0+ xweakobj
0+2220 g    D  \*ABS\*	0+ _edata
0+2220 g    D  \*ABS\*	0+ _end


DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE 
0+ R_CRIS_NONE       \*ABS\*
0+2218 R_CRIS_32         xweakobj
0+221c R_CRIS_32         xregobj

Contents of section \.hash:
#...
Contents of section .data:
 2214 00000000 00000000 00000000           .*
