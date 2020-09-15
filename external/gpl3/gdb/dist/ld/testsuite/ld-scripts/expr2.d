# ld: -T expr2.t
#nm: -B
#xfail: arm-*-*aout [is_xcoff_format]

#...
.* D defined
#pass
