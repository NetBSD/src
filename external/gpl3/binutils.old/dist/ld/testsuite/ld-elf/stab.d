#source: start.s
#as: -gstabs
#readelf: -S --wide
#ld:
#notarget: "ia64-*-*" "alpha*"

# Disabled on alpha because the entry point may be above 4GB but the stabs
# value on 32 bits.

#...
  \[[0-9 ][0-9]\] \.stab +PROGBITS +0+ [0-9a-f]+ [0-9a-f]+ [0-9a-f]+ +[1-9]+ +0.*
#...
  \[[0-9 ][0-9]\] \.stabstr +STRTAB +0+ [0-9a-f]+ [0-9a-f]+ 00 +0 +0.*
#...
