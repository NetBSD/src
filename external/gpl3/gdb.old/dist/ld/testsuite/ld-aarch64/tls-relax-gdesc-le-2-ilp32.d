#source: tls-relax-gdesc-le-2.s
#as: -mabi=ilp32
#ld: -m [aarch64_choose_ilp32_emul] -T relocs-ilp32.ld -e0
#objdump: -dr
#...
 +10000:	52a00000 	movz	w0, #0x0, lsl #16
 +10004:	d503201f 	nop
 +10008:	d503201f 	nop
 +1000c:	72800100 	movk	w0, #0x8
 +10010:	d503201f 	nop
 +10014:	d503201f 	nop
 +10018:	d503201f 	nop
 +1001c:	d503201f 	nop
 +10020:	d503201f 	nop
 +10024:	d503201f 	nop
 +10028:	d503201f 	nop
 +1002c:	d53bd041 	mrs	x1, tpidr_el0
 +10030:	8b000020 	add	x0, x1, x0
 +10034:	b9400000 	ldr	w0, \[x0\]
