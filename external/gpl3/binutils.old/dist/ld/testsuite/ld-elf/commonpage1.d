#source: maxpage1.s
#ld: -z max-page-size=0x200000 -z common-page-size=0x100000
#readelf: -l --wide
#target: *-*-linux* *-*-gnu*

#...
  LOAD+.*0x200000
  LOAD+.*0x200000
#pass
