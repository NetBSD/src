#source: ifunc-14b.s
#source: ifunc-14a.s
#ld: -shared -z nocombreloc
#readelf: -d
#target: aarch64*-*-*

#failif
#...
.*\(TEXTREL\).*
#...
