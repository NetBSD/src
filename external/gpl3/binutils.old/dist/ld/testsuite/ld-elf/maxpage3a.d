#source: maxpage1.s
#ld: -z max-page-size=0x10000000 -T maxpage3.t
#readelf: -lS --wide
#target: *-*-linux* *-*-gnu*

#...
  \[[ 0-9]+\] \.data[ \t]+PROGBITS[ \t]+0*10000000[ \t]+[ \t0-9a-f]+WA?.*
#...
  LOAD+.*0x10000000
  LOAD+.*0x10000000
#pass
