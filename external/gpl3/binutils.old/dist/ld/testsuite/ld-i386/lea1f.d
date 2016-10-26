#source: lea1.s
#as: --32
#ld: -melf_i386
#readelf: -Sw

#failif
#...
[ 	]*\[.*\][ 	]+.*\.got .*
#...
