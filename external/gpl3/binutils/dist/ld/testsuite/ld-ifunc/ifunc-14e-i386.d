#source: ifunc-14a.s
#source: ifunc-14c.s
#source: ifunc-14b.s
#ld: -shared -m elf_i386 -z nocombreloc
#as: --32
#readelf: -r --wide
#target: x86_64-*-* i?86-*-*

#failif
#...
.* +R_386_NONE +.*
#...
