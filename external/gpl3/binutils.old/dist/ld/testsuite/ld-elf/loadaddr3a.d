#source: loadaddr.s
#ld: -T loadaddr3.t -z max-page-size=0x200000
#readelf: -l --wide
#target: *-*-linux* *-*-gnu*

#...
  LOAD +0x000000 0x0*00000000 0x0*00000000 0x0*0110 0x0*0110 R E 0x.*
  LOAD +0x000200 0x0*00000200 0x0*00000110 0x0*0010 0x0*0010 RW  0x.*
#pass
