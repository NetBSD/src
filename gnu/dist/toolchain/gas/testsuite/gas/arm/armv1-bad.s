	.global entry
	.text
entry:
	str	r0, =0x00ff0000
	ldr	r0, {r1}
	ldr	r0, [r1, #4096]
	ldr	r0, [r1, #-4096]
	mov	r0, #0x1ff
	cmpl	r0, r0
	strh	r0, [r1]
