#as: -I$srcdir/$subdir
#nm: -n
#name: included file with .if 0 wrapped in APP/NO_APP, no final NO_APP, macro in main file
#...
0+ T d
#...
0+[1-f] T a
#...
0+[2-f] T b
#pass
