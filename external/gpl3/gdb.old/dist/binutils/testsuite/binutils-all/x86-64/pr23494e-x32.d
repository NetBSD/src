#PROG: objcopy
#source: pr23494c.s
#as: --x32 -mx86-used-note=yes
#objcopy: -O elf64-x86-64 --decompress-debug-sections
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x00000040	NT_GNU_PROPERTY_TYPE_0
      Properties: stack size: 0xffffffff
	x86 ISA needed: SSE4_1, AVX
	x86 ISA used: SSE, SSE3, SSE4_1, AVX
	x86 feature used: x86
