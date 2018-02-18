#include "sparc_arch.h"

#define LOCALS (STACK_BIAS+STACK_FRAME)

.text

.globl	aes_fx_encrypt
.align	32
aes_fx_encrypt:
	and		%o0, 7, %o4		! is input aligned?
	andn		%o0, 7, %o0
	ldd		[%o2 +  0], %f6	! round[0]
	ldd		[%o2 +  8], %f8
	mov		%o7, %g1
	ld		[%o2 + 240], %o3

1:	call		.+8
	add		%o7, .Linp_align-1b, %o7

	sll		%o4, 3, %o4
	ldd		[%o0 + 0], %f0		! load input
	brz,pt		%o4, .Lenc_inp_aligned
	ldd		[%o0 + 8], %f2

	ldd		[%o7 + %o4], %f14	! shift left params
	ldd		[%o0 + 16], %f4
	.word	0x81b81d62 !fshiftorx	%f0,%f2,%f14,%f0
	.word	0x85b89d64 !fshiftorx	%f2,%f4,%f14,%f2

.Lenc_inp_aligned:
	ldd		[%o2 + 16], %f10	! round[1]
	ldd		[%o2 + 24], %f12

	.word	0x81b00d86 !fxor	%f0,%f6,%f0		! ^=round[0]
	.word	0x85b08d88 !fxor	%f2,%f8,%f2
	ldd		[%o2 + 32], %f6	! round[2]
	ldd		[%o2 + 40], %f8
	add		%o2, 32, %o2
	sub		%o3, 4, %o3

.Loop_enc:
	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%o2 + 16], %f10
	ldd		[%o2 + 24], %f12
	add		%o2, 32, %o2

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%o2 +  0], %f6
	ldd		[%o2 +  8], %f8

	brnz,a		%o3, .Loop_enc
	sub		%o3, 2, %o3

	andcc		%o1, 7, %o4		! is output aligned?
	andn		%o1, 7, %o1
	mov		0xff, %o5
	srl		%o5, %o4, %o5
	add		%o7, 64, %o7
	sll		%o4, 3, %o4

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%o7 + %o4], %f14	! shift right params

	fmovd		%f0, %f4
	.word	0x81b09246 !faesenclx	%f2,%f6,%f0
	.word	0x85b11248 !faesenclx	%f4,%f8,%f2

	bnz,pn		%icc, .Lenc_out_unaligned
	mov		%g1, %o7

	std		%f0, [%o1 + 0]
	retl
	std		%f2, [%o1 + 8]

.align	16
.Lenc_out_unaligned:
	add		%o1, 16, %o0
	orn		%g0, %o5, %o4
	.word	0x89b81d60 !fshiftorx	%f0,%f0,%f14,%f4
	.word	0x8db81d62 !fshiftorx	%f0,%f2,%f14,%f6
	.word	0x91b89d62 !fshiftorx	%f2,%f2,%f14,%f8

	stda		%f4, [%o1 + %o5]0xc0	! partial store
	std		%f6, [%o1 + 8]
	stda		%f8, [%o0 + %o4]0xc0	! partial store
	retl
	nop
.type	aes_fx_encrypt,#function
.size	aes_fx_encrypt,.-aes_fx_encrypt

.globl	aes_fx_decrypt
.align	32
aes_fx_decrypt:
	and		%o0, 7, %o4		! is input aligned?
	andn		%o0, 7, %o0
	ldd		[%o2 +  0], %f6	! round[0]
	ldd		[%o2 +  8], %f8
	mov		%o7, %g1
	ld		[%o2 + 240], %o3

1:	call		.+8
	add		%o7, .Linp_align-1b, %o7

	sll		%o4, 3, %o4
	ldd		[%o0 + 0], %f0		! load input
	brz,pt		%o4, .Ldec_inp_aligned
	ldd		[%o0 + 8], %f2

	ldd		[%o7 + %o4], %f14	! shift left params
	ldd		[%o0 + 16], %f4
	.word	0x81b81d62 !fshiftorx	%f0,%f2,%f14,%f0
	.word	0x85b89d64 !fshiftorx	%f2,%f4,%f14,%f2

.Ldec_inp_aligned:
	ldd		[%o2 + 16], %f10	! round[1]
	ldd		[%o2 + 24], %f12

	.word	0x81b00d86 !fxor	%f0,%f6,%f0		! ^=round[0]
	.word	0x85b08d88 !fxor	%f2,%f8,%f2
	ldd		[%o2 + 32], %f6	! round[2]
	ldd		[%o2 + 40], %f8
	add		%o2, 32, %o2
	sub		%o3, 4, %o3

.Loop_dec:
	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%o2 + 16], %f10
	ldd		[%o2 + 24], %f12
	add		%o2, 32, %o2

	fmovd		%f0, %f4
	.word	0x81b09226 !faesdecx	%f2,%f6,%f0
	.word	0x85b11228 !faesdecx	%f4,%f8,%f2
	ldd		[%o2 +  0], %f6
	ldd		[%o2 +  8], %f8

	brnz,a		%o3, .Loop_dec
	sub		%o3, 2, %o3

	andcc		%o1, 7, %o4		! is output aligned?
	andn		%o1, 7, %o1
	mov		0xff, %o5
	srl		%o5, %o4, %o5
	add		%o7, 64, %o7
	sll		%o4, 3, %o4

	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%o7 + %o4], %f14	! shift right params

	fmovd		%f0, %f4
	.word	0x81b09266 !faesdeclx	%f2,%f6,%f0
	.word	0x85b11268 !faesdeclx	%f4,%f8,%f2

	bnz,pn		%icc, .Ldec_out_unaligned
	mov		%g1, %o7

	std		%f0, [%o1 + 0]
	retl
	std		%f2, [%o1 + 8]

.align	16
.Ldec_out_unaligned:
	add		%o1, 16, %o0
	orn		%g0, %o5, %o4
	.word	0x89b81d60 !fshiftorx	%f0,%f0,%f14,%f4
	.word	0x8db81d62 !fshiftorx	%f0,%f2,%f14,%f6
	.word	0x91b89d62 !fshiftorx	%f2,%f2,%f14,%f8

	stda		%f4, [%o1 + %o5]0xc0	! partial store
	std		%f6, [%o1 + 8]
	stda		%f8, [%o0 + %o4]0xc0	! partial store
	retl
	nop
.type	aes_fx_decrypt,#function
.size	aes_fx_decrypt,.-aes_fx_decrypt
.globl	aes_fx_set_decrypt_key
.align	32
aes_fx_set_decrypt_key:
	b		.Lset_encrypt_key
	mov		-1, %o4
	retl
	nop
.type	aes_fx_set_decrypt_key,#function
.size	aes_fx_set_decrypt_key,.-aes_fx_set_decrypt_key

.globl	aes_fx_set_encrypt_key
.align	32
aes_fx_set_encrypt_key:
	mov		1, %o4
	nop
.Lset_encrypt_key:
	and		%o0, 7, %o3
	andn		%o0, 7, %o0
	sll		%o3, 3, %o3
	mov		%o7, %g1

1:	call		.+8
	add		%o7, .Linp_align-1b, %o7

	ldd		[%o7 + %o3], %f10	! shift left params
	mov		%g1, %o7

	cmp		%o1, 192
	ldd		[%o0 + 0], %f0
	bl,pt		%icc, .L128
	ldd		[%o0 + 8], %f2

	be,pt		%icc, .L192
	ldd		[%o0 + 16], %f4
	brz,pt		%o3, .L256aligned
	ldd		[%o0 + 24], %f6

	ldd		[%o0 + 32], %f8
	.word	0x81b81562 !fshiftorx	%f0,%f2,%f10,%f0
	.word	0x85b89564 !fshiftorx	%f2,%f4,%f10,%f2
	.word	0x89b91566 !fshiftorx	%f4,%f6,%f10,%f4
	.word	0x8db99568 !fshiftorx	%f6,%f8,%f10,%f6

.L256aligned:
	mov		14, %o1
	and		%o4, 224, %o3
	st		%o1, [%o2 + 240]	! store rounds
	add		%o2, %o3, %o2	! start or end of key schedule
	sllx		%o4, 4, %o4		! 16 or -16
	std		%f0, [%o2 + 0]
	.word	0x81b19290 !faeskeyx	%f6,16,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09281 !faeskeyx	%f2,0x01,%f4
	std		%f6, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x8db11280 !faeskeyx	%f4,0x00,%f6
	std		%f0, [%o2 + 0]
	.word	0x81b19291 !faeskeyx	%f6,17,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09281 !faeskeyx	%f2,0x01,%f4
	std		%f6, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x8db11280 !faeskeyx	%f4,0x00,%f6
	std		%f0, [%o2 + 0]
	.word	0x81b19292 !faeskeyx	%f6,18,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09281 !faeskeyx	%f2,0x01,%f4
	std		%f6, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x8db11280 !faeskeyx	%f4,0x00,%f6
	std		%f0, [%o2 + 0]
	.word	0x81b19293 !faeskeyx	%f6,19,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09281 !faeskeyx	%f2,0x01,%f4
	std		%f6, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x8db11280 !faeskeyx	%f4,0x00,%f6
	std		%f0, [%o2 + 0]
	.word	0x81b19294 !faeskeyx	%f6,20,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09281 !faeskeyx	%f2,0x01,%f4
	std		%f6, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x8db11280 !faeskeyx	%f4,0x00,%f6
	std		%f0, [%o2 + 0]
	.word	0x81b19295 !faeskeyx	%f6,21,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09281 !faeskeyx	%f2,0x01,%f4
	std		%f6, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x8db11280 !faeskeyx	%f4,0x00,%f6
	std		%f0, [%o2 + 0]
	.word	0x81b19296 !faeskeyx	%f6,22,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4,[%o2 + 0]
	std		%f6,[%o2 + 8]
	add		%o2, %o4, %o2
	std		%f0,[%o2 + 0]
	std		%f2,[%o2 + 8]
	retl
	xor		%o0, %o0, %o0		! return 0

.align	16
.L192:
	brz,pt		%o3, .L192aligned
	nop

	ldd		[%o0 + 24], %f6
	.word	0x81b81562 !fshiftorx	%f0,%f2,%f10,%f0
	.word	0x85b89564 !fshiftorx	%f2,%f4,%f10,%f2
	.word	0x89b91566 !fshiftorx	%f4,%f6,%f10,%f4

.L192aligned:
	mov		12, %o1
	and		%o4, 192, %o3
	st		%o1, [%o2 + 240]	! store rounds
	add		%o2, %o3, %o2	! start or end of key schedule
	sllx		%o4, 4, %o4		! 16 or -16
	std		%f0, [%o2 + 0]
	.word	0x81b11290 !faeskeyx	%f4,16,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x81b11291 !faeskeyx	%f4,17,%f0
	std		%f2, [%o2 + 0]
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 0]
	.word	0x81b11292 !faeskeyx	%f4,18,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x81b11293 !faeskeyx	%f4,19,%f0
	std		%f2, [%o2 + 0]
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 0]
	.word	0x81b11294 !faeskeyx	%f4,20,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x81b11295 !faeskeyx	%f4,21,%f0
	std		%f2, [%o2 + 0]
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 0]
	.word	0x81b11296 !faeskeyx	%f4,22,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 0]
	.word	0x89b09280 !faeskeyx	%f2,0x00,%f4
	std		%f0, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x81b11297 !faeskeyx	%f4,23,%f0
	std		%f2, [%o2 + 0]
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f4, [%o2 + 8]
	add		%o2, %o4, %o2
	std		%f0, [%o2 + 0]
	std		%f2, [%o2 + 8]
	retl
	xor		%o0, %o0, %o0		! return 0

.align	16
.L128:
	brz,pt		%o3, .L128aligned
	nop

	ldd		[%o0 + 16], %f4
	.word	0x81b81562 !fshiftorx	%f0,%f2,%f10,%f0
	.word	0x85b89564 !fshiftorx	%f2,%f4,%f10,%f2

.L128aligned:
	mov		10, %o1
	and		%o4, 160, %o3
	st		%o1, [%o2 + 240]	! store rounds
	add		%o2, %o3, %o2	! start or end of key schedule
	sllx		%o4, 4, %o4		! 16 or -16
	std		%f0, [%o2 + 0]
	.word	0x81b09290 !faeskeyx	%f2,16,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09291 !faeskeyx	%f2,17,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09292 !faeskeyx	%f2,18,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09293 !faeskeyx	%f2,19,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09294 !faeskeyx	%f2,20,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09295 !faeskeyx	%f2,21,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09296 !faeskeyx	%f2,22,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09297 !faeskeyx	%f2,23,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09298 !faeskeyx	%f2,24,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	.word	0x81b09299 !faeskeyx	%f2,25,%f0
	std		%f2, [%o2 + 8]
	add		%o2, %o4, %o2
	.word	0x85b01280 !faeskeyx	%f0,0x00,%f2
	std		%f0, [%o2 + 0]
	std		%f2, [%o2 + 8]
	retl
	xor		%o0, %o0, %o0		! return 0
.type	aes_fx_set_encrypt_key,#function
.size	aes_fx_set_encrypt_key,.-aes_fx_set_encrypt_key
.globl	aes_fx_cbc_encrypt
.align	32
aes_fx_cbc_encrypt:
	save		%sp, -STACK_FRAME-16, %sp
	srln		%i2, 4, %i2
	and		%i0, 7, %l4
	andn		%i0, 7, %i0
	brz,pn		%i2, .Lcbc_no_data
	sll		%l4, 3, %l4

1:	call		.+8
	add		%o7, .Linp_align-1b, %o7

	ld		[%i3 + 240], %l0
	and		%i1, 7, %l5
	ld		[%i4 + 0], %f0		! load ivec
	andn		%i1, 7, %i1
	ld		[%i4 + 4], %f1
	sll		%l5, 3, %l6
	ld		[%i4 + 8], %f2
	ld		[%i4 + 12], %f3

	sll		%l0, 4, %l0
	add		%l0, %i3, %l2
	ldd		[%i3 + 0], %f20	! round[0]
	ldd		[%i3 + 8], %f22

	add		%i0, 16, %i0
	sub		%i2,  1, %i2
	ldd		[%l2 + 0], %f24	! round[last]
	ldd		[%l2 + 8], %f26

	mov		16, %l3
	movrz		%i2, 0, %l3
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	ldd		[%o7 + %l4], %f36	! shift left params
	add		%o7, 64, %o7
	ldd		[%i0 - 16], %f28	! load input
	ldd		[%i0 -  8], %f30
	ldda		[%i0]0x82, %f32	! non-faulting load
	brz		%i5, .Lcbc_decrypt
	add		%i0, %l3, %i0	! inp+=16

	.word	0x81b50d80 !fxor	%f20,%f0,%f0		! ivec^=round[0]
	.word	0x85b58d82 !fxor	%f22,%f2,%f2
	.word	0xb9bf0b7e !fshiftorx	%f28,%f30,%f36,%f28
	.word	0xbdbf8b61 !fshiftorx	%f30,%f32,%f36,%f30
	nop

.Loop_cbc_enc:
	.word	0x81b70d80 !fxor	%f28,%f0,%f0		! inp^ivec^round[0]
	.word	0x85b78d82 !fxor	%f30,%f2,%f2
	ldd		[%i3 + 32], %f6	! round[2]
	ldd		[%i3 + 40], %f8
	add		%i3, 32, %l2
	sub		%l0, 16*6, %l1

.Lcbc_enc:
	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10
	ldd		[%l2 + 24], %f12
	add		%l2, 32, %l2

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%l2 + 0], %f6
	ldd		[%l2 + 8], %f8

	brnz,a		%l1, .Lcbc_enc
	sub		%l1, 16*2, %l1

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10	! round[last-1]
	ldd		[%l2 + 24], %f12

	movrz		%i2, 0, %l3
	fmovd		%f32, %f28
	ldd		[%i0 - 8], %f30	! load next input block
	ldda		[%i0]0x82, %f32	! non-faulting load
	add		%i0, %l3, %i0	! inp+=16

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2

	.word	0xb9bf0b7e !fshiftorx	%f28,%f30,%f36,%f28
	.word	0xbdbf8b61 !fshiftorx	%f30,%f32,%f36,%f30

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	.word	0xb9b50d9c !fxor	%f20,%f28,%f28	! inp^=round[0]
	.word	0xbdb58d9e !fxor	%f22,%f30,%f30

	fmovd		%f0, %f4
	.word	0x81b09258 !faesenclx	%f2,%f24,%f0
	.word	0x85b1125a !faesenclx	%f4,%f26,%f2

	brnz,pn		%l5, .Lcbc_enc_unaligned_out
	nop

	std		%f0, [%i1 + 0]
	std		%f2, [%i1 + 8]
	add		%i1, 16, %i1

	brnz,a		%i2, .Loop_cbc_enc
	sub		%i2, 1, %i2

	st		%f0, [%i4 + 0]		! output ivec
	st		%f1, [%i4 + 4]
	st		%f2, [%i4 + 8]
	st		%f3, [%i4 + 12]

.Lcbc_no_data:
	ret
	restore

.align	32
.Lcbc_enc_unaligned_out:
	ldd		[%o7 + %l6], %f36	! shift right params
	mov		0xff, %l6
	srl		%l6, %l5, %l6
	sub		%g0, %l4, %l5

	.word	0x8db80b60 !fshiftorx	%f0,%f0,%f36,%f6
	.word	0x91b80b62 !fshiftorx	%f0,%f2,%f36,%f8

	stda		%f6, [%i1 + %l6]0xc0	! partial store
	orn		%g0, %l6, %l6
	std		%f8, [%i1 + 8]
	add		%i1, 16, %i1
	brz		%i2, .Lcbc_enc_unaligned_out_done
	sub		%i2, 1, %i2
	b		.Loop_cbc_enc_unaligned_out
	nop

.align	32
.Loop_cbc_enc_unaligned_out:
	fmovd		%f2, %f34
	.word	0x81b70d80 !fxor	%f28,%f0,%f0		! inp^ivec^round[0]
	.word	0x85b78d82 !fxor	%f30,%f2,%f2
	ldd		[%i3 + 32], %f6	! round[2]
	ldd		[%i3 + 40], %f8

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%i3 + 48], %f10	! round[3]
	ldd		[%i3 + 56], %f12

	ldx		[%i0 - 16], %o0
	ldx		[%i0 -  8], %o1
	brz		%l4, .Lcbc_enc_aligned_inp
	movrz		%i2, 0, %l3

	ldx		[%i0], %o2
	sllx		%o0, %l4, %o0
	srlx		%o1, %l5, %g1
	sllx		%o1, %l4, %o1
	or		%g1, %o0, %o0
	srlx		%o2, %l5, %o2
	or		%o2, %o1, %o1

.Lcbc_enc_aligned_inp:
	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%i3 + 64], %f6	! round[4]
	ldd		[%i3 + 72], %f8
	add		%i3, 64, %l2
	sub		%l0, 16*8, %l1

	stx		%o0, [%sp + LOCALS + 0]
	stx		%o1, [%sp + LOCALS + 8]
	add		%i0, %l3, %i0	! inp+=16
	nop

.Lcbc_enc_unaligned:
	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10
	ldd		[%l2 + 24], %f12
	add		%l2, 32, %l2

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%l2 + 0], %f6
	ldd		[%l2 + 8], %f8

	brnz,a		%l1, .Lcbc_enc_unaligned
	sub		%l1, 16*2, %l1

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10	! round[last-1]
	ldd		[%l2 + 24], %f12

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2

	ldd		[%sp + LOCALS + 0], %f28
	ldd		[%sp + LOCALS + 8], %f30

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	.word	0xb9b50d9c !fxor	%f20,%f28,%f28	! inp^=round[0]
	.word	0xbdb58d9e !fxor	%f22,%f30,%f30

	fmovd		%f0, %f4
	.word	0x81b09258 !faesenclx	%f2,%f24,%f0
	.word	0x85b1125a !faesenclx	%f4,%f26,%f2

	.word	0x8db8cb60 !fshiftorx	%f34,%f0,%f36,%f6
	.word	0x91b80b62 !fshiftorx	%f0,%f2,%f36,%f8
	std		%f6, [%i1 + 0]
	std		%f8, [%i1 + 8]
	add		%i1, 16, %i1

	brnz,a		%i2, .Loop_cbc_enc_unaligned_out
	sub		%i2, 1, %i2

.Lcbc_enc_unaligned_out_done:
	.word	0x91b88b62 !fshiftorx	%f2,%f2,%f36,%f8
	stda		%f8, [%i1 + %l6]0xc0	! partial store

	st		%f0, [%i4 + 0]		! output ivec
	st		%f1, [%i4 + 4]
	st		%f2, [%i4 + 8]
	st		%f3, [%i4 + 12]

	ret
	restore

.align	32
.Lcbc_decrypt:
	.word	0xb9bf0b7e !fshiftorx	%f28,%f30,%f36,%f28
	.word	0xbdbf8b61 !fshiftorx	%f30,%f32,%f36,%f30
	fmovd		%f0, %f16
	fmovd		%f2, %f18

.Loop_cbc_dec:
	.word	0x81b70d94 !fxor	%f28,%f20,%f0	! inp^round[0]
	.word	0x85b78d96 !fxor	%f30,%f22,%f2
	ldd		[%i3 + 32], %f6	! round[2]
	ldd		[%i3 + 40], %f8
	add		%i3, 32, %l2
	sub		%l0, 16*6, %l1

.Lcbc_dec:
	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10
	ldd		[%l2 + 24], %f12
	add		%l2, 32, %l2

	fmovd		%f0, %f4
	.word	0x81b09226 !faesdecx	%f2,%f6,%f0
	.word	0x85b11228 !faesdecx	%f4,%f8,%f2
	ldd		[%l2 + 0], %f6
	ldd		[%l2 + 8], %f8

	brnz,a		%l1, .Lcbc_dec
	sub		%l1, 16*2, %l1

	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10	! round[last-1]
	ldd		[%l2 + 24], %f12

	fmovd		%f0, %f4
	.word	0x81b09226 !faesdecx	%f2,%f6,%f0
	.word	0x85b11228 !faesdecx	%f4,%f8,%f2
	.word	0x8db40d98 !fxor	%f16,%f24,%f6	! ivec^round[last]
	.word	0x91b48d9a !fxor	%f18,%f26,%f8
	fmovd		%f28, %f16
	fmovd		%f30, %f18

	movrz		%i2, 0, %l3
	fmovd		%f32, %f28
	ldd		[%i0 - 8], %f30	! load next input block
	ldda		[%i0]0x82, %f32	! non-faulting load
	add		%i0, %l3, %i0	! inp+=16

	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	.word	0xb9bf0b7e !fshiftorx	%f28,%f30,%f36,%f28
	.word	0xbdbf8b61 !fshiftorx	%f30,%f32,%f36,%f30

	fmovd		%f0, %f4
	.word	0x81b09266 !faesdeclx	%f2,%f6,%f0
	.word	0x85b11268 !faesdeclx	%f4,%f8,%f2

	brnz,pn		%l5, .Lcbc_dec_unaligned_out
	nop

	std		%f0, [%i1 + 0]
	std		%f2, [%i1 + 8]
	add		%i1, 16, %i1

	brnz,a		%i2, .Loop_cbc_dec
	sub		%i2, 1, %i2

	st		%f16,    [%i4 + 0]	! output ivec
	st		%f17, [%i4 + 4]
	st		%f18,    [%i4 + 8]
	st		%f19, [%i4 + 12]

	ret
	restore

.align	32
.Lcbc_dec_unaligned_out:
	ldd		[%o7 + %l6], %f36	! shift right params
	mov		0xff, %l6
	srl		%l6, %l5, %l6
	sub		%g0, %l4, %l5

	.word	0x8db80b60 !fshiftorx	%f0,%f0,%f36,%f6
	.word	0x91b80b62 !fshiftorx	%f0,%f2,%f36,%f8

	stda		%f6, [%i1 + %l6]0xc0	! partial store
	orn		%g0, %l6, %l6
	std		%f8, [%i1 + 8]
	add		%i1, 16, %i1
	brz		%i2, .Lcbc_dec_unaligned_out_done
	sub		%i2, 1, %i2
	b		.Loop_cbc_dec_unaligned_out
	nop

.align	32
.Loop_cbc_dec_unaligned_out:
	fmovd		%f2, %f34
	.word	0x81b70d94 !fxor	%f28,%f20,%f0	! inp^round[0]
	.word	0x85b78d96 !fxor	%f30,%f22,%f2
	ldd		[%i3 + 32], %f6	! round[2]
	ldd		[%i3 + 40], %f8

	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%i3 + 48], %f10	! round[3]
	ldd		[%i3 + 56], %f12

	ldx		[%i0 - 16], %o0
	ldx		[%i0 - 8], %o1
	brz		%l4, .Lcbc_dec_aligned_inp
	movrz		%i2, 0, %l3

	ldx		[%i0], %o2
	sllx		%o0, %l4, %o0
	srlx		%o1, %l5, %g1
	sllx		%o1, %l4, %o1
	or		%g1, %o0, %o0
	srlx		%o2, %l5, %o2
	or		%o2, %o1, %o1

.Lcbc_dec_aligned_inp:
	fmovd		%f0, %f4
	.word	0x81b09226 !faesdecx	%f2,%f6,%f0
	.word	0x85b11228 !faesdecx	%f4,%f8,%f2
	ldd		[%i3 + 64], %f6	! round[4]
	ldd		[%i3 + 72], %f8
	add		%i3, 64, %l2
	sub		%l0, 16*8, %l1

	stx		%o0, [%sp + LOCALS + 0]
	stx		%o1, [%sp + LOCALS + 8]
	add		%i0, %l3, %i0	! inp+=16
	nop

.Lcbc_dec_unaligned:
	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10
	ldd		[%l2 + 24], %f12
	add		%l2, 32, %l2

	fmovd		%f0, %f4
	.word	0x81b09226 !faesdecx	%f2,%f6,%f0
	.word	0x85b11228 !faesdecx	%f4,%f8,%f2
	ldd		[%l2 + 0], %f6
	ldd		[%l2 + 8], %f8

	brnz,a		%l1, .Lcbc_dec_unaligned
	sub		%l1, 16*2, %l1

	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10	! round[last-1]
	ldd		[%l2 + 24], %f12

	fmovd		%f0, %f4
	.word	0x81b09226 !faesdecx	%f2,%f6,%f0
	.word	0x85b11228 !faesdecx	%f4,%f8,%f2

	.word	0x8db40d98 !fxor	%f16,%f24,%f6	! ivec^round[last]
	.word	0x91b48d9a !fxor	%f18,%f26,%f8
	fmovd		%f28, %f16
	fmovd		%f30, %f18
	ldd		[%sp + LOCALS + 0], %f28
	ldd		[%sp + LOCALS + 8], %f30

	fmovd		%f0, %f4
	.word	0x81b0922a !faesdecx	%f2,%f10,%f0
	.word	0x85b1122c !faesdecx	%f4,%f12,%f2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	fmovd		%f0, %f4
	.word	0x81b09266 !faesdeclx	%f2,%f6,%f0
	.word	0x85b11268 !faesdeclx	%f4,%f8,%f2

	.word	0x8db8cb60 !fshiftorx	%f34,%f0,%f36,%f6
	.word	0x91b80b62 !fshiftorx	%f0,%f2,%f36,%f8
	std		%f6, [%i1 + 0]
	std		%f8, [%i1 + 8]
	add		%i1, 16, %i1

	brnz,a		%i2, .Loop_cbc_dec_unaligned_out
	sub		%i2, 1, %i2

.Lcbc_dec_unaligned_out_done:
	.word	0x91b88b62 !fshiftorx	%f2,%f2,%f36,%f8
	stda		%f8, [%i1 + %l6]0xc0	! partial store

	st		%f16,    [%i4 + 0]	! output ivec
	st		%f17, [%i4 + 4]
	st		%f18,    [%i4 + 8]
	st		%f19, [%i4 + 12]

	ret
	restore
.type	aes_fx_cbc_encrypt,#function
.size	aes_fx_cbc_encrypt,.-aes_fx_cbc_encrypt
.globl	aes_fx_ctr32_encrypt_blocks
.align	32
aes_fx_ctr32_encrypt_blocks:
	save		%sp, -STACK_FRAME-16, %sp
	srln		%i2, 0, %i2
	and		%i0, 7, %l4
	andn		%i0, 7, %i0
	brz,pn		%i2, .Lctr32_no_data
	sll		%l4, 3, %l4

.Lpic:	call		.+8
	add		%o7, .Linp_align - .Lpic, %o7

	ld		[%i3 + 240], %l0
	and		%i1, 7, %l5
	ld		[%i4 +  0], %f16	! load counter
	andn		%i1, 7, %i1
	ld		[%i4 +  4], %f17
	sll		%l5, 3, %l6
	ld		[%i4 +  8], %f18
	ld		[%i4 + 12], %f19
	ldd		[%o7 + 128], %f14

	sll		%l0, 4, %l0
	add		%l0, %i3, %l2
	ldd		[%i3 + 0], %f20	! round[0]
	ldd		[%i3 + 8], %f22

	add		%i0, 16, %i0
	sub		%i2, 1, %i2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	mov		16, %l3
	movrz		%i2, 0, %l3
	ldd		[%l2 + 0], %f24	! round[last]
	ldd		[%l2 + 8], %f26

	ldd		[%o7 + %l4], %f36	! shiftleft params
	add		%o7, 64, %o7
	ldd		[%i0 - 16], %f28	! load input
	ldd		[%i0 -  8], %f30
	ldda		[%i0]0x82, %f32	! non-faulting load
	add		%i0, %l3, %i0	! inp+=16

	.word	0xb9bf0b7e !fshiftorx	%f28,%f30,%f36,%f28
	.word	0xbdbf8b61 !fshiftorx	%f30,%f32,%f36,%f30

.Loop_ctr32:
	.word	0x81b40d94 !fxor	%f16,%f20,%f0	! counter^round[0]
	.word	0x85b48d96 !fxor	%f18,%f22,%f2
	ldd		[%i3 + 32], %f6	! round[2]
	ldd		[%i3 + 40], %f8
	add		%i3, 32, %l2
	sub		%l0, 16*6, %l1

.Lctr32_enc:
	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10
	ldd		[%l2 + 24], %f12
	add		%l2, 32, %l2

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%l2 + 0], %f6
	ldd		[%l2 + 8], %f8

	brnz,a		%l1, .Lctr32_enc
	sub		%l1, 16*2, %l1

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10	! round[last-1]
	ldd		[%l2 + 24], %f12

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	.word	0x8db70d98 !fxor	%f28,%f24,%f6	! inp^round[last]
	.word	0x91b78d9a !fxor	%f30,%f26,%f8

	movrz		%i2, 0, %l3
	fmovd		%f32, %f28
	ldd		[%i0 - 8], %f30	! load next input block
	ldda		[%i0]0x82, %f32	! non-faulting load
	add		%i0, %l3, %i0	! inp+=16

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	.word	0xb9bf0b7e !fshiftorx	%f28,%f30,%f36,%f28
	.word	0xbdbf8b61 !fshiftorx	%f30,%f32,%f36,%f30
	.word	0xa5b48a4e !fpadd32	%f18,%f14,%f18	! increment counter

	fmovd		%f0, %f4
	.word	0x81b09246 !faesenclx	%f2,%f6,%f0
	.word	0x85b11248 !faesenclx	%f4,%f8,%f2

	brnz,pn		%l5, .Lctr32_unaligned_out
	nop

	std		%f0, [%i1 + 0]
	std		%f2, [%i1 + 8]
	add		%i1, 16, %i1

	brnz,a		%i2, .Loop_ctr32
	sub		%i2, 1, %i2

.Lctr32_no_data:
	ret
	restore

.align	32
.Lctr32_unaligned_out:
	ldd		[%o7 + %l6], %f36	! shift right params
	mov		0xff, %l6
	srl		%l6, %l5, %l6
	sub		%g0, %l4, %l5

	.word	0x8db80b60 !fshiftorx	%f0,%f0,%f36,%f6
	.word	0x91b80b62 !fshiftorx	%f0,%f2,%f36,%f8

	stda		%f6, [%i1 + %l6]0xc0	! partial store
	orn		%g0, %l6, %l6
	std		%f8, [%i1 + 8]
	add		%i1, 16, %i1
	brz		%i2, .Lctr32_unaligned_out_done
	sub		%i2, 1, %i2
	b		.Loop_ctr32_unaligned_out
	nop

.align	32
.Loop_ctr32_unaligned_out:
	fmovd		%f2, %f34
	.word	0x81b40d94 !fxor	%f16,%f20,%f0	! counter^round[0]
	.word	0x85b48d96 !fxor	%f18,%f22,%f2
	ldd		[%i3 + 32], %f6	! round[2]
	ldd		[%i3 + 40], %f8

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%i3 + 48], %f10	! round[3]
	ldd		[%i3 + 56], %f12

	ldx		[%i0 - 16], %o0
	ldx		[%i0 -  8], %o1
	brz		%l4, .Lctr32_aligned_inp
	movrz		%i2, 0, %l3

	ldx		[%i0], %o2
	sllx		%o0, %l4, %o0
	srlx		%o1, %l5, %g1
	sllx		%o1, %l4, %o1
	or		%g1, %o0, %o0
	srlx		%o2, %l5, %o2
	or		%o2, %o1, %o1

.Lctr32_aligned_inp:
	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%i3 + 64], %f6	! round[4]
	ldd		[%i3 + 72], %f8
	add		%i3, 64, %l2
	sub		%l0, 16*8, %l1

	stx		%o0, [%sp + LOCALS + 0]
	stx		%o1, [%sp + LOCALS + 8]
	add		%i0, %l3, %i0	! inp+=16
	nop

.Lctr32_enc_unaligned:
	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10
	ldd		[%l2 + 24], %f12
	add		%l2, 32, %l2

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	ldd		[%l2 + 0], %f6
	ldd		[%l2 + 8], %f8

	brnz,a		%l1, .Lctr32_enc_unaligned
	sub		%l1, 16*2, %l1

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%l2 + 16], %f10	! round[last-1]
	ldd		[%l2 + 24], %f12
	.word	0xa5b48a4e !fpadd32	%f18,%f14,%f18	! increment counter

	fmovd		%f0, %f4
	.word	0x81b09206 !faesencx	%f2,%f6,%f0
	.word	0x85b11208 !faesencx	%f4,%f8,%f2
	.word	0x8db70d98 !fxor	%f28,%f24,%f6	! inp^round[last]
	.word	0x91b78d9a !fxor	%f30,%f26,%f8
	ldd		[%sp + LOCALS + 0], %f28
	ldd		[%sp + LOCALS + 8], %f30

	fmovd		%f0, %f4
	.word	0x81b0920a !faesencx	%f2,%f10,%f0
	.word	0x85b1120c !faesencx	%f4,%f12,%f2
	ldd		[%i3 + 16], %f10	! round[1]
	ldd		[%i3 + 24], %f12

	fmovd		%f0, %f4
	.word	0x81b09246 !faesenclx	%f2,%f6,%f0
	.word	0x85b11248 !faesenclx	%f4,%f8,%f2

	.word	0x8db8cb60 !fshiftorx	%f34,%f0,%f36,%f6
	.word	0x91b80b62 !fshiftorx	%f0,%f2,%f36,%f8
	std		%f6, [%i1 + 0]
	std		%f8, [%i1 + 8]
	add		%i1, 16, %i1

	brnz,a		%i2, .Loop_ctr32_unaligned_out
	sub		%i2, 1, %i2

.Lctr32_unaligned_out_done:
	.word	0x91b88b62 !fshiftorx	%f2,%f2,%f36,%f8
	stda		%f8, [%i1 + %l6]0xc0	! partial store

	ret
	restore
.type	aes_fx_ctr32_encrypt_blocks,#function
.size	aes_fx_ctr32_encrypt_blocks,.-aes_fx_ctr32_encrypt_blocks

.align	32
.Linp_align:		! fshiftorx parameters for left shift toward %rs1
	.byte	0, 0, 64,  0,	0, 64,  0, -64
	.byte	0, 0, 56,  8,	0, 56,  8, -56
	.byte	0, 0, 48, 16,	0, 48, 16, -48
	.byte	0, 0, 40, 24,	0, 40, 24, -40
	.byte	0, 0, 32, 32,	0, 32, 32, -32
	.byte	0, 0, 24, 40,	0, 24, 40, -24
	.byte	0, 0, 16, 48,	0, 16, 48, -16
	.byte	0, 0,  8, 56,	0,  8, 56, -8
.Lout_align:		! fshiftorx parameters for right shift toward %rs2
	.byte	0, 0,  0, 64,	0,  0, 64,   0
	.byte	0, 0,  8, 56,	0,  8, 56,  -8
	.byte	0, 0, 16, 48,	0, 16, 48, -16
	.byte	0, 0, 24, 40,	0, 24, 40, -24
	.byte	0, 0, 32, 32,	0, 32, 32, -32
	.byte	0, 0, 40, 24,	0, 40, 24, -40
	.byte	0, 0, 48, 16,	0, 48, 16, -48
	.byte	0, 0, 56,  8,	0, 56,  8, -56
.Lone:
	.word	0, 1
.asciz	"AES for Fujitsu SPARC64 X, CRYPTOGAMS by <appro@openssl.org>"
.align	4
