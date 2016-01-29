#as: --64 -madd-bnd-prefix
#ld: -shared -melf_x86_64 -z bndplt
#objdump: -dw

#...
[ 	]*[a-f0-9]+:	f2 e8 f0 ff ff ff    	bnd callq 220 <\*ABS\*\+0x228@plt>
#pass
