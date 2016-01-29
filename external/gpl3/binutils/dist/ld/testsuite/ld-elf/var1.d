#ld:  $srcdir/$subdir/var1.t --sort-section name
#nm: -n

#...
[0-9a-f]* D var1
#...
[0-9a-f]*3 A var2
#...
[0-9a-f]* D var3
#pass
