#source: property-x86-empty.s
#source: property-x86-ibt.s
#source: property-x86-shstk.s
#source: property-x86-3.s
#as: --64 -defsym __64_bit__=1 -mx86-used-note=yes
#ld: -r -melf_x86_64 -z cet-report=warning
#warning: .*: warning: missing IBT and SHSTK properties.*: warning: missing SHSTK property.*: warning: missing IBT property.*: warning: missing IBT and SHSTK properties
#readelf: -n

Displaying notes found in: .note.gnu.property
[ 	]+Owner[ 	]+Data size[ 	]+Description
  GNU                  0x00000030	NT_GNU_PROPERTY_TYPE_0
      Properties: x86 ISA needed: CMOV, SSE, SSSE3, SSE4_1
	x86 ISA used: SSE, SSE3, SSE4_1, AVX
	x86 feature used: x86
