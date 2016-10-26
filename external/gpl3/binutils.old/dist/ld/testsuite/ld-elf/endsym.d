#source: start.s
#source: endsym.s
#ld: --sort-common
#nm: -n
#notarget: hppa*-*-hpux*

#...
.* end
#...
.* end2
#...
.* _?_end
#pass
