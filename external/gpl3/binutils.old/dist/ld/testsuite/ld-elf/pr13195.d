#ld: --gc-sections -shared -version-script pr13195.t
#readelf: -s --wide -D
#target: *-*-linux* *-*-gnu*
#notarget: aarch64*-*-* arc-*-* d30v-*-* dlx-*-* i960-*-* or32-*-* pj*-*-*
#notarget: hppa64-*-* i370-*-* i860-*-* ia64-*-* mep-*-* mn10200-*-*
# generic linker targets don't support --gc-sections, nor do a bunch of others

#...
 +[0-9]+ +[0-9]+: +[0-9a-f]+ +[0-9]+ +FUNC +GLOBAL +DEFAULT +[1-9]+ foo
#pass
