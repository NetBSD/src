#source: ifunc-14b.s
#source: ifunc-14a.s
#ld: -shared -z nocombreloc
#readelf: -r --wide
#target: aarch64*-*-*

#failif
#...
.* +R_AARCH64_NONE +.*
#...
