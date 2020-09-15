#objdump: -r
#name: macro test 2
# darwin(mach-o) reverses the order of relocs.
#notarget: *-*-darwin*

.*: +file format .*

RELOCATION RECORDS FOR .*
OFFSET[ 	]+TYPE[ 	]+VALUE.*
0+00[ 	]+[a-zA-Z0-9_]+[ 	]+foo1
0+04[ 	]+[a-zA-Z0-9_]+[ 	]+foo2
0+08[ 	]+[a-zA-Z0-9_]+[ 	]+foo3
#pass
