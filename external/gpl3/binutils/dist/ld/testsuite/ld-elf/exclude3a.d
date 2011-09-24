#source: exclude3.s
#ld:
#readelf: -S --wide
#target: x86_64-*-* i?86-*-* ia64-*-*

#...
[ 	]*\[.*\][ 	]+NULL.*
[ 	]*\[.*\][ 	]+\.text[ 	]+PROGBITS.*
[ 	]*\[.*\][ 	]+.*STRTAB.*
#pass
