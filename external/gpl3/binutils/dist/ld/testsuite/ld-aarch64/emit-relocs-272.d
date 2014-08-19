#source: emit-relocs-272.s
#ld: -T relocs.ld --defsym tempy=0x1012 --defsym tempy2=-12345678912345 --defsym tempy3=-292  -e0 --emit-relocs
#objdump: -dr -Mno-aliases

#...
 +10000:	8a000000 	and	x0, x0, x0
 +10004:	92400000 	and	x0, x0, #0x1
 +10008:	d2c00004 	movz	x4, #0x0, lsl #32
	+10008: R_AARCH64_MOVW_SABS_G2	tempy
 +1000c:	92c16747 	movn	x7, #0xb3a, lsl #32
	+1000c: R_AARCH64_MOVW_SABS_G2	tempy2
 +10010:	92c00011 	movn	x17, #0x0, lsl #32
	+10010: R_AARCH64_MOVW_SABS_G2	tempy3

