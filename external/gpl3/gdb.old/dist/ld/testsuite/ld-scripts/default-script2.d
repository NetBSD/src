#source: default-script.s
#ld: -T default-script.t -defsym _START=0x800
#nm: -n
#xfail: {[is_pecoff_format x86_64-*]}
# Skipped on Mingw64 and Cygwin because the image base defaults to 0x100000000

#...
0*800 . _START
#...
0*900 T text
#pass
