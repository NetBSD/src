#ld: -Tdefined5.t
#nm: -B
#source: defined5.s
#xfail: rs6000-*-aix*

# Check that arithmetic on DEFINED works.
#...
0+1000 D defined
#pass
