#ld: --shared -melf_i386
#readelf: -S --wide
#as: --32

#...
[ 	]*\[.*\][ 	]+\.dynamic[ 	]+DYNAMIC.*
[ 	]*\[.*\][ 	]+.*STRTAB.*
#pass
