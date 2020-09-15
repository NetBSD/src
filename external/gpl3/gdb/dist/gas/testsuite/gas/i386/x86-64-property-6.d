#name: x86-64 property 6
#source: property-6.s
#as: -mx86-used-note=yes --generate-missing-build-notes=no
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x[0-9a-f]+	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA used: AVX512F
	x86 feature used: x86, XMM, YMM, ZMM
