#name: aarch64-farcall-b-plt
#source: farcall-b-plt.s
#as:
#ld: -shared
#objdump: -dr
#...

Disassembly of section .plt:

.* <foo@plt.*>:
.*:	a9bf7bf0 	stp	x16, x30, \[sp,#-16\]!
.*:	.* 	adrp	x16, .* <__foo_veneer\+.*>
.*:	.* 	ldr	x17, \[x16,#.*\]
.*:	.* 	add	x16, x16, #.*
.*:	d61f0220 	br	x17
.*:	d503201f 	nop
.*:	d503201f 	nop
.*:	d503201f 	nop

.* <foo@plt>:
.*:	.* 	adrp	x16, .* <__foo_veneer\+.*>
.*:	.* 	ldr	x17, \[x16,#.*\]
.*:	.* 	add	x16, x16, #.*
.*:	d61f0220 	br	x17

Disassembly of section .text:

.* <_start>:
	...
.*:	.* 	b	.* <__foo_veneer>
.*:	d65f03c0 	ret
.*:	.* 	b	.* <__foo_veneer\+.*>

.* <__foo_veneer>:
.*:	.* 	adrp	x16, 0 <foo@plt.*>
.*:	.* 	add	x16, x16, #.*
.*:	d61f0200 	br	x16
	...
