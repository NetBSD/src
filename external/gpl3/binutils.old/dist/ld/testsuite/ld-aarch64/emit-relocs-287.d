#source: emit-relocs-287.s
#ld: -T relocs.ld --defsym tempy=0x11000 --defsym tempy2=0x45000 --defsym tempy3=0x1234 --defsym _GOT_=0x10000 -e0 --emit-relocs
#objdump: -dr

#...
 +10000:	8a000000 	and	x0, x0, x0
 +10004:	92400000 	and	x0, x0, #0x1
 +10008:	d2820004 	movz	x4, #0x1000
	+10008: R_AARCH64_MOVW_PREL_G0	_GOT_
 +1000c:	d28a0007 	movz	x7, #0x5000
	+1000c: R_AARCH64_MOVW_PREL_G0	_GOT_
 +10010:	d2824691 	movz	x17, #0x1234
	+10010: R_AARCH64_MOVW_PREL_G0	_GOT_

