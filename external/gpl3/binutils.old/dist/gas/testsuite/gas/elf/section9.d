#readelf: -S --wide
#name: section flags

#...
[ 	]*\[.*\][ 	]+\.gnu\.lto_main[ 	]+PROGBITS.*[ 	]+E[   ]+.*
[ 	]*\[.*\][ 	]+\.gnu\.lto_\.pureconst[ 	]+PROGBITS.*[ 	]+E[   ]+.*
#pass
