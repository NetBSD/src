#source: init-fini-arrays.s
#ld: -r
#readelf: -S --wide
#xfail: cr16-*-* crx-*-* msp430-*-*
# msp430 puts the init_array and fini_array inside the .rodata section.
# cr16 and crx use non-standard scripts with memory regions, which don't play
#  well with unique group sections under ld -r.

#...
  \[[ 0-9]+\] \.init_array\.01000[ \t]+PROGBITS[ \t0-9a-f]+WA?.*
#...
  \[[ 0-9]+\] \.fini_array\.01000[ \t]+PROGBITS[ \t0-9a-f]+WA?.*
#pass
