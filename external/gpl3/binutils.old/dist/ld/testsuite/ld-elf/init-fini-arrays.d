#source: init-fini-arrays.s
#ld: -r
#readelf: -S --wide
#xfail: cr16-*-* crx-*-*
# cr16 and crx use non-standard scripts with memory regions, which don't play
# well with unique group sections under ld -r.

#...
  \[[ 0-9]+\] \.init_array\.01000[ \t]+PROGBITS[ \t0-9a-f]+WA?.*
#...
  \[[ 0-9]+\] \.fini_array\.01000[ \t]+PROGBITS[ \t0-9a-f]+WA?.*
#pass
