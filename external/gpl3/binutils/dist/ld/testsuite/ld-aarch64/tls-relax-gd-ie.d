#source: tls-relax-gd-ie.s
#ld: -T relocs.ld -e0
#objdump: -dr
#...
 +10000:	90000080 	adrp	x0, 20000 <var\+0x20000>
 +10004:	f9400000 	ldr	x0, \[x0\]
 +10008:	d53bd041 	mrs	x1, tpidr_el0
 +1000c:	8b000020 	add	x0, x1, x0
 +10010:	b9400000 	ldr	w0, \[x0\]
