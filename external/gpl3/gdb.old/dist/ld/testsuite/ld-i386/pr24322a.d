#source: ../ld-x86-64/pr24322a.s
#source: ../ld-x86-64/pr24322b.s
#as: --32 -mx86-used-note=yes
#ld: -z shstk -m elf_i386
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x00000024	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 feature: SHSTK
	x86 ISA used: <None>
	x86 feature used: x86
