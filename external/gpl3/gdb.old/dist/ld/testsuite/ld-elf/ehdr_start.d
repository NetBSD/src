#source: ehdr_start.s
#ld: -e _start --build-id
#nm: -n
#target: *-*-linux* *-*-gnu* *-*-nacl* arm*-*-uclinuxfdpiceabi
#xfail: frv-*-*

#...
[0-9a-f]*000 [Adrt] __ehdr_start
#pass
