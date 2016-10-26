#source: lea1.s
#as: --32
#ld: -Bsymbolic -shared -melf_i386
#readelf: -Sw

#failif
#...
[ 	]*\[.*\][ 	]+.*\.got .*
#...
