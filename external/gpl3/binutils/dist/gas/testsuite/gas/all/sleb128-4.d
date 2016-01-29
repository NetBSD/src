#objdump : -s -j .data -j "\$DATA\$"
#name : .sleb128 tests (4)
#skip: msp430*-*-*

.*: .*

Contents of section (\.data|\$DATA\$):
 .* 83808080 082a.*
