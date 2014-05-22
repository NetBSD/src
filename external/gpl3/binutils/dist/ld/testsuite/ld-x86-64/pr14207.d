#name: PR ld/14207
#as: --64
#ld: -melf_x86_64 -shared -z relro -z now
#readelf: -l --wide

#failif
#...
  NULL +.*
#...
