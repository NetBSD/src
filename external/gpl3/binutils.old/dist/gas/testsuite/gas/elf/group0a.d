#readelf: -SW
#name: group section
#source: group0.s

#...
[ 	]*\[.*\][ 	]+\.group[ 	]+GROUP.*
#...
[ 	]*\[.*\][ 	]+\.foo[ 	]+PROGBITS.*[ 	]+AXG[ 	]+.*
[ 	]*\[.*\][ 	]+\.bar[ 	]+PROGBITS.*[ 	]+AG[ 	]+.*
#pass
