#readelf: -SW
#name: group section with multiple sections of same name
#source: group1.s
# The RX port uses non-standard section names.
#not-target: rx-*

#...
[ 	]*\[.*\][ 	]+\.group[ 	]+GROUP.*
#...
[ 	]*\[.*\][ 	]+\.text[ 	]+PROGBITS.*[ 	]+AX[ 	]+.*
#...
[ 	]*\[.*\][ 	]+\.text[ 	]+PROGBITS.*[ 	]+AXG[ 	]+.*
#pass
