#source: pr23486d.s
#source: pr23486c.s
#as: --x32
#ld: -r -m elf32_x86_64
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x0000000c	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA needed: CMOV, SSE
