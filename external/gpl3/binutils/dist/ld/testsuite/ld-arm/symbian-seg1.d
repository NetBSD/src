#source: symbian-seg1.s
#ld: -Ttext 0x10000 -Tdata 0x400000
#objdump: -dR
#...
 +10000:	00400000 	.word	0x00400000
	+10000: R_ARM_RELATIVE	.data
 +10004:	00010008 	.word	0x00010008
	+10004: R_ARM_RELATIVE	.text
