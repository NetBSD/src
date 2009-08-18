#name: GOT page test 2
#source: got-page-2.s
#as: -EB -n32
#ld: -T got-page-1.ld -shared -melf32btsmipn32
#readelf: -d
#
# There should be 10 page entries and 2 reserved entries
#
#...
.* \(MIPS_LOCAL_GOTNO\) *12
#pass
