/*	$NetBSD: pio_subr.S,v 1.8.2.2 2004/03/29 01:15:15 simonb Exp $	*/

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
/* LINTSTUB: include <sys/param.h> */
/* LINTSTUB: include <sys/types.h> */
/* LINTSTUB: include <machine/bus.h> */

/* LINTSTUB: Func: void out8(volatile u_int8_t *a, u_int8_t v) */

ENTRY_NOPROFILE(out8)
	stbx	4,0,3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v) */
/* LINTSTUB: Func: void bsw1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v) */

ENTRY_NOPROFILE(bsw1_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw1)
	stbx	6,4,5		/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out16(volatile u_int16_t *a, u_int16_t v) */

ENTRY_NOPROFILE(out16)
	sth	4,0(3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */
/* LINTSTUB: Func: void bsw2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */

ENTRY_NOPROFILE(bsw2_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw2)
	sthx	6,4,5		/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out32(volatile u_int32_t *a, u_int32_t v) */

ENTRY_NOPROFILE(out32)
	stw	4,0(3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */
/* LINTSTUB: Func: void bsw4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */

ENTRY_NOPROFILE(bsw4_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw4)
	stwx	6,4,5		/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */
/* LINTSTUB: Func: void bsw8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */
ENTRY_NOPROFILE(bsw8_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw8)
#ifdef __ARCH64__
	stdx	6,4,5		/* store value */
#else
	trap			/* die */
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out16rb(volatile u_int16_t *a, u_int16_t v) */

ENTRY_NOPROFILE(out16rb)
	sthbrx	4,0,3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */
/* LINTSTUB: Func: void bsw2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t v) */


ENTRY_NOPROFILE(bsw2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw2rb)
	sthbrx	6,4,5		/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void out32rb(volatile u_int32_t *a, u_int32_t v) */

ENTRY_NOPROFILE(out32rb)
	stwbrx	4,0,3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */
/* LINTSTUB: Func: void bsw4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t v) */

ENTRY_NOPROFILE(bsw4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw4rb)
	stwbrx	6,4,5		/* store value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsw8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */
/* LINTSTUB: Func: void bsw8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t v) */

ENTRY_NOPROFILE(bsw8rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsw8rb)
#ifdef __ARCH64__
	stdbrx	6,4,5		/* store value */
#else
	trap			/* die */
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in8(const volatile u_int8_t *a) */

ENTRY_NOPROFILE(in8)
	lbz	3,0(3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr1_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr1)
	lbzx	3,4,5		/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in16(const volatile u_int16_t *a) */

ENTRY_NOPROFILE(in16)
	lhz	3,0(3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr2_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr2)
	lhzx	3,4,5		/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in32(const volatile u_int32_t *a) */

ENTRY_NOPROFILE(in32)
	lwz	3,0(3)
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr4_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr4)
	lwzx	3,4,5		/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: u_int64_t bsr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: u_int64_t bsr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr8_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr8)
#ifdef __ARCH64__
	lwdx	3,4,5		/* load value */
#else
	trap
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in16rb(const volatile u_int16_t *a) */

ENTRY_NOPROFILE(in16rb)
	lhbrx	3,0,3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr2rb)
	lhbrx	3,4,5		/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int in32rb(const volatile u_int32_t *a) */

ENTRY_NOPROFILE(in32rb)
	lwbrx	3,0,3
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: int bsr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: int bsr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr4rb)
	lwbrx	3,4,5		/* load value */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: u_int64_t bsr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */
/* LINTSTUB: Func: u_int64_t bsr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o) */

ENTRY_NOPROFILE(bsr8rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsr8rb)
#ifdef __ARCH64__
	ldbrx	3,4,5		/* load value */
#else
	trap
#endif
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs8(volatile u_int8_t *dst, const u_int8_t *src, size_t len) */

ENTRY_NOPROFILE(bswm1_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bswm1)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(outs8)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	add	5,5,4		/* len += src */
	addi	4,4,-1		/* pre-decrement */
1:	lbzu	0,1(4)		/* load and increment */
	stb	0,0(3)		/* store */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs16(volatile u_int16_t *dst, const u_int16_t *src, size_t len) */

ENTRY_NOPROFILE(bswm2_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bswm2)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(outs16)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,1		/* len *= 2 */
	add	5,5,4		/* len += src */
	addi	4,4,-2		/* pre-decrement */
1:	lhzu	0,2(4)		/* load and increment */
	sth	0,0(3)		/* store */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs32(volatile u_int32_t *dst, const u_int32_t *src, size_t len) */

ENTRY_NOPROFILE(bswm4_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bswm4)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(outs32)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,2		/* len *= 4 */
	add	5,5,4		/* len += src */
	addi	4,4,-4		/* pre-decrement */
1:	lwzu	0,4(4)		/* load and increment */
	stw	0,0(3)		/* store */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */

#ifdef _LP64
ENTRY_NOPROFILE(bswm8_s)
	ld	0,0(3)		/* get log2(stride) */
	rotld	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bswm8)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(outs32)
	cmpdi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	sldi	5,5,2		/* len *= 4 */
	add	5,5,4		/* len += src */
	addi	4,4,-4		/* pre-decrement */
1:	ldzu	0,4(4)		/* load and increment */
	std	0,0(3)		/* store */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */
#else
ENTRY_NOPROFILE(bswm8_s)
ENTRY_NOPROFILE(bswm8)
	trap			/* die */
#endif

/* LINTSTUB: Func: void bswm2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs16rb(volatile u_int16_t *dst, const u_int16_t *src, size_t len) */

ENTRY_NOPROFILE(bswm2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bswm2rb)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(outs16rb)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,1		/* len *= 2 */
	add	5,5,4		/* len += src */
	addi	4,4,-2		/* pre-decrement */
1:	lwzu	0,2(4)		/* load and increment */
	sthbrx	0,0,3		/* store (byte-reversed) */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void outs32rb(volatile u_int32_t *dst, const u_int32_t *src, size_t len) */

ENTRY_NOPROFILE(bswm4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bswm4rb)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(outs32rb)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,2		/* len *= 4 */
	add	5,5,4		/* len += src */
	addi	4,4,-4		/* pre-decrement */
1:	lwzu	0,4(4)		/* load and increment */
	stwbrx	0,0,3		/* store (byte-reversed) */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswm8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bswm8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *addr, bus_size_t len) */
ENTRY_NOPROFILE(bswm8rb_s)
ENTRY_NOPROFILE(bswm8rb)
	trap

/* LINTSTUB: Func: void bsrm1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins8(const volatile u_int8_t *src, u_int8_t *dst, size_t len) */

ENTRY_NOPROFILE(bsrm1_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsrm1)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(ins8)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	add	5,5,4		/* len += src */
	addi	4,4,-1		/* pre-decrement */
1:	lbz	0,0(3)		/* load value */
	stbu	0,1(4)		/* store and increment */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins16(const volatile u_int16_t *src, u_int16_t *dst, size_t len) */

ENTRY_NOPROFILE(bsrm2_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsrm2)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(ins16)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,1		/* len *= 2 */
	add	5,5,4		/* len += src */
	addi	4,4,-2		/* pre-decrement */
1:	lhz	0,0(3)		/* load value */
	sthu	0,2(4)		/* store and increment */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins32(const volatile u_int32_t *src, u_int32_t *dst, size_t len) */

ENTRY_NOPROFILE(bsrm4_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsrm4)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(ins32)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,2		/* len *= 4 */
	add	5,5,4		/* len += src */
	addi	4,4,-4		/* pre-decrement */
1:	lwz	0,0(3)		/* load value */
	stwu	0,4(4)		/* store and increment */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
ENTRY_NOPROFILE(bsrm8_s)
ENTRY_NOPROFILE(bsrm8)
	trap

/* LINTSTUB: Func: void bsrm2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins16rb(const volatile u_int16_t *src, u_int16_t *dst, size_t len) */

ENTRY_NOPROFILE(bsrm2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsrm2rb)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(ins16rb)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,1		/* len *= 2 */
	add	5,5,4		/* len += src */
	addi	4,4,-2		/* pre-decrement */
1:	lhbrx	0,0,3		/* load value (byte reversed) */
	sthu	0,2(4)		/* store and increment */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void ins32rb(const volatile u_int32_t *src, u_int32_t *dst, size_t len) */
ENTRY_NOPROFILE(bsrm4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	rotlw	5,5,0		/* shift offset */
ENTRY_NOPROFILE(bsrm4rb)
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	mr	5,7		/* move count to argument 2 register */
ENTRY_NOPROFILE(ins32rb)
	cmpwi	5,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	5,5,-1		/* len -= 1 */
	slwi	5,5,2		/* len *= 4 */
	add	5,5,4		/* len += src */
	addi	4,4,-4		/* pre-decrement */
1:	lwbrx	0,0,3		/* load value (byte reversed) */
	stwu	0,4(4)		/* store and increment */
	cmpl	0,4,5		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrm8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
/* LINTSTUB: Func: void bsrm8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *addr, bus_size_t len) */
ENTRY_NOPROFILE(bsrm8rb_s)
ENTRY_NOPROFILE(bsrm8rb)
	trap

/* LINTSTUB: Func: void bswr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bswr1_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,1		/* distance between dst bytes */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbswr1_enter
ENTRY_NOPROFILE(bswr1)
	li	8,1		/* distance between dst bytes */
.Lbswr1_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	add	7,7,6		/* len += src */
	addi	6,6,-1		/* pre-decrement */
	sub	3,3,8
1:	lbzu	0,1(6)		/* load and increment */
	stbux	0,3,8		/* store and add distance */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bswr2_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,2		/* distance between dst halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbswr2_enter
ENTRY_NOPROFILE(bswr2)
	li	8,2		/* distance between dst halfwords */
.Lbswr2_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,1		/* len *= 2 */
	add	7,7,6		/* len += src */
	addi	6,6,-2		/* pre-decrement */
	sub	3,3,8
1:	lhzu	0,2(6)		/* load and increment */
	sthux	0,3,8		/* store and add distance */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bswr2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,2		/* distance between dst halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbswr2rb_enter
ENTRY_NOPROFILE(bswr2rb)
	li	8,2		/* distance between dst halfwords */
.Lbswr2rb_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,1		/* len *= 2 */
	add	7,7,6		/* len += src */
	addi	6,6,-2		/* pre-decrement */
1:	lhzu	0,2(6)		/* load and increment */
	sthbrx	0,0,3		/* store (reversed)
	add	3,3,8		/* dst += distance */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bswr4_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,4		/* distance between dst halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbswr4_enter
ENTRY_NOPROFILE(bswr4)
	li	8,4		/* distance between dst halfwords */
.Lbswr4_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,2		/* len *= 4 */
	add	7,7,6		/* len += src */
	addi	6,6,-4		/* pre-decrement */
	sub	3,3,8
1:	lwzu	0,4(6)		/* load and increment */
	stwux	0,3,8		/* store and add distance */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bswr4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,4		/* distance between dst halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbswr4rb_enter
ENTRY_NOPROFILE(bswr4rb)
	li	8,4		/* distance between dst halfwords */
.Lbswr4rb_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,2		/* len *= 4 */
	add	7,7,6		/* len += src */
	addi	6,6,-4		/* pre-decrement */
1:	lwzu	0,4(6)		/* load and increment */
	stwbrx	0,0,3		/* store (reversed)
	add	3,3,8		/* dst += distance */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bswr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bswr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bswr8_s)
ENTRY_NOPROFILE(bswr8rb_s)
	lwz	0,0(3)
	rotlw	5,5,0
ENTRY_NOPROFILE(bswr8)
ENTRY_NOPROFILE(bswr8rb)
	trap

/* LINTSTUB: Func: void bsrr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bsrr1_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,1		/* distance between src bytes */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbsrr1_enter
ENTRY_NOPROFILE(bsrr1)
	li	8,1		/* distance between src bytes */
.Lbsrr1_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	add	7,7,6		/* len += src */
	addi	6,6,-1		/* pre-decrement */
	sub	3,3,8
1:	lbzux	0,3,8		/* load value and add distance */
	stbu	0,1(6)		/* store and increment */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bsrr2_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,2		/* distance between src halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbsrr2_enter
ENTRY_NOPROFILE(bsrr2)
	li	8,2		/* distance between src halfwords */
.Lbsrr2_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,1		/* len *= 2 */
	add	7,7,6		/* len += src */
	addi	6,6,-2		/* pre-decrement */
	sub	3,3,8
1:	lhzux	0,3,8		/* load value and add distance */
	sthu	0,2(6)		/* store and increment */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bsrr2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,2		/* distance between source halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbsrr2rb_enter
ENTRY_NOPROFILE(bsrr2rb)
	li	8,2		/* distance between source halfwords */
.Lbsrr2rb_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,1		/* len *= 2 */
	add	7,7,6		/* len += src */
	addi	6,6,-2		/* pre-decrement */
1:	lhbrx	0,0,3		/* load value (reversed) */
	add	3,3,8		/* src += distance */
	sthu	0,2(6)		/* store and increment */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bsrr4_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,4		/* distance between src words */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbsrr4_enter
ENTRY_NOPROFILE(bsrr4)
	li	8,4		/* distance between src words */
.Lbsrr4_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,2		/* len *= 4 */
	add	7,7,6		/* len += src */
	addi	6,6,-4		/* pre-decrement */
	sub	3,3,8
1:	lwzux	0,3,8		/* load value and add distance */
	stwu	0,4(6)		/* store and increment */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bsrr4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,4		/* distance between src words */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbsrr4rb_enter
ENTRY_NOPROFILE(bsrr4rb)
	li	8,4		/* distance between src words */
.Lbsrr4rb_enter:
	add	3,4,5		/* add offset to handle & place in argument 0 */
	mr	4,6		/* move addr to argument 1 register */
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	addi	7,7,-1		/* len -= 1 */
	slwi	7,7,2		/* len *= 4 */
	add	7,7,6		/* len += src */
	addi	6,6,-4		/* pre-decrement */
1:	lwbrx	0,0,3		/* load value (reversed) */
	add	3,3,8		/* src += distance */
	sthu	0,4(6)		/* store and increment */
	cmpl	0,6,7		/* at the end? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bsrr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
/* LINTSTUB: Func: void bsrr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c); */
ENTRY_NOPROFILE(bsrr8_s)
ENTRY_NOPROFILE(bsrr8rb_s)
	lwz	0,0(3)
	rotlw	5,5,0
ENTRY_NOPROFILE(bsrr8)
ENTRY_NOPROFILE(bsrr8rb)
	trap

/* LINTSTUB: Func: void bssr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v, bus_size_t c); */
ENTRY_NOPROFILE(bssr1_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,1		/* distance between src bytes */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbssr1_enter
ENTRY_NOPROFILE(bssr1)
	li	8,1		/* distance between src bytes */
.Lbssr1_enter:
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	7,7,-1		/* len -= 1 */
	stbx	6,4,5		/* store value */
	add	5,5,8		/* add offset */
	cmpwi	7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
ENTRY_NOPROFILE(bssr2_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,2		/* distance between src halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbssr2_enter
ENTRY_NOPROFILE(bssr2)
	li	8,2		/* distance between src halfwords */
.Lbssr2_enter:
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	7,7,-1		/* len -= 1 */
	sthx	6,4,5		/* store value */
	add	5,5,8		/* add offset */
	cmpwi	7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr2rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr2rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int16_t v, bus_size_t c); */
ENTRY_NOPROFILE(bssr2rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,2		/* distance between src halfwords */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbssr2rb_enter
ENTRY_NOPROFILE(bssr2rb)
	li	8,2		/* distance between src halfwords */
.Lbssr2rb_enter:
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	7,7,-1		/* len -= 1 */
	sthbrx	6,4,5		/* store value */
	add	5,5,8		/* add offset */
	cmpwi	7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
ENTRY_NOPROFILE(bssr4_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,4		/* distance between src words */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbssr4_enter
ENTRY_NOPROFILE(bssr4)
	li	8,4		/* distance between src words */
.Lbssr4_enter:
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	7,7,-1		/* len -= 1 */
	stwx	6,4,5		/* store value */
	add	5,5,8		/* add offset */
	cmpwi	7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr4rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr4rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int32_t v, bus_size_t c); */
ENTRY_NOPROFILE(bssr4rb_s)
	lwz	0,0(3)		/* get log2(stride) */
	li	8,4		/* distance between src words */
	rotlw	5,5,0		/* shift offset */
	rotlw	8,8,0		/* shift distance */
	b	.Lbssr4rb_enter
ENTRY_NOPROFILE(bssr4rb)
	li	8,4		/* distance between src words */
.Lbssr4rb_enter:
	cmpwi	7,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
1:	addi	7,7,-1		/* len -= 1 */
	stwbrx	6,4,5		/* store value */
	add	5,5,8		/* add offset */
	cmpwi	7,0		/* len == 0? */
	bne+	1b		/*   nope, do another pass */
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bssr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr8rb_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
/* LINTSTUB: Func: void bssr8rb(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, const u_int64_t v, bus_size_t c); */
ENTRY_NOPROFILE(bssr8_s)
ENTRY_NOPROFILE(bssr8rb_s)
	lwz	0,0(3)
	rotlw	5,5,0
ENTRY_NOPROFILE(bssr8)
ENTRY_NOPROFILE(bssr8rb)
	trap

/* LINTSTUB: Func: void bscr1_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY_NOPROFILE(bscr1_s)
	lwz	0,0(3)		/* get log2(stride) */
	b	.Lbscr1_enter
ENTRY_NOPROFILE(bscr1)
	li	0,0		/* non stride */
.Lbscr1_enter:
	li	9,1		/* distance between src bytes */
	rotlw	9,9,0		/* shift distance */
	cmpwi	8,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	rotlw	5,5,0		/* shift src offset */
	rotlw	7,7,0		/* shift dest offset */
	add	10,4,5		/* calculate src end address */
	add	11,6,7		/* calculate dest end address */
	mtctr	8
	cmpw	10,11		/* compare end address */
	blt	2f		/* jump if h + o < h2 + o2 */

				/* h + o >= h2 + o2 */
1:	lbzx	12,4,5		/* load value */
	stbx	12,6,7		/* store value */
	add	5,5,9		/* src offset += 1 */
	add	7,7,9		/* dest offset += 1 */
	bdnz+	1b		/* jump if len != 0 */
	b	.Lbscr1_end	/* end */

				/* h + o < h2 + o2 */
2:	addi	8,8,-1		/* len -= 1 */
	rotlw	8,8,0		/* shift len */
	add	5,10,8		/* src offset = o + len - 1 */
	add	7,11,8		/* dest offset = o2 + len - 1 */
3:	lbzx	12,4,5		/* load value */
	stbx	12,6,7		/* store value */
	sub	5,5,9		/* src offset -= 1 */
	sub	7,7,9		/* dest offset -= 1 */
	bdnz+	3b		/* jump if len != 0 */
.Lbscr1_end:
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bscr2_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY_NOPROFILE(bscr2_s)
	lwz	0,0(3)		/* get log2(stride) */
	b	.Lbscr2_enter
ENTRY_NOPROFILE(bscr2)
	li	0,0		/* non stride */
.Lbscr2_enter:
	li	9,2		/* distance between src halfwords */
	rotlw	9,9,0		/* shift distance */
	cmpwi	8,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	rotlw	5,5,0		/* shift src offset */
	rotlw	7,7,0		/* shift dest offset */
	add	10,4,5		/* calculate src end address */
	add	11,6,7		/* calculate dest end address */
	mtctr	8
	cmpw	10,11		/* compare end address */
	blt	2f		/* jump if h + o < h2 + o2 */

				/* h + o >= h2 + o2 */
1:	lhzx	12,4,5		/* load value */
	sthx	12,6,7		/* store value */
	add	5,5,9		/* src offset += 2 */
	add	7,7,9		/* dest offset += 2 */
	bdnz+	1b		/* jump if len != 0 */
	b	.Lbscr2_end	/* end */

				/* h + o < h2 + o2 */
2:	addi	8,8,-1		/* len -= 1 */
	rotlw	8,8,0		/* shift len */
	add	5,10,8		/* src offset = o + len - 1 */
	add	7,11,8		/* dest offset = o2 + len - 1 */
3:	lhzx	12,4,5		/* load value */
	sthx	12,6,7		/* store value */
	sub	5,5,9		/* src offset -= 2 */
	sub	7,7,9		/* dest offset -= 2 */
	bdnz+	3b		/* jump if len != 0 */
.Lbscr2_end:
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bscr4_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY_NOPROFILE(bscr4_s)
	lwz	0,0(3)		/* get log2(stride) */
	b	.Lbscr4_enter
ENTRY_NOPROFILE(bscr4)
	li	0,0		/* non stride */
.Lbscr4_enter:
	li	9,4		/* distance between src words */
	rotlw	9,9,0		/* shift distance */
	cmpwi	8,0		/* len == 0? */
	beqlr-			/*   return if len == 0 */
	rotlw	5,5,0		/* shift src offset */
	rotlw	7,7,0		/* shift dest offset */
	add	10,4,5		/* calculate src end address */
	add	11,6,7		/* calculate dest end address */
	mtctr	8
	cmpw	10,11		/* compare end address */
	blt	2f		/* jump if h + o < h2 + o2 */

				/* h + o >= h2 + o2 */
1:	lwzx	12,4,5		/* load value */
	stwx	12,6,7		/* store value */
	add	5,5,9		/* src offset += 4 */
	add	7,7,9		/* dest offset += 4 */
	bdnz+	1b		/* jump if len != 0 */
	b	.Lbscr4_end	/* end */

				/* h + o < h2 + o2 */
2:	addi	8,8,-1		/* len -= 1 */
	rotlw	8,8,0		/* shift len */
	add	5,10,8		/* src offset = o + len - 1 */
	add	7,11,8		/* dest offset = o2 + len - 1 */
3:	lwzx	12,4,5		/* load value */
	stwx	12,6,7		/* store value */
	sub	5,5,9		/* src offset -= 4 */
	sub	7,7,9		/* dest offset -= 4 */
	bdnz+	3b		/* jump if len != 0 */
.Lbscr4_end:
	eieio			/* memory barrier (reorder protection) */
	DBGSYNC			/* force exceptions */
	blr			/* return */

/* LINTSTUB: Func: void bscr8_s(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
/* LINTSTUB: Func: void bscr8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, bus_space_handle_t h2, bus_size_t o2, bus_size_t len); */
ENTRY_NOPROFILE(bscr8_s)
ENTRY_NOPROFILE(bscr8)
	trap

