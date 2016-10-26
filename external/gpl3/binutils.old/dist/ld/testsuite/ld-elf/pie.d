#source: start.s
#readelf: -d -W
#ld: -pie
#target: *-*-linux* *-*-gnu* *-*-solaris*

#...
 +0x[0-9a-f]+ +\(FLAGS_1\) +Flags: +PIE
#pass
