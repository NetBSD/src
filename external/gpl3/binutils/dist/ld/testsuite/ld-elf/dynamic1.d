#ld: -shared -T dynamic1.ld
#readelf: -l --wide
#target: *-*-linux* *-*-gnu*

#...
 Section to Segment mapping:
  Segment Sections...
#...
   0[1-9]     .dynamic[ 	]*
#pass
