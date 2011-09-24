#source: exclude3.s
#ld: --shared
#readelf: -S --wide
#target: ia64-*-*

#...
[ 	]*\[.*\][ 	]+\.got.*[ 	]+PROGBITS.*
[ 	]*\[.*\][ 	]+.*STRTAB.*
#pass
