#source: empty.s
#ld: -shared
#readelf: --dyn-syms
#target: *-*-linux* *-*-gnu*

#...
 +[0-9]+: +[0-9a-f]+ +[0-9]+ +FUNC +GLOBAL +DEFAULT +[1-9] _start
#pass
