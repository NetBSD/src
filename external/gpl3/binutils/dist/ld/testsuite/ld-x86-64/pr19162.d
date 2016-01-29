#source: pr19162a.s
#source: pr19162b.s
#as: --64
#ld: -melf_x86_64 -shared -z max-page-size=0x200000 -z common-page-size=0x1000
#readelf: -l --wide
#target: x86_64-*-linux*

#...
  DYNAMIC        0x0001a8 0x00000000002001a8 0x00000000002001a8 0x0000b0 0x0000b0 RW  0x8
#pass
