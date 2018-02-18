#include "sparc_arch.h"

#ifdef	__arch64__
.register	%g2,#scratch
.register	%g3,#scratch
# define	STPTR	stx
# define	SIZE_T	8
#else
# define	STPTR	st
# define	SIZE_T	4
#endif
#define	LOCALS	(STACK_BIAS+STACK_FRAME)

.section	".text",#alloc,#execinstr

#ifdef __PIC__
SPARC_PIC_THUNK(%g1)
#endif

.globl	poly1305_init
.align	32
poly1305_init:
	save	%sp,-STACK_FRAME-16,%sp
	nop

	SPARC_LOAD_ADDRESS(OPENSSL_sparcv9cap_P,%g1)
	ld	[%g1],%g1

	and	%g1,SPARCV9_FMADD|SPARCV9_VIS3,%g1
	cmp	%g1,SPARCV9_FMADD
	be	.Lpoly1305_init_fma
	nop

	stx	%g0,[%i0+0]
	stx	%g0,[%i0+8]		! zero hash value
	brz,pn	%i1,.Lno_key
	stx	%g0,[%i0+16]

	and	%i1,7,%i5		! alignment factor
	andn	%i1,7,%i1
	sll	%i5,3,%i5		! *8
	neg	%i5,%i4

	sethi	%hi(0x0ffffffc),%o4
	set	8,%o1
	or	%o4,%lo(0x0ffffffc),%o4
	set	16,%o2
	sllx	%o4,32,%o5
	or	%o4,%o5,%o5		! 0x0ffffffc0ffffffc
	or	%o5,3,%o4		! 0x0ffffffc0fffffff

	ldxa	[%i1+%g0]0x88,%o0	! load little-endian key
	brz,pt	%i5,.Lkey_aligned
	ldxa	[%i1+%o1]0x88,%o1

	ldxa	[%i1+%o2]0x88,%o2
	srlx	%o0,%i5,%o0
	sllx	%o1,%i4,%o7
	srlx	%o1,%i5,%o1
	or	%o7,%o0,%o0
	sllx	%o2,%i4,%o2
	or	%o2,%o1,%o1

.Lkey_aligned:
	and	%o4,%o0,%o0
	and	%o5,%o1,%o1
	stx	%o0,[%i0+32+0]		! store key
	stx	%o1,[%i0+32+8]

	andcc	%g1,SPARCV9_VIS3,%g0
	be	.Lno_key
	nop

1:	call	.+8
	add	%o7,poly1305_blocks_vis3-1b,%o7

	add	%o7,poly1305_emit-poly1305_blocks_vis3,%o5
	STPTR	%o7,[%i2]
	STPTR	%o5,[%i2+SIZE_T]

	ret
	restore	%g0,1,%o0		! return 1

.Lno_key:
	ret
	restore	%g0,%g0,%o0		! return 0
.type	poly1305_init,#function
.size	poly1305_init,.-poly1305_init

.globl	poly1305_blocks
.align	32
poly1305_blocks:
	save	%sp,-STACK_FRAME,%sp
	srln	%i2,4,%i2

	brz,pn	%i2,.Lno_data
	nop

	ld	[%i0+32+0],%l1		! load key
	ld	[%i0+32+4],%l0
	ld	[%i0+32+8],%l3
	ld	[%i0+32+12],%l2

	ld	[%i0+0],%o1		! load hash value
	ld	[%i0+4],%o0
	ld	[%i0+8],%o3
	ld	[%i0+12],%o2
	ld	[%i0+16],%l7

	and	%i1,7,%i5		! alignment factor
	andn	%i1,7,%i1
	set	8,%g2
	sll	%i5,3,%i5		! *8
	set	16,%g3
	neg	%i5,%i4

	srl	%l1,2,%l4
	srl	%l2,2,%l5
	add	%l1,%l4,%l4
	srl	%l3,2,%l6
	add	%l2,%l5,%l5
	add	%l3,%l6,%l6

.Loop:
	ldxa	[%i1+%g0]0x88,%g1	! load little-endian input
	brz,pt	%i5,.Linp_aligned
	ldxa	[%i1+%g2]0x88,%g2

	ldxa	[%i1+%g3]0x88,%g3
	srlx	%g1,%i5,%g1
	sllx	%g2,%i4,%o5
	srlx	%g2,%i5,%g2
	or	%o5,%g1,%g1
	sllx	%g3,%i4,%g3
	or	%g3,%g2,%g2

.Linp_aligned:
	srlx	%g1,32,%o4
	addcc	%g1,%o0,%o0		! accumulate input
	srlx	%g2,32,%o5
	addccc	%o4,%o1,%o1
	addccc	%g2,%o2,%o2
	addccc	%o5,%o3,%o3
	addc	%i3,%l7,%l7

	umul	%l0,%o0,%g1
	umul	%l1,%o0,%g2
	umul	%l2,%o0,%g3
	umul	%l3,%o0,%g4
	 sub	%i2,1,%i2
	 add	%i1,16,%i1

	umul	%l6,%o1,%o4
	umul	%l0,%o1,%o5
	umul	%l1,%o1,%o7
	add	%o4,%g1,%g1
	add	%o5,%g2,%g2
	umul	%l2,%o1,%o4
	add	%o7,%g3,%g3
	add	%o4,%g4,%g4

	umul	%l5,%o2,%o5
	umul	%l6,%o2,%o7
	umul	%l0,%o2,%o4
	add	%o5,%g1,%g1
	add	%o7,%g2,%g2
	umul	%l1,%o2,%o5
	add	%o4,%g3,%g3
	add	%o5,%g4,%g4

	umul	%l4,%o3,%o7
	umul	%l5,%o3,%o4
	umul	%l6,%o3,%o5
	add	%o7,%g1,%g1
	add	%o4,%g2,%g2
	umul	%l0,%o3,%o7
	add	%o5,%g3,%g3
	add	%o7,%g4,%g4

	umul	%l4,%l7,%o4
	umul	%l5,%l7,%o5
	umul	%l6,%l7,%o7
	umul	%l0,%l7,%l7
	add	%o4,%g2,%g2
	add	%o5,%g3,%g3
	srlx	%g1,32,%o1
	add	%o7,%g4,%g4
	srlx	%g2,32,%o2

	addcc	%g2,%o1,%o1
	srlx	%g3,32,%o3
	 set	8,%g2
	addccc	%g3,%o2,%o2
	srlx	%g4,32,%o4
	 set	16,%g3
	addccc	%g4,%o3,%o3
	addc	%o4,%l7,%l7

	srl	%l7,2,%o4		! final reduction step
	andn	%l7,3,%o5
	and	%l7,3,%l7
	add	%o5,%o4,%o4

	addcc	%o4,%g1,%o0
	addccc	%g0,%o1,%o1
	addccc	%g0,%o2,%o2
	addccc	%g0,%o3,%o3
	brnz,pt	%i2,.Loop
	addc	%g0,%l7,%l7

	st	%o1,[%i0+0]		! store hash value
	st	%o0,[%i0+4]
	st	%o3,[%i0+8]
	st	%o2,[%i0+12]
	st	%l7,[%i0+16]

.Lno_data:
	ret
	restore
.type	poly1305_blocks,#function
.size	poly1305_blocks,.-poly1305_blocks
.align	32
poly1305_blocks_vis3:
	save	%sp,-STACK_FRAME,%sp
	srln	%i2,4,%i2

	brz,pn	%i2,.Lno_data
	nop

	ldx	[%i0+32+0],%o3		! load key
	ldx	[%i0+32+8],%o4

	ldx	[%i0+0],%o0		! load hash value
	ldx	[%i0+8],%o1
	ld	[%i0+16],%o2

	and	%i1,7,%i5		! alignment factor
	andn	%i1,7,%i1
	set	8,%l1
	sll	%i5,3,%i5		! *8
	set	16,%l2
	neg	%i5,%i4

	srlx	%o4,2,%o5
	b	.Loop_vis3
	add	%o4,%o5,%o5

.Loop_vis3:
	ldxa	[%i1+%g0]0x88,%g1	! load little-endian input
	brz,pt	%i5,.Linp_aligned_vis3
	ldxa	[%i1+%l1]0x88,%g2

	ldxa	[%i1+%l2]0x88,%g3
	srlx	%g1,%i5,%g1
	sllx	%g2,%i4,%o7
	srlx	%g2,%i5,%g2
	or	%o7,%g1,%g1
	sllx	%g3,%i4,%g3
	or	%g3,%g2,%g2

.Linp_aligned_vis3:
	addcc	%g1,%o0,%o0		! accumulate input
	 sub	%i2,1,%i2
	.word	0x93b08269 !addxccc	%g2,%o1,%o1
	 add	%i1,16,%i1

	mulx	%o3,%o0,%g1		! r0*h0
	.word	0x95b6c22a !addxc	%i3,%o2,%o2
	.word	0x85b2c2c8 !umulxhi	%o3,%o0,%g2
	mulx	%o5,%o1,%g4		! s1*h1
	.word	0x9fb342c9 !umulxhi	%o5,%o1,%o7
	addcc	%g4,%g1,%g1
	mulx	%o4,%o0,%g4		! r1*h0
	.word	0x85b3c222 !addxc	%o7,%g2,%g2
	.word	0x87b302c8 !umulxhi	%o4,%o0,%g3
	addcc	%g4,%g2,%g2
	mulx	%o3,%o1,%g4		! r0*h1
	.word	0x87b00223 !addxc	%g0,%g3,%g3
	.word	0x9fb2c2c9 !umulxhi	%o3,%o1,%o7
	addcc	%g4,%g2,%g2
	mulx	%o5,%o2,%g4		! s1*h2
	.word	0x87b3c223 !addxc	%o7,%g3,%g3
	mulx	%o3,%o2,%o7		! r0*h2
	addcc	%g4,%g2,%g2
	.word	0x87b3c223 !addxc	%o7,%g3,%g3

	srlx	%g3,2,%g4		! final reduction step
	andn	%g3,3,%o7
	and	%g3,3,%o2
	add	%o7,%g4,%g4

	addcc	%g4,%g1,%o0
	.word	0x93b00262 !addxccc	%g0,%g2,%o1
	brnz,pt	%i2,.Loop_vis3
	.word	0x95b0022a !addxc	%g0,%o2,%o2

	stx	%o0,[%i0+0]		! store hash value
	stx	%o1,[%i0+8]
	st	%o2,[%i0+16]

	ret
	restore
.type	poly1305_blocks_vis3,#function
.size	poly1305_blocks_vis3,.-poly1305_blocks_vis3
.globl	poly1305_emit
.align	32
poly1305_emit:
	save	%sp,-STACK_FRAME,%sp

	ld	[%i0+0],%o1		! load hash value
	ld	[%i0+4],%o0
	ld	[%i0+8],%o3
	ld	[%i0+12],%o2
	ld	[%i0+16],%l7

	addcc	%o0,5,%l0		! compare to modulus
	addccc	%o1,0,%l1
	addccc	%o2,0,%l2
	addccc	%o3,0,%l3
	addc	%l7,0,%l7
	andcc	%l7,4,%g0		! did it carry/borrow?

	movnz	%icc,%l0,%o0
	ld	[%i2+0],%l0		! load nonce
	movnz	%icc,%l1,%o1
	ld	[%i2+4],%l1
	movnz	%icc,%l2,%o2
	ld	[%i2+8],%l2
	movnz	%icc,%l3,%o3
	ld	[%i2+12],%l3

	addcc	%l0,%o0,%o0		! accumulate nonce
	addccc	%l1,%o1,%o1
	addccc	%l2,%o2,%o2
	addc	%l3,%o3,%o3

	srl	%o0,8,%l0
	stb	%o0,[%i1+0]		! store little-endian result
	srl	%o0,16,%l1
	stb	%l0,[%i1+1]
	srl	%o0,24,%l2
	stb	%l1,[%i1+2]
	stb	%l2,[%i1+3]

	srl	%o1,8,%l0
	stb	%o1,[%i1+4]
	srl	%o1,16,%l1
	stb	%l0,[%i1+5]
	srl	%o1,24,%l2
	stb	%l1,[%i1+6]
	stb	%l2,[%i1+7]

	srl	%o2,8,%l0
	stb	%o2,[%i1+8]
	srl	%o2,16,%l1
	stb	%l0,[%i1+9]
	srl	%o2,24,%l2
	stb	%l1,[%i1+10]
	stb	%l2,[%i1+11]

	srl	%o3,8,%l0
	stb	%o3,[%i1+12]
	srl	%o3,16,%l1
	stb	%l0,[%i1+13]
	srl	%o3,24,%l2
	stb	%l1,[%i1+14]
	stb	%l2,[%i1+15]

	ret
	restore
.type	poly1305_emit,#function
.size	poly1305_emit,.-poly1305_emit
.align	32
poly1305_init_fma:
	save	%sp,-STACK_FRAME-16,%sp
	nop

.Lpoly1305_init_fma:
1:	call	.+8
	add	%o7,.Lconsts_fma-1b,%o7

	ldd	[%o7+8*0],%f16			! load constants
	ldd	[%o7+8*1],%f18
	ldd	[%o7+8*2],%f20
	ldd	[%o7+8*3],%f22
	ldd	[%o7+8*5],%f26

	std	%f16,[%i0+8*0]		! initial hash value, biased 0
	std	%f18,[%i0+8*1]
	std	%f20,[%i0+8*2]
	std	%f22,[%i0+8*3]

	brz,pn	%i1,.Lno_key_fma
	nop

	stx	%fsr,[%sp+LOCALS]		! save original %fsr
	ldx	[%o7+8*6],%fsr			! load new %fsr

	std	%f16,[%i0+8*4] 		! key "template"
	std	%f18,[%i0+8*5]
	std	%f20,[%i0+8*6]
	std	%f22,[%i0+8*7]

	and	%i1,7,%l2
	andn	%i1,7,%i1			! align pointer
	mov	8,%l0
	sll	%l2,3,%l2
	mov	16,%l1
	neg	%l2,%l3

	ldxa	[%i1+%g0]0x88,%o0		! load little-endian key
	ldxa	[%i1+%l0]0x88,%o2

	brz	%l2,.Lkey_aligned_fma
	sethi	%hi(0xf0000000),%l0		!   0xf0000000

	ldxa	[%i1+%l1]0x88,%o4

	srlx	%o0,%l2,%o0			! align data
	sllx	%o2,%l3,%o1
	srlx	%o2,%l2,%o2
	or	%o1,%o0,%o0
	sllx	%o4,%l3,%o3
	or	%o3,%o2,%o2

.Lkey_aligned_fma:
	or	%l0,3,%l1			!   0xf0000003
	srlx	%o0,32,%o1
	andn	%o0,%l0,%o0			! &=0x0fffffff
	andn	%o1,%l1,%o1			! &=0x0ffffffc
	srlx	%o2,32,%o3
	andn	%o2,%l1,%o2
	andn	%o3,%l1,%o3

	st	%o0,[%i0+36]		! fill "template"
	st	%o1,[%i0+44]
	st	%o2,[%i0+52]
	st	%o3,[%i0+60]

	ldd	[%i0+8*4],%f0 		! load [biased] key
	ldd	[%i0+8*5],%f4
	ldd	[%i0+8*6],%f8
	ldd	[%i0+8*7],%f12

	fsubd	%f0,%f16, %f0		! r0
	 ldd	[%o7+8*7],%f16 		! more constants
	fsubd	%f4,%f18,%f4		! r1
	 ldd	[%o7+8*8],%f18
	fsubd	%f8,%f20,%f8		! r2
	 ldd	[%o7+8*9],%f20
	fsubd	%f12,%f22,%f12		! r3
	 ldd	[%o7+8*10],%f22

	fmuld	%f26,%f4,%f52	! s1
	fmuld	%f26,%f8,%f40	! s2
	fmuld	%f26,%f12,%f44	! s3

	faddd	%f0,%f16, %f2
	faddd	%f4,%f18,%f6
	faddd	%f8,%f20,%f10
	faddd	%f12,%f22,%f14

	fsubd	%f2,%f16, %f2
	 ldd	[%o7+8*11],%f16		! more constants
	fsubd	%f6,%f18,%f6
	 ldd	[%o7+8*12],%f18
	fsubd	%f10,%f20,%f10
	 ldd	[%o7+8*13],%f20
	fsubd	%f14,%f22,%f14

	fsubd	%f0,%f2,%f0
	 std	%f2,[%i0+8*5] 		! r0hi
	fsubd	%f4,%f6,%f4
	 std	%f6,[%i0+8*7] 		! r1hi
	fsubd	%f8,%f10,%f8
	 std	%f10,[%i0+8*9] 		! r2hi
	fsubd	%f12,%f14,%f12
	 std	%f14,[%i0+8*11]		! r3hi

	faddd	%f52,%f16, %f54
	faddd	%f40,%f18,%f42
	faddd	%f44,%f20,%f46

	fsubd	%f54,%f16, %f54
	fsubd	%f42,%f18,%f42
	fsubd	%f46,%f20,%f46

	fsubd	%f52,%f54,%f52
	fsubd	%f40,%f42,%f40
	fsubd	%f44,%f46,%f44

	ldx	[%sp+LOCALS],%fsr		! restore %fsr

	std	%f0,[%i0+8*4] 		! r0lo
	std	%f4,[%i0+8*6] 		! r1lo
	std	%f8,[%i0+8*8] 		! r2lo
	std	%f12,[%i0+8*10]		! r3lo

	std	%f54,[%i0+8*13]
	std	%f42,[%i0+8*15]
	std	%f46,[%i0+8*17]

	std	%f52,[%i0+8*12]
	std	%f40,[%i0+8*14]
	std	%f44,[%i0+8*16]

	add	%o7,poly1305_blocks_fma-.Lconsts_fma,%o0
	add	%o7,poly1305_emit_fma-.Lconsts_fma,%o1
	STPTR	%o0,[%i2]
	STPTR	%o1,[%i2+SIZE_T]

	ret
	restore	%g0,1,%o0			! return 1

.Lno_key_fma:
	ret
	restore	%g0,%g0,%o0			! return 0
.type	poly1305_init_fma,#function
.size	poly1305_init_fma,.-poly1305_init_fma

.align	32
poly1305_blocks_fma:
	save	%sp,-STACK_FRAME-48,%sp
	srln	%i2,4,%i2

	brz,pn	%i2,.Labort
	sub	%i2,1,%i2

1:	call	.+8
	add	%o7,.Lconsts_fma-1b,%o7

	ldd	[%o7+8*0],%f16			! load constants
	ldd	[%o7+8*1],%f18
	ldd	[%o7+8*2],%f20
	ldd	[%o7+8*3],%f22
	ldd	[%o7+8*4],%f24
	ldd	[%o7+8*5],%f26

	ldd	[%i0+8*0],%f0 		! load [biased] hash value
	ldd	[%i0+8*1],%f4
	ldd	[%i0+8*2],%f8
	ldd	[%i0+8*3],%f12

	std	%f16,[%sp+LOCALS+8*0]		! input "template"
	sethi	%hi((1023+52+96)<<20),%o3
	std	%f18,[%sp+LOCALS+8*1]
	or	%i3,%o3,%o3
	std	%f20,[%sp+LOCALS+8*2]
	st	%o3,[%sp+LOCALS+8*3]

	and	%i1,7,%l2
	andn	%i1,7,%i1			! align pointer
	mov	8,%l0
	sll	%l2,3,%l2
	mov	16,%l1
	neg	%l2,%l3

	ldxa	[%i1+%g0]0x88,%o0		! load little-endian input
	brz	%l2,.Linp_aligned_fma
	ldxa	[%i1+%l0]0x88,%o2

	ldxa	[%i1+%l1]0x88,%o4
	add	%i1,8,%i1

	srlx	%o0,%l2,%o0			! align data
	sllx	%o2,%l3,%o1
	srlx	%o2,%l2,%o2
	or	%o1,%o0,%o0
	sllx	%o4,%l3,%o3
	srlx	%o4,%l2,%o4			! pre-shift
	or	%o3,%o2,%o2

.Linp_aligned_fma:
	srlx	%o0,32,%o1
	movrz	%i2,0,%l1
	srlx	%o2,32,%o3
	add	%l1,%i1,%i1			! conditional advance

	st	%o0,[%sp+LOCALS+8*0+4]		! fill "template"
	st	%o1,[%sp+LOCALS+8*1+4]
	st	%o2,[%sp+LOCALS+8*2+4]
	st	%o3,[%sp+LOCALS+8*3+4]

	ldd	[%i0+8*4],%f28 		! load key
	ldd	[%i0+8*5],%f30
	ldd	[%i0+8*6],%f32
	ldd	[%i0+8*7],%f34
	ldd	[%i0+8*8],%f36
	ldd	[%i0+8*9],%f38
	ldd	[%i0+8*10],%f48
	ldd	[%i0+8*11],%f50
	ldd	[%i0+8*12],%f52
	ldd	[%i0+8*13],%f54
	ldd	[%i0+8*14],%f40
	ldd	[%i0+8*15],%f42
	ldd	[%i0+8*16],%f44
	ldd	[%i0+8*17],%f46

	stx	%fsr,[%sp+LOCALS+8*4]		! save original %fsr
	ldx	[%o7+8*6],%fsr			! load new %fsr

	subcc	%i2,1,%i2
	movrz	%i2,0,%l1

	ldd	[%sp+LOCALS+8*0],%f56		! load biased input
	ldd	[%sp+LOCALS+8*1],%f58
	ldd	[%sp+LOCALS+8*2],%f60
	ldd	[%sp+LOCALS+8*3],%f62

	fsubd	%f0,%f16, %f0		! de-bias hash value
	fsubd	%f4,%f18,%f4
	 ldxa	[%i1+%g0]0x88,%o0		! modulo-scheduled input load
	fsubd	%f8,%f20,%f8
	fsubd	%f12,%f22,%f12
	 ldxa	[%i1+%l0]0x88,%o2

	fsubd	%f56,%f16, %f56  		! de-bias input
	fsubd	%f58,%f18,%f58
	fsubd	%f60,%f20,%f60
	fsubd	%f62,%f22,%f62

	brz	%l2,.Linp_aligned_fma2
	add	%l1,%i1,%i1			! conditional advance

	sllx	%o0,%l3,%o1			! align data
	srlx	%o0,%l2,%o3
	or	%o1,%o4,%o0
	sllx	%o2,%l3,%o1
	srlx	%o2,%l2,%o4			! pre-shift
	or	%o3,%o1,%o2
.Linp_aligned_fma2:
	srlx	%o0,32,%o1
	srlx	%o2,32,%o3

	faddd	%f0,%f56,%f56			! accumulate input
	 stw	%o0,[%sp+LOCALS+8*0+4]
	faddd	%f4,%f58,%f58
	 stw	%o1,[%sp+LOCALS+8*1+4]
	faddd	%f8,%f60,%f60
	 stw	%o2,[%sp+LOCALS+8*2+4]
	faddd	%f12,%f62,%f62
	 stw	%o3,[%sp+LOCALS+8*3+4]

	b	.Lentry_fma
	nop

.align	16
.Loop_fma:
	ldxa	[%i1+%g0]0x88,%o0		! modulo-scheduled input load
	ldxa	[%i1+%l0]0x88,%o2
	movrz	%i2,0,%l1

	faddd	%f52,%f0,%f0 		! accumulate input
	faddd	%f54,%f2,%f2
	faddd	%f62,%f8,%f8
	faddd	%f60,%f10,%f10

	brz,pn	%l2,.Linp_aligned_fma3
	add	%l1,%i1,%i1			! conditional advance

	sllx	%o0,%l3,%o1			! align data
	srlx	%o0,%l2,%o3
	or	%o1,%o4,%o0
	sllx	%o2,%l3,%o1
	srlx	%o2,%l2,%o4			! pre-shift
	or	%o3,%o1,%o2

.Linp_aligned_fma3:
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! base 2^48 -> base 2^32
	faddd	%f20,%f4,%f52
	 srlx	%o0,32,%o1
	faddd	%f20,%f6,%f54
	 srlx	%o2,32,%o3
	faddd	%f24,%f12,%f60
	 st	%o0,[%sp+LOCALS+8*0+4]		! fill "template"
	faddd	%f24,%f14,%f62
	 st	%o1,[%sp+LOCALS+8*1+4]
	faddd	%f18,%f0,%f48
	 st	%o2,[%sp+LOCALS+8*2+4]
	faddd	%f18,%f2,%f50
	 st	%o3,[%sp+LOCALS+8*3+4]
	faddd	%f22,%f8,%f56
	faddd	%f22,%f10,%f58

	fsubd	%f52,%f20,%f52
	fsubd	%f54,%f20,%f54
	fsubd	%f60,%f24,%f60
	fsubd	%f62,%f24,%f62
	fsubd	%f48,%f18,%f48
	fsubd	%f50,%f18,%f50
	fsubd	%f56,%f22,%f56
	fsubd	%f58,%f22,%f58

	fsubd	%f4,%f52,%f4
	fsubd	%f6,%f54,%f6
	fsubd	%f12,%f60,%f12
	fsubd	%f14,%f62,%f14
	fsubd	%f8,%f56,%f8
	fsubd	%f10,%f58,%f10
	fsubd	%f0,%f48,%f0
	fsubd	%f2,%f50,%f2

	faddd	%f4,%f48,%f4
	faddd	%f6,%f50,%f6
	faddd	%f12,%f56,%f12
	faddd	%f14,%f58,%f14
	faddd	%f8,%f52,%f8
	faddd	%f10,%f54,%f10
	.word	0x81be805d !fmaddd	%f26,%f60,%f0,%f0
	.word	0x85be845f !fmaddd	%f26,%f62,%f2,%f2

	faddd	%f4,%f6,%f58
	 ldd	[%i0+8*12],%f52		! reload constants
	faddd	%f12,%f14,%f62
	 ldd	[%i0+8*13],%f54
	faddd	%f8,%f10,%f60
	 ldd	[%i0+8*10],%f48
	faddd	%f0,%f2,%f56
	 ldd	[%i0+8*11],%f50

.Lentry_fma:
	fmuld	%f58,%f44,%f0
	fmuld	%f58,%f46,%f2
	fmuld	%f58,%f32,%f8
	fmuld	%f58,%f34,%f10
	fmuld	%f58,%f28,%f4
	fmuld	%f58,%f30,%f6
	fmuld	%f58,%f36,%f12
	fmuld	%f58,%f38,%f14

	.word	0x81bfc055 !fmaddd	%f62,%f52,%f0,%f0
	.word	0x85bfc457 !fmaddd	%f62,%f54,%f2,%f2
	.word	0x91bfd04d !fmaddd	%f62,%f44,%f8,%f8
	.word	0x95bfd44f !fmaddd	%f62,%f46,%f10,%f10
	.word	0x89bfc849 !fmaddd	%f62,%f40,%f4,%f4
	.word	0x8dbfcc4b !fmaddd	%f62,%f42,%f6,%f6
	.word	0x99bfd85c !fmaddd	%f62,%f28,%f12,%f12
	.word	0x9dbfdc5e !fmaddd	%f62,%f30,%f14,%f14

	.word	0x81bf4049 !fmaddd	%f60,%f40,%f0,%f0
	.word	0x85bf444b !fmaddd	%f60,%f42,%f2,%f2
	.word	0x91bf505c !fmaddd	%f60,%f28,%f8,%f8
	.word	0x95bf545e !fmaddd	%f60,%f30,%f10,%f10
	.word	0x89bf484d !fmaddd	%f60,%f44,%f4,%f4
	 ldd	[%sp+LOCALS+8*0],%f52		! load [biased] input
	.word	0x8dbf4c4f !fmaddd	%f60,%f46,%f6,%f6
	 ldd	[%sp+LOCALS+8*1],%f54
	.word	0x99bf5841 !fmaddd	%f60,%f32,%f12,%f12
	 ldd	[%sp+LOCALS+8*2],%f62
	.word	0x9dbf5c43 !fmaddd	%f60,%f34,%f14,%f14
	 ldd	[%sp+LOCALS+8*3],%f60

	.word	0x81be405c !fmaddd	%f56,%f28,%f0,%f0
	 fsubd	%f52,%f16, %f52  		! de-bias input
	.word	0x85be445e !fmaddd	%f56,%f30,%f2,%f2
	 fsubd	%f54,%f18,%f54
	.word	0x91be5045 !fmaddd	%f56,%f36,%f8,%f8
	 fsubd	%f62,%f20,%f62
	.word	0x95be5447 !fmaddd	%f56,%f38,%f10,%f10
	 fsubd	%f60,%f22,%f60
	.word	0x89be4841 !fmaddd	%f56,%f32,%f4,%f4
	.word	0x8dbe4c43 !fmaddd	%f56,%f34,%f6,%f6
	.word	0x99be5851 !fmaddd	%f56,%f48,%f12,%f12
	.word	0x9dbe5c53 !fmaddd	%f56,%f50,%f14,%f14

	bcc	SIZE_T_CC,.Loop_fma
	subcc	%i2,1,%i2

	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! base 2^48 -> base 2^32
	faddd	%f0,%f18,%f48
	faddd	%f2,%f18,%f50
	faddd	%f8,%f22,%f56
	faddd	%f10,%f22,%f58
	faddd	%f4,%f20,%f52
	faddd	%f6,%f20,%f54
	faddd	%f12,%f24,%f60
	faddd	%f14,%f24,%f62

	fsubd	%f48,%f18,%f48
	fsubd	%f50,%f18,%f50
	fsubd	%f56,%f22,%f56
	fsubd	%f58,%f22,%f58
	fsubd	%f52,%f20,%f52
	fsubd	%f54,%f20,%f54
	fsubd	%f60,%f24,%f60
	fsubd	%f62,%f24,%f62

	fsubd	%f4,%f52,%f4
	fsubd	%f6,%f54,%f6
	fsubd	%f12,%f60,%f12
	fsubd	%f14,%f62,%f14
	fsubd	%f8,%f56,%f8
	fsubd	%f10,%f58,%f10
	fsubd	%f0,%f48,%f0
	fsubd	%f2,%f50,%f2

	faddd	%f4,%f48,%f4
	faddd	%f6,%f50,%f6
	faddd	%f12,%f56,%f12
	faddd	%f14,%f58,%f14
	faddd	%f8,%f52,%f8
	faddd	%f10,%f54,%f10
	.word	0x81be805d !fmaddd	%f26,%f60,%f0,%f0
	.word	0x85be845f !fmaddd	%f26,%f62,%f2,%f2

	faddd	%f4,%f6,%f58
	faddd	%f12,%f14,%f62
	faddd	%f8,%f10,%f60
	faddd	%f0,%f2,%f56

	faddd	%f58,%f18,%f58  		! bias
	faddd	%f62,%f22,%f62
	faddd	%f60,%f20,%f60
	faddd	%f56,%f16, %f56

	ldx	[%sp+LOCALS+8*4],%fsr		! restore saved %fsr

	std	%f58,[%i0+8*1]			! store [biased] hash value
	std	%f62,[%i0+8*3]
	std	%f60,[%i0+8*2]
	std	%f56,[%i0+8*0]

.Labort:
	ret
	restore
.type	poly1305_blocks_fma,#function
.size	poly1305_blocks_fma,.-poly1305_blocks_fma
.align	32
poly1305_emit_fma:
	save	%sp,-STACK_FRAME,%sp

	ld	[%i0+8*0+0],%l5		! load hash
	ld	[%i0+8*0+4],%l0
	ld	[%i0+8*1+0],%o0
	ld	[%i0+8*1+4],%l1
	ld	[%i0+8*2+0],%o1
	ld	[%i0+8*2+4],%l2
	ld	[%i0+8*3+0],%o2
	ld	[%i0+8*3+4],%l3

	sethi	%hi(0xfff00000),%o3
	andn	%l5,%o3,%l5			! mask exponent
	andn	%o0,%o3,%o0
	andn	%o1,%o3,%o1
	andn	%o2,%o3,%o2			! can be partially reduced...
	mov	3,%o3

	srl	%o2,2,%i3			! ... so reduce
	and	%o2,%o3,%l4
	andn	%o2,%o3,%o2
	add	%i3,%o2,%o2

	addcc	%o2,%l0,%l0
	addccc	%l5,%l1,%l1
	addccc	%o0,%l2,%l2
	addccc	%o1,%l3,%l3
	addc	%g0,%l4,%l4

	addcc	%l0,5,%l5			! compare to modulus
	addccc	%l1,0,%o0
	addccc	%l2,0,%o1
	addccc	%l3,0,%o2
	addc	%l4,0,%o3

	srl	%o3,2,%o3			! did it carry/borrow?
	neg	%o3,%o3
	sra	%o3,31,%o3			! mask

	andn	%l0,%o3,%l0
	and	%l5,%o3,%l5
	andn	%l1,%o3,%l1
	and	%o0,%o3,%o0
	or	%l5,%l0,%l0
	ld	[%i2+0],%l5			! load nonce
	andn	%l2,%o3,%l2
	and	%o1,%o3,%o1
	or	%o0,%l1,%l1
	ld	[%i2+4],%o0
	andn	%l3,%o3,%l3
	and	%o2,%o3,%o2
	or	%o1,%l2,%l2
	ld	[%i2+8],%o1
	or	%o2,%l3,%l3
	ld	[%i2+12],%o2

	addcc	%l5,%l0,%l0			! accumulate nonce
	addccc	%o0,%l1,%l1
	addccc	%o1,%l2,%l2
	addc	%o2,%l3,%l3

	stb	%l0,[%i1+0]			! write little-endian result
	srl	%l0,8,%l0
	stb	%l1,[%i1+4]
	srl	%l1,8,%l1
	stb	%l2,[%i1+8]
	srl	%l2,8,%l2
	stb	%l3,[%i1+12]
	srl	%l3,8,%l3

	stb	%l0,[%i1+1]
	srl	%l0,8,%l0
	stb	%l1,[%i1+5]
	srl	%l1,8,%l1
	stb	%l2,[%i1+9]
	srl	%l2,8,%l2
	stb	%l3,[%i1+13]
	srl	%l3,8,%l3

	stb	%l0,[%i1+2]
	srl	%l0,8,%l0
	stb	%l1,[%i1+6]
	srl	%l1,8,%l1
	stb	%l2,[%i1+10]
	srl	%l2,8,%l2
	stb	%l3,[%i1+14]
	srl	%l3,8,%l3

	stb	%l0,[%i1+3]
	stb	%l1,[%i1+7]
	stb	%l2,[%i1+11]
	stb	%l3,[%i1+15]

	ret
	restore
.type	poly1305_emit_fma,#function
.size	poly1305_emit_fma,.-poly1305_emit_fma
.align	64
.Lconsts_fma:
.word	0x43300000,0x00000000		! 2^(52+0)
.word	0x45300000,0x00000000		! 2^(52+32)
.word	0x47300000,0x00000000		! 2^(52+64)
.word	0x49300000,0x00000000		! 2^(52+96)
.word	0x4b500000,0x00000000		! 2^(52+130)

.word	0x37f40000,0x00000000		! 5/2^130
.word	0,1<<30				! fsr: truncate, no exceptions

.word	0x44300000,0x00000000		! 2^(52+16+0)
.word	0x46300000,0x00000000		! 2^(52+16+32)
.word	0x48300000,0x00000000		! 2^(52+16+64)
.word	0x4a300000,0x00000000		! 2^(52+16+96)
.word	0x3e300000,0x00000000		! 2^(52+16+0-96)
.word	0x40300000,0x00000000		! 2^(52+16+32-96)
.word	0x42300000,0x00000000		! 2^(52+16+64-96)
.asciz	"Poly1305 for SPARCv9/VIS3/FMA, CRYPTOGAMS by <appro@openssl.org>"
.align	4
