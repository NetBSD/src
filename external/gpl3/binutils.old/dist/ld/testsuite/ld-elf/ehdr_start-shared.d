#source: ehdr_start.s
#ld: -e _start -shared
#nm: -n
#target: *-*-linux* *-*-gnu* *-*-nacl*
#xfail: cris*-*-* frv-*-*

#...
[0-9a-f]*000 [Adrt] __ehdr_start
#pass
