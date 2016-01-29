#readelf: -SW
#name: debug info in group section and non-group section with same name
#source: group2.s
# The RX port uses non-standard section names.
#not-target: rx-*

#...
[ 	]*\[.*\][ 	]+\.group[ 	]+GROUP.*
#...
[ 	]*\[.*\][ 	]+\.text\.startup[ 	]+PROGBITS.*[ 	]+AXG[ 	]+.*
[ 	]*\[.*\][ 	]+\.text\.startup[ 	]+PROGBITS.*[ 	]+AX[ 	]+.*
#pass
