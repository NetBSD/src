#source: ifunc-14a.s
#source: ifunc-14b.s
#ld: -shared -z nocombreloc
#readelf: -d
#target: aarch64*-*-*

#failif
#...
.*\(TEXTREL\).*
#...
