#source: error1.s
#ld: -pie -shared
#readelf: -s

#...
[ 	]+[0-9]+:[ 	]+[0]+[ 	]+0[ 	]+NOTYPE[ 	]+GLOBAL DEFAULT[ 	]+UND[ 	]+foo
#pass
