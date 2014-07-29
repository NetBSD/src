/*	$NetBSD: pio_subr.S,v 1.16 2014/07/29 16:19:45 joerg Exp $	*/

/*
 * Copyright (c) 2003 Matt Thomas
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * Assembly note:
 *	We use rotlw instead of slw because rotlw ignores bit 26 and slw
 *	doesn't.  However, this may make the high bits of the offset rotate
 *	in the low bits but if that happens then the offset was too large
 *	to being with.
 */
#ifdef DEBUG
#define	DBGSYNC	sync
#else
#define	DBGSYNC	/* nothing */
#endif

#undef DBGSYNC
#define	DBGSYNC	msync
#define	eieio	mbar 0

/* LINTSTUB: include <sys/param.h> */
/* LINTSTUB: include <sys/types.h> */
/* LINTSTUB: include <sys/bus.h> */

/* LINTSTUB: Func: void out8(volatile u_int8_t *a, u_int8_t v) */

ENTRY(out8)
	stbx	%r4,0,%r3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v) */
/* LINTSTUB: Func: void bsw1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v) */

ENTRY(bsw1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw1)
	stbx	%r6,%r4,%r5	/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out16(volatile u_int16_t *a, u_int16_t v) */

ENTRY(out16)
	sth	%r4,0(%r3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */
/* LINTSTUB: Func: void bsw2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */

ENTRY(bsw2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw2)
	sthx	%r6,%r4,%r5	/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out32(volatile u_int32_t *a, u_int32_t v) */

ENTRY(out32)
	stw	%r4,0(%r3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */
/* LINTSTUB: Func: void bsw4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */

ENTRY(bsw4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw4)
	stwx	%r6,%r4,%r5	/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */
/* LINTSTUB: Func: void bsw8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */
ENTRY(bsw8_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw8)
#ifdef __ARCH64__
	stdx	%r6,%r4,%r5	/* store value */
#else
	trap			/* die */
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out16rb(volatile u_int16_t *a, u_int16_t v) */

ENTRY(out16rb)
	sthbrx	%r4,0,%r3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */
/* LINTSTUB: Func: void bsw2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */


ENTRY(bsw2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw2rb)
	sthbrx	%r6,%r4,%r5	/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out32rb(volatile u_int32_t *a, u_int32_t v) */

ENTRY(out32rb)
	stwbrx	%r4,0,%r3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */
/* LINTSTUB: Func: void bsw4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */

ENTRY(bsw4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw4rb)
	stwbrx	%r6,%r4,%r5	/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */
/* LINTSTUB: Func: void bsw8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */

ENTRY(bsw8rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsw8rb)
#ifdef __ARCH64__
	stdbrx	%r6,%r4,%r5	/* store value */
#else
	trap			/* die */
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in8(const volatile u_int8_t *a) */

ENTRY(in8)
	lbz	%r3,0(%r3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr1)
	lbzx	%r3,%r4,%r5	/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in16(const volatile u_int16_t *a) */

ENTRY(in16)
	lhz	%r3,0(%r3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr2)
	lhzx	%r3,%r4,%r5	/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in32(const volatile u_int32_t *a) */

ENTRY(in32)
	lwz	%r3,0(%r3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr4)
	lwzx	%r3,%r4,%r5	/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: u_int64_t bsr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: u_int64_t bsr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr8_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr8)
#ifdef __ARCH64__
	lwdx	%r3,%r4,%r5	/* load value */
#else
	trap
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in16rb(const volatile u_int16_t *a) */

ENTRY(in16rb)
	lhbrx	%r3,0,%r3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr2rb)
	lhbrx	%r3,%r4,%r5	/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in32rb(const volatile u_int32_t *a) */

ENTRY(in32rb)
	lwbrx	%r3,0,%r3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr4rb)
	lwbrx	%r3,%r4,%r5	/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: u_int64_t bsr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: u_int64_t bsr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY(bsr8rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsr8rb)
#ifdef __ARCH64__
	ldbrx	%r3,%r4,%r5	/* load value */
#else
	trap
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs8(volatile u_int8_t *dst, const u_int8_t *src, size_t len) */

ENTRY(bswm1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bswm1)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(outs8)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-1	/* pre-decrement */
1:	lbzu	%r0,1(%r4)	/* load and increment */
	stb	%r0,0(%r3)	/* store */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs16(volatile u_int16_t *dst, const u_int16_t *src, size_t len) */

ENTRY(bswm2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bswm2)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(outs16)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,1	/* len *= 2 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-2	/* pre-decrement */
1:	lhzu	%r0,2(%r4)	/* load and increment */
	sth	%r0,0(%r3)	/* store */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs32(volatile u_int32_t *dst, const u_int32_t *src, size_t len) */

ENTRY(bswm4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bswm4)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(outs32)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,2	/* len *= 4 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-4	/* pre-decrement */
1:	lwzu	%r0,4(%r4)	/* load and increment */
	stw	%r0,0(%r3)	/* store */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */

#ifdef _LP64
ENTRY(bswm8_s)
	ld	%r0,0(%r3)	/* get log2(stride) */
	rotld	%r5,%r5,%r0	/* shift offset */
ENTRY(bswm8)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(outs64)
	cmpdi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	sldi	%r5,%r5,2	/* len *= 4 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-4	/* pre-decrement */
1:	ldu	%r0,4(%r4)	/* load and increment */
	std	%r0,0(%r3)	/* store */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */
#else
ENTRY(bswm8_s)
ENTRY(bswm8)
	trap			/* die */
#endif

/* LINTSTUB: Func: void bswm2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs16rb(volatile u_int16_t *dst, const u_int16_t *src, size_t len) */

ENTRY(bswm2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bswm2rb)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(outs16rb)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,1	/* len *= 2 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-2	/* pre-decrement */
1:	lwzu	%r0,2(%r4)	/* load and increment */
	sthbrx	%r0,0,%r3	/* store (byte-reversed) */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs32rb(volatile u_int32_t *dst, const u_int32_t *src, size_t len) */

ENTRY(bswm4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bswm4rb)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(outs32rb)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,2	/* len *= 4 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-4	/* pre-decrement */
1:	lwzu	%r0,4(%r4)	/* load and increment */
	stwbrx	%r0,0,%r3	/* store (byte-reversed) */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */
ENTRY(bswm8rb_s)
ENTRY(bswm8rb)
	trap

/* LINTSTUB: Func: void bsrm1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins8(const volatile u_int8_t *src, u_int8_t *dst, size_t len) */

ENTRY(bsrm1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsrm1)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(ins8)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-1	/* pre-decrement */
1:	lbz	%r0,0(%r3)	/* load value */
	stbu	%r0,1(%r4)	/* store and increment */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins16(const volatile u_int16_t *src, u_int16_t *dst, size_t len) */

ENTRY(bsrm2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsrm2)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(ins16)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,1	/* len *= 2 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-2	/* pre-decrement */
1:	lhz	%r0,0(%r3)	/* load value */
	sthu	%r0,2(%r4)	/* store and increment */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins32(const volatile u_int32_t *src, u_int32_t *dst, size_t len) */

ENTRY(bsrm4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsrm4)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(ins32)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,2	/* len *= 4 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-4	/* pre-decrement */
1:	lwz	%r0,0(%r3)	/* load value */
	stwu	%r0,4(%r4)	/* store and increment */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
ENTRY(bsrm8_s)
ENTRY(bsrm8)
	trap

/* LINTSTUB: Func: void bsrm2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins16rb(const volatile u_int16_t *src, u_int16_t *dst, size_t len) */

ENTRY(bsrm2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsrm2rb)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(ins16rb)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,1	/* len *= 2 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-2	/* pre-decrement */
1:	lhbrx	%r0,0,%r3	/* load value (byte reversed) */
	sthu	%r0,2(%r4)	/* store and increment */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins32rb(const volatile u_int32_t *src, u_int32_t *dst, size_t len) */
ENTRY(bsrm4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	rotlw	%r5,%r5,%r0	/* shift offset */
ENTRY(bsrm4rb)
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	mr	%r5,%r7		/* move count to argument 2 register */
ENTRY(ins32rb)
	cmpwi	%r5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r5,%r5,-1	/* len -= 1 */
	slwi	%r5,%r5,2	/* len *= 4 */
	add	%r5,%r5,%r4	/* len += src */
	addi	%r4,%r4,-4	/* pre-decrement */
1:	lwbrx	%r0,0,%r3	/* load value (byte reversed) */
	stwu	%r0,4(%r4)	/* store and increment */
	cmplw	%r4,%r5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
ENTRY(bsrm8rb_s)
ENTRY(bsrm8rb)
	trap

/* LINTSTUB: Func: void bswr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c); */
ENTRY(bswr1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,1		/* distance between dst bytes */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbswr1_enter
ENTRY(bswr1)
	li	%r8,1		/* distance between dst bytes */
.Lbswr1_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-1	/* pre-decrement */
	sub	%r3,%r3,%r8
1:	lbzu	%r0,1(%r6)	/* load and increment */
	stbux	%r0,%r3,%r8	/* store and add distance */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
ENTRY(bswr2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,2		/* distance between dst halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbswr2_enter
ENTRY(bswr2)
	li	%r8,2		/* distance between dst halfwords */
.Lbswr2_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,1	/* len *= 2 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-2	/* pre-decrement */
	sub	%r3,%r3,%r8
1:	lhzu	%r0,2(%r6)	/* load and increment */
	sthux	%r0,%r3,%r8	/* store and add distance */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
ENTRY(bswr2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,2		/* distance between dst halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbswr2rb_enter
ENTRY(bswr2rb)
	li	%r8,2		/* distance between dst halfwords */
.Lbswr2rb_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,1	/* len *= 2 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-2	/* pre-decrement */
1:	lhzu	%r0,2(%r6)	/* load and increment */
	sthbrx	%r0,0,%r3	/* store (reversed) */
	add	%r3,%r3,%r8	/* dst += distance */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
ENTRY(bswr4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,4		/* distance between dst halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbswr4_enter
ENTRY(bswr4)
	li	%r8,4		/* distance between dst halfwords */
.Lbswr4_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,2	/* len *= 4 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-4	/* pre-decrement */
	sub	%r3,%r3,%r8
1:	lwzu	%r0,4(%r6)	/* load and increment */
	stwux	%r0,%r3,%r8	/* store and add distance */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
ENTRY(bswr4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,4		/* distance between dst halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbswr4rb_enter
ENTRY(bswr4rb)
	li	%r8,4		/* distance between dst halfwords */
.Lbswr4rb_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,2	/* len *= 4 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-4	/* pre-decrement */
1:	lwzu	%r0,4(%r6)	/* load and increment */
	stwbrx	%r0,0,%r3	/* store (reversed) */
	add	%r3,%r3,%r8	/* dst += distance */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
ENTRY(bswr8_s)
ENTRY(bswr8rb_s)
	lwz	%r0,0(%r3)
	rotlw	%r5,%r5,%r0
ENTRY(bswr8)
ENTRY(bswr8rb)
	trap

/* LINTSTUB: Func: void bsrr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c); */
ENTRY(bsrr1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,1		/* distance between src bytes */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbsrr1_enter
ENTRY(bsrr1)
	li	%r8,1		/* distance between src bytes */
.Lbsrr1_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-1	/* pre-decrement */
	sub	%r3,%r3,%r8
1:	lbzux	%r0,%r3,%r8	/* load value and add distance */
	stbu	%r0,1(%r6)	/* store and increment */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
ENTRY(bsrr2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,2		/* distance between src halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbsrr2_enter
ENTRY(bsrr2)
	li	%r8,2		/* distance between src halfwords */
.Lbsrr2_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,1	/* len *= 2 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-2	/* pre-decrement */
	sub	%r3,%r3,%r8
1:	lhzux	%r0,%r3,%r8	/* load value and add distance */
	sthu	%r0,2(%r6)	/* store and increment */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
ENTRY(bsrr2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,2		/* distance between source halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbsrr2rb_enter
ENTRY(bsrr2rb)
	li	%r8,2		/* distance between source halfwords */
.Lbsrr2rb_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,1	/* len *= 2 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-2	/* pre-decrement */
1:	lhbrx	%r0,0,%r3	/* load value (reversed) */
	add	%r3,%r3,%r8	/* src += distance */
	sthu	%r0,2(%r6)	/* store and increment */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
ENTRY(bsrr4_s)
	lwz	%r0,0(%r3)		/* get log2(stride) */
	li	%r8,4		/* distance between src words */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbsrr4_enter
ENTRY(bsrr4)
	li	%r8,4		/* distance between src words */
.Lbsrr4_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,2	/* len *= 4 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-4	/* pre-decrement */
	sub	%r3,%r3,%r8
1:	lwzux	%r0,%r3,%r8	/* load value and add distance */
	stwu	%r0,4(%r6)	/* store and increment */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
ENTRY(bsrr4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,4		/* distance between src words */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbsrr4rb_enter
ENTRY(bsrr4rb)
	li	%r8,4		/* distance between src words */
.Lbsrr4rb_enter:
	add	%r3,%r4,%r5	/* add offset to handle & place in argument 0 */
	mr	%r4,%r6		/* move addr to argument 1 register */
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	%r7,%r7,-1	/* len -= 1 */
	slwi	%r7,%r7,2	/* len *= 4 */
	add	%r7,%r7,%r6	/* len += src */
	addi	%r6,%r6,-4	/* pre-decrement */
1:	lwbrx	%r0,0,%r3	/* load value (reversed) */
	add	%r3,%r3,%r8	/* src += distance */
	sthu	%r0,4(%r6)	/* store and increment */
	cmplw	%r6,%r7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
ENTRY(bsrr8_s)
ENTRY(bsrr8rb_s)
	lwz	%r0,0(%r3)
	rotlw	%r5,%r5,%r0
ENTRY(bsrr8)
ENTRY(bsrr8rb)
	trap

/* LINTSTUB: Func: void bssr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v, bus_size_t c); */
ENTRY(bssr1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,1		/* distance between src bytes */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbssr1_enter
ENTRY(bssr1)
	li	%r8,1		/* distance between src bytes */
.Lbssr1_enter:
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	%r7,%r7,-1	/* len -= 1 */
	stbx	%r6,%r4,%r5	/* store value */
	add	%r5,%r5,%r8	/* add offset */
	cmpwi	%r7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
ENTRY(bssr2_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,2		/* distance between src halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbssr2_enter
ENTRY(bssr2)
	li	%r8,2		/* distance between src halfwords */
.Lbssr2_enter:
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	%r7,%r7,-1	/* len -= 1 */
	sthx	%r6,%r4,%r5	/* store value */
	add	%r5,%r5,%r8	/* add offset */
	cmpwi	%r7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
ENTRY(bssr2rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,2		/* distance between src halfwords */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbssr2rb_enter
ENTRY(bssr2rb)
	li	%r8,2		/* distance between src halfwords */
.Lbssr2rb_enter:
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	%r7,%r7,-1	/* len -= 1 */
	sthbrx	%r6,%r4,%r5	/* store value */
	add	%r5,%r5,%r8	/* add offset */
	cmpwi	%r7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
ENTRY(bssr4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,4		/* distance between src words */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbssr4_enter
ENTRY(bssr4)
	li	%r8,4		/* distance between src words */
.Lbssr4_enter:
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	%r7,%r7,-1	/* len -= 1 */
	stwx	%r6,%r4,%r5	/* store value */
	add	%r5,%r5,%r8	/* add offset */
	cmpwi	%r7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
ENTRY(bssr4rb_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	li	%r8,4		/* distance between src words */
	rotlw	%r5,%r5,%r0	/* shift offset */
	rotlw	%r8,%r8,%r0	/* shift distance */
	b	.Lbssr4rb_enter
ENTRY(bssr4rb)
	li	%r8,4		/* distance between src words */
.Lbssr4rb_enter:
	cmpwi	%r7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	%r7,%r7,-1	/* len -= 1 */
	stwbrx	%r6,%r4,%r5	/* store value */
	add	%r5,%r5,%r8	/* add offset */
	cmpwi	%r7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
ENTRY(bssr8_s)
ENTRY(bssr8rb_s)
	lwz	%r0,0(%r3)
	rotlw	%r5,%r5,%r0
ENTRY(bssr8)
ENTRY(bssr8rb)
	trap

/* LINTSTUB: Func: void bscr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY(bscr1_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	b	.Lbscr1_enter
ENTRY(bscr1)
	li	%r0,0		/* non stride */
.Lbscr1_enter:
	li	%r9,1		/* distance between src bytes */
	rotlw	%r9,%r9,%r0		/* shift distance */
	cmpwi	%r8,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	rotlw	%r5,%r5,%r0	/* shift src offset */
	rotlw	%r7,%r7,%r0	/* shift dest offset */
	add	%r10,%r4,%r5	/* calculate src end address */
	add	%r11,%r6,%r7	/* calculate dest end address */
	mtctr	%r8
	cmpw	%r10,%r11	/* compare end address */
	blt	2f		/* jump if h + o < h2 + o2 */

				/* h + o >= h2 + o2 */
1:	lbzx	%r12,%r4,%r5	/* load value */
	stbx	%r12,%r6,%r7	/* store value */
	add	%r5,%r5,%r9	/* src offset += 1 */
	add	%r7,%r7,%r9	/* dest offset += 1 */
	bdnz+	1b		/* jump if len != 0 */
	b	.Lbscr1_end	/* end */

				/* h + o < h2 + o2 */
2:	addi	%r8,%r8,-1	/* len -= 1 */
	rotlw	%r8,%r8,%r0	/* shift len */
	add	%r5,%r10,%r8	/* src offset = o + len - 1 */
	add	%r7,%r11,%r8	/* dest offset = o2 + len - 1 */
3:	lbzx	%r12,%r4,%r5	/* load value */
	stbx	%r12,%r6,%r7	/* store value */
	sub	%r5,%r5,%r9	/* src offset -= 1 */
	sub	%r7,%r7,%r9	/* dest offset -= 1 */
	bdnz+	3b		/* jump if len != 0 */
.Lbscr1_end:
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bscr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY(bscr2_s)
	lwz	%r0,0(%r3)		/* get log2(stride) */
	b	.Lbscr2_enter
ENTRY(bscr2)
	li	%r0,0		/* non stride */
.Lbscr2_enter:
	li	%r9,2		/* distance between src halfwords */
	rotlw	%r9,%r9,%r0		/* shift distance */
	cmpwi	%r8,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	rotlw	%r5,%r5,%r0	/* shift src offset */
	rotlw	%r7,%r7,%r0	/* shift dest offset */
	add	%r10,%r4,%r5	/* calculate src end address */
	add	%r11,%r6,%r7	/* calculate dest end address */
	mtctr	%r8
	cmpw	%r10,%r11	/* compare end address */
	blt	2f		/* jump if h + o < h2 + o2 */

				/* h + o >= h2 + o2 */
1:	lhzx	%r12,%r4,%r5	/* load value */
	sthx	%r12,%r6,%r7	/* store value */
	add	%r5,%r5,%r9	/* src offset += 2 */
	add	%r7,%r7,%r9	/* dest offset += 2 */
	bdnz+	1b		/* jump if len != 0 */
	b	.Lbscr2_end	/* end */

				/* h + o < h2 + o2 */
2:	addi	%r8,%r8,-1	/* len -= 1 */
	rotlw	%r8,%r8,%r0	/* shift len */
	add	%r5,%r10,%r8	/* src offset = o + len - 1 */
	add	%r7,%r11,%r8	/* dest offset = o2 + len - 1 */
3:	lhzx	%r12,%r4,%r5	/* load value */
	sthx	%r12,%r6,%r7	/* store value */
	sub	%r5,%r5,%r9	/* src offset -= 2 */
	sub	%r7,%r7,%r9	/* dest offset -= 2 */
	bdnz+	3b		/* jump if len != 0 */
.Lbscr2_end:
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bscr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY(bscr4_s)
	lwz	%r0,0(%r3)	/* get log2(stride) */
	b	.Lbscr4_enter
ENTRY(bscr4)
	li	%r0,0		/* non stride */
.Lbscr4_enter:
	li	%r9,4		/* distance between src words */
	rotlw	%r9,%r9,%r0	/* shift distance */
	cmpwi	%r8,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	rotlw	%r5,%r5,%r0	/* shift src offset */
	rotlw	%r7,%r7,%r0	/* shift dest offset */
	add	%r10,%r4,%r5	/* calculate src end address */
	add	%r11,%r6,%r7	/* calculate dest end address */
	mtctr	%r8
	cmpw	%r10,%r11	/* compare end address */
	blt	2f		/* jump if h + o < h2 + o2 */

				/* h + o >= h2 + o2 */
1:	lwzx	%r12,%r4,%r5	/* load value */
	stwx	%r12,%r6,%r7	/* store value */
	add	%r5,%r5,%r9	/* src offset += 4 */
	add	%r7,%r7,%r9	/* dest offset += 4 */
	bdnz+	1b		/* jump if len != 0 */
	b	.Lbscr4_end	/* end */

				/* h + o < h2 + o2 */
2:	addi	%r8,%r8,-1	/* len -= 1 */
	rotlw	%r8,%r8,%r0	/* shift len */
	add	%r5,%r10,%r8	/* src offset = o + len - 1 */
	add	%r7,%r11,%r8	/* dest offset = o2 + len - 1 */
3:	lwzx	%r12,%r4,%r5	/* load value */
	stwx	%r12,%r6,%r7	/* store value */
	sub	%r5,%r5,%r9	/* src offset -= 4 */
	sub	%r7,%r7,%r9	/* dest offset -= 4 */
	bdnz+	3b		/* jump if len != 0 */
.Lbscr4_end:
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bscr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY(bscr8_s)
ENTRY(bscr8)
	trap

