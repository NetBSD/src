#ld: -Tdefined5.t
#nm: -B
#source: defined5.s
#xfail: [is_xcoff_format]
# xcoff outputs value of "defined" from the object file

# Check that arithmetic on DEFINED works.
#...
0+1000 D defined
#pass
