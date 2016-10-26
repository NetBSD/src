#source: error1.s
#ld: -pie -unresolved-symbols=ignore-all
#readelf: -s

#...
[ 	]+[0-9]+:[ 	]+[0]+[ 	]+0[ 	]+NOTYPE[ 	]+GLOBAL DEFAULT[ 	]+UND[ 	]+foo
#pass
