#source: orphan-region.s
#ld: -T orphan-region.ld -N
#readelf: -S -l --wide
#xfail: arc-*-* d30v-*-* dlx-*-* fr30-*-* frv-*-elf i860-*-* i960-*-*
#xfail: iq*-*-* mn10200-*-* moxie-*-* msp*-*-* mt-*-* or32-*-* pj*-*-*
#xfail: spu-*-* hppa*64*-*-* frv-*-*
# if not using elf32.em, you don't get fancy orphan handling
# spu twiddles LOAD range, hppa64 adds PHDR, frv-linux adds GNU_STACK

#...
  \[[ 0-9]+\] \.text[ \t]+PROGBITS[ \t]+0*40000000[ \t]+.*
  \[[ 0-9]+\] \.rodata[ \t]+PROGBITS[ \t]+0*400000[0-9a-f]+[ \t]+.*
  \[[ 0-9]+\] \.moredata[ \t]+PROGBITS[ \t]+0*400000[0-9a-f]+[ \t]+.*
#...
Program Headers:
  Type.*
  LOAD[ \t]+0x[0-9a-f]+ 0x0*40000000 0x0*40000000 0x[0-9a-f]+ 0x[0-9a-f]+ RWE 0x[0-9a-f]+

 Section to Segment mapping:
  Segment Sections...
   00     .text .rodata .moredata *
