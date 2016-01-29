#source: exclude3.s
#ld: --shared
#readelf: -S --wide
#target: *-*-linux* *-*-gnu*

#failif
#...
[ 	]*\[.*\][ 	]+\.foo1[ 	]+.*
#...
