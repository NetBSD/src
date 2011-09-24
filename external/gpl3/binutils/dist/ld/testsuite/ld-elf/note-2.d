#ld: -Tnote-2.t
#objcopy_linked_file: -R .foo 
#readelf: -l --wide
#xfail: hppa64-*-*
# hppa64 adds PHDR

#...
Program Headers:
  Type.*
  LOAD +0x[0-9a-f]+ .*
  NOTE +0x[0-9a-f]+ .*

#...
  Segment Sections...
   00[ \t]+.text *
   01[ \t]+.note *
#pass
