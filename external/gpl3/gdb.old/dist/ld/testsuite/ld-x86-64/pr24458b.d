#source: pr24458.s
#as: --64 -mx86-used-note=yes
#ld: -pie -m elf_x86_64
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x[0-9a-f]+	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA used: <None>
	x86 feature used: x86
