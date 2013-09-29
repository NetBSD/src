#readelf: --sections
#name: label arithmetic with multiple same-name sections
# The RX port uses non-standard section names.
#not-target: rx-*

#...
[ 	]*\[.*\][ 	]+\.group[ 	]+GROUP.*
#...
[ 	]*\[.*\][ 	]+\.text[ 	]+PROGBITS.*
#...
[ 	]*\[.*\][ 	]+\.data[ 	]+PROGBITS.*
#...
[ 	]*\[.*\][ 	]+\.text[ 	]+PROGBITS.*
#pass
