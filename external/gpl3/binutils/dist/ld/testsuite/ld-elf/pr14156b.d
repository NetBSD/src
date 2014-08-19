#source: fini0.s
#source: fini1.s
#source: fini2.s
#source: fini3.s
#source: finin.s
#ld: --sort-section=alignment
#nm: -n

#...
[0-9a-f]+ T foo
[0-9a-f]+ t foo1
[0-9a-f]+ t foo2
[0-9a-f]+ t foo3
[0-9a-f]+ t last
#pass
