#source: emit-relocs-271.s
#ld: -T relocs.ld --defsym tempy=0x1012 --defsym tempy2=0x674500 --defsym tempy3=-292  -e0 --emit-relocs
#objdump: -dr -Mno-aliases

#...
 +10000:	8a000000 	and	x0, x0, x0
 +10004:	92400000 	and	x0, x0, #0x1
 +10008:	d2a00004 	movz	x4, #0x0, lsl #16
	+10008: R_AARCH64_MOVW_SABS_G1	tempy
 +1000c:	d2a00ce7 	movz	x7, #0x67, lsl #16
	+1000c: R_AARCH64_MOVW_SABS_G1	tempy2
 +10010:	92a00011 	movn	x17, #0x0, lsl #16
	+10010: R_AARCH64_MOVW_SABS_G1	tempy3

