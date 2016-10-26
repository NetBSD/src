#source: start.s
#readelf: -d -W
#ld: -shared -rpath . --enable-new-dtags
#target: *-*-linux* *-*-gnu*

#...
 +0x[0-9a-f]+ +\(RUNPATH\) +Library runpath: +\[.\]
#pass
