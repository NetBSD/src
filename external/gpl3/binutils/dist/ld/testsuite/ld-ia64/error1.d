#source: error1.s
#ld: -unresolved-symbols=ignore-all
#readelf: -s

#...
[ 	]+[0-9]+:[ 	]+[0]+[ 	]+0[ 	]+NOTYPE[ 	]+GLOBAL DEFAULT[ 	]+UND[ 	]+foo
#pass
