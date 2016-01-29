#source: start.s
#readelf: -d -W
#ld: -shared -rpath .
#target: *-*-linux* *-*-gnu*

#...
 +0x[0-9a-f]+ +\(RPATH\) +Library rpath: +\[.\]
#pass
