#source: pr13177.s
#ld: --gc-sections -shared
#readelf: -s -D --wide
#target: *-*-linux* *-*-gnu*
#notarget: arc-*-* d30v-*-* dlx-*-* i960-*-* pj*-*-*
#notarget: hppa64-*-* i370-*-* i860-*-* ia64-*-* mep-*-* mn10200-*-*
# generic linker targets don't support --gc-sections, nor do a bunch of others

#failif
#...
.*: 0+0 +0 +NOTYPE +GLOBAL +DEFAULT +UND bar
#...
