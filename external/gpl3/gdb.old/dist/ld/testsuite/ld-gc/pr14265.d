#name: --gc-sections with KEEP
#source: dummy.s
#ld: --gc-sections -T pr14265.t -e 0 tmpdir/pr14265.o
#nm: --format=bsd --numeric-sort

#...
[0-9a-f]+[ 	][dD][ 	]_*foo1_start
#...
[0-9a-f]+[ 	]D[ 	]_*foo1
#...
[0-9a-f]+[ 	][dD][ 	]_*foo1_end
[0-9a-f]+[ 	][dD][ 	]_*foo2_start
[0-9a-f]+[ 	]D[ 	]_*foo2
[0-9a-f]+[ 	][dD][ 	]_*foo2_end
#...
