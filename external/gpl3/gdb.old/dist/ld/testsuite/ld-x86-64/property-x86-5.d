#source: property-x86-5a.s
#source: property-x86-5b.s
#as: --64 -defsym __64_bit__=1
#ld: -m elf_x86_64
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x[0-9a-f]+	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA needed: CMOV
	x86 ISA used: <None>
