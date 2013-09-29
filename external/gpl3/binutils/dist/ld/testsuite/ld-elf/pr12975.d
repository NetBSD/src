#ld: --gc-sections -shared -version-script pr12975.t
#readelf: -s --wide
#target: *-*-linux* *-*-gnu*
#notarget: aarch64*-*-* arc-*-* d30v-*-* dlx-*-* i960-*-* or32-*-* pj*-*-*
#notarget: hppa64-*-* i370-*-* i860-*-* ia64-*-* mep-*-* mn10200-*-*
# generic linker targets don't support --gc-sections, nor do a bunch of others

#failif
#...
 +[0-9]+: +[0-9a-f]+ +[0-9]+ +FUNC +LOCAL +DEFAULT +[1-9]+ bar
#...
