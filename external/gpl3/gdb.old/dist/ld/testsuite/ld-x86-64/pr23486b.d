#source: pr23486b.s
#source: pr23486a.s
#as: --64 -defsym __64_bit__=1 -mx86-used-note=no
#ld: -r -m elf_x86_64
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x00000010	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA needed: i486, 586
