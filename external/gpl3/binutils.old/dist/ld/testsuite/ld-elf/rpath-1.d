#source: start.s
#readelf: -d -W
#ld: -shared -rpath .
#target: *-*-linux* *-*-gnu*

#failif
#...
 +0x[0-9a-f]+ +\(RUNPATH\) +Library runpath: +\[.\]
#...
