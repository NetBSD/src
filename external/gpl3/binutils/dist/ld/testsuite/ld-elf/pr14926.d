#ld: -Ttext=0x60
#readelf: -S --wide
#notarget: d10v-*-* msp*-*-* visium-*-* xstormy*-*-*
# the above targets use memory regions that don't allow 0x60 for .text

#...
  \[[ 0-9]+\] \.text[ \t]+PROGBITS[ \t]+0*60[ \t]+.*
#pass
