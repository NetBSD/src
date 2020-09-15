#ld: --gc-sections -T pr23648.t
#target: [check_gc_sections_available]
#xfail: frv-*-linux* lm32-*-linux*
#nm: -B

#failif
#...
.*test0
#...
