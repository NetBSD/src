#name: PR ld/18176
#as: --64
#ld: -melf_x86_64 -shared -z relro -T pr18176.t -z max-page-size=0x200000 -z common-page-size=0x1000
#readelf: -l --wide
#target: x86_64-*-linux*

#...
  GNU_RELRO      0x04bd17 0x000000000024bd17 0x000000000024bd17 0x0022e9 0x0022e9 R   0x1
#pass
