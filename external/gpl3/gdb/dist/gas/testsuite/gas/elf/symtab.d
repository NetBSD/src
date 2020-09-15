# The Alpha has its own version of .set.
# The HPPA does not output non-global absolute symbols.
#skip: alpha-*-* hppa*-*-*
#readelf: -s
#name: .set with expression

#...
.*ABS.*shift.*
#pass
