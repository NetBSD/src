#source: group1a.s
#source: group1b.s
#ld: -T group.ld
#readelf: -s
Symbol table '.symtab' contains .* entries:
#...
    .*: 0[0]*1000     0 (NOTYPE|OBJECT)  WEAK   DEFAULT    . foo
    .*: 0[0]*0000     0 (NOTYPE|OBJECT)  GLOBAL DEFAULT  UND bar
#...
