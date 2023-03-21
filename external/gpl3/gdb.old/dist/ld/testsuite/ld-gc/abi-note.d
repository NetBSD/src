#name: --gc-sections with note section
#ld: --gc-sections -e _start
#readelf: -S --wide
#target: *-*-linux* *-*-gnu* arm*-*-uclinuxfdpiceabi

#...
.* .note.ABI-tag[ 	]+NOTE.*
#...
