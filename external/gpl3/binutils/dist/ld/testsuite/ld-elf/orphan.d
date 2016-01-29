#source: orphan.s
#ld: -T orphan.ld
#readelf: -S --wide
#xfail: arc-*-* d30v-*-* dlx-*-* fr30-*-* frv-*-elf i860-*-* i960-*-*
#xfail: iq*-*-* mn10200-*-* moxie-*-* msp*-*-* mt-*-* pj*-*-*
# if not using elf32.em, you don't get fancy orphan handling

#...
  \[[ 0-9]+\] \.(text|notbad)[ \t]+PROGBITS[ \t0-9a-f]+AX?.*
#...
  \[[ 0-9]+\] \.(text|notbad)[ \t]+PROGBITS[ \t0-9a-f]+AX?.*
  \[[ 0-9]+\] \.data[ \t]+PROGBITS[ \t0-9a-f]+WA.*
#...
  \[[ 0-9]+\] \.note[ \t]+NOTE[ \t0-9a-f]+A.*
  \[[ 0-9]+\] \.note.bar[ \t]+NOTE[ \t0-9a-f]+A.*
#pass
