#source: start.s
#source: pr19539.s
#ld: -pie -T pr19539.t --warn-textrel
#readelf : --dyn-syms --wide
#warning: .*: creating DT_TEXTREL in a PIE
#target: *-*-linux* *-*-gnu* *-*-solaris* arm*-*-uclinuxfdpiceabi
#xfail: ![check_pie_support]

Symbol table '\.dynsym' contains [0-9]+ entr(y|ies):
#pass
