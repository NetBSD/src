#source: exclude3.s
#ld: --shared
#readelf: -S --wide
#target: x86_64-*-* i?86-*-*

#...
[ 	]*\[.*\][ 	]+\.dynamic[ 	]+DYNAMIC.*
[ 	]*\[.*\][ 	]+.*STRTAB.*
#pass
