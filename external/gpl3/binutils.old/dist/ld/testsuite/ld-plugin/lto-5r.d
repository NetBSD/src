#ld: -r tmpdir/lto-5a.o tmpdir/lto-5b.o
#source: dummy.s
#nm: -p

#...
[0-9a-f]+ C __gnu_lto_v.*
#pass
