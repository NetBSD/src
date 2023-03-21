#source: property-x86-3.s
#source: property-x86-shstk.s
#as: --32 -mx86-used-note=yes
#ld: -r -melf_i386
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x00000024	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA needed: SSE, SSE3, SSE4_1, AVX
	x86 ISA used: CMOV, SSE, SSSE3, SSE4_1
	x86 feature used: x86
