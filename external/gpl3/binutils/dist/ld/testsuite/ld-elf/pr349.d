#source: pr349-1.s
#source: pr349-2.s
#ld: -r
#readelf: -S
#xfail: arc-*-* d30v-*-* dlx-*-* fr30-*-* frv-*-elf i860-*-* i960-*-*
#xfail: iq*-*-* mn10200-*-* moxie-*-* msp*-*-* mt-*-* or32-*-* pj*-*-*
# if not using elf32.em, you don't get fancy section handling

#...
.* .abcxyz .*
#...
.* .abcxyz .*
#pass
