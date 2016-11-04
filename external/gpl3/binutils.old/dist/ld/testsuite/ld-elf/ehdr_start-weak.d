#source: ehdr_start.s
#ld: -e _start -T ehdr_start-missing.t
#nm: -n
#target: *-*-linux* *-*-gnu* *-*-nacl*
#xfail: frv-*-*

#...
\s+[wU] __ehdr_start
#pass
