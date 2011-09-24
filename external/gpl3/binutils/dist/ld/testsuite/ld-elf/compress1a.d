#source: compress1.s
#as: --compress-debug-sections
#ld: -e func_cu2
#readelf: -S --wide
#notarget: alpha-*

#failif
#...
  \[[ 0-9]+\] \.zdebug_.*[ 	]+(PROGBITS|MIPS_DWARF)[ 	0-9a-z]+ .*
#...
