#ld: -shared -z relro
#readelf: -l --wide
#target: *-*-linux-gnu *-*-gnu* *-*-nacl*

#...
  GNU_RELRO .*
#pass
