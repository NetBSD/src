#name: --gc-sections with __gxx_personality
#ld: --gc-sections -e main -L tmpdir -lpersonality
#nm: -n
#xfail: bfin-*-* cris*-*-* frv-*-* mn10300-*-* vax-*-* xtensa-*-* metag-*-*
# above targets don't support cfi

#failif
#...
.*gxx_personality.*
#...
