#source: init0.s
#source: init1.s
#source: init2.s
#source: init3.s
#source: initn.s
#ld: --sort-section=alignment
#nm: -n

#...
[0-9a-f]+ T foo
[0-9a-f]+ t foo1
[0-9a-f]+ t foo2
[0-9a-f]+ t foo3
[0-9a-f]+ t last
#pass
