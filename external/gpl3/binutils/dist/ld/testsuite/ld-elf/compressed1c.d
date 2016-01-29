#source: compress1.s
#as: --compress-debug-sections=zlib-gabi
#ld: -shared
#readelf: -t
#target: *-*-linux* *-*-gnu*

#failif
#...
  .*COMPRESSED.*
#...
