#ld: -shared
#readelf: -W -x .strtab
#target: *-*-linux* *-*-gnu*

#failif
#...
 +0x[0-9 ]+.*\.xxxx\..*
#...
