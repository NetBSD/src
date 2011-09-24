#source: unknown2.s
#ld: -shared
#readelf: -S
#target: *-*-linux* *-*-gnu*

#...
  \[[ 0-9]+\] \.note.foo[ \t]+NOTE[ \t]+.*
#pass
