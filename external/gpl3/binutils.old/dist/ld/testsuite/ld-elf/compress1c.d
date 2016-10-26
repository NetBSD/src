#source: compress1.s
#as: --compress-debug-sections
#ld: -shared
#readelf: -S --wide
#target: *-*-linux* *-*-gnu*

#failif
#...
  \[[ 0-9]+\] \.zdebug_.*[ 	]+(PROGBITS|MIPS_DWARF)[ 	0-9a-z]+ .*
#...
