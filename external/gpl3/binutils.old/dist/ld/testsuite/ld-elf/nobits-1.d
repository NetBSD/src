#ld: -Tnobits-1.t
#readelf: -l --wide
#xfail: hppa64-*-*
# hppa64 adds PHDR

#...
 Section to Segment mapping:
  Segment Sections...
   00     .foo .bar 
