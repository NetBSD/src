#source: tls-relax-gdesc-ie.s
#ld: -T relocs.ld -e0
#objdump: -dr
#...
 +10000:	90000080 	adrp	x0, 20000 <_GLOBAL_OFFSET_TABLE_>
 +10004:	f9400400 	ldr	x0, \[x0, #8\]
 +10008:	d503201f 	nop
 +1000c:	d503201f 	nop
 +10010:	d53bd041 	mrs	x1, tpidr_el0
 +10014:	8b000020 	add	x0, x1, x0
 +10018:	b9400000 	ldr	w0, \[x0\]
