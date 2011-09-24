#source: linkonce1a.s
#source: linkonce1b.s
#ld: -emit-relocs
#objdump: -r
#notarget: arc-*-* d30v-*-* dlx-*-* i960-*-* or32-*-* pj*-*-*
# generic elf targets don't emit relocs

.*:     file format .*

RELOCATION RECORDS FOR \[.debug_frame\]:
OFFSET[ 	]+TYPE[ 	]+VALUE[ 	]*
.*(NONE|unused|UNUSED).*\*ABS\*

#pass
