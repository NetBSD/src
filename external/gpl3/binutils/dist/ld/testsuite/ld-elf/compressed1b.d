#source: compress1.s
#as: --compress-debug-sections=zlib-gabi
#ld: -r
#readelf: -t

#failif
#...
  .*COMPRESSED.*
#...
