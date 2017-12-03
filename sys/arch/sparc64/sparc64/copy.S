/*	$NetBSD: copy.S,v 1.4.22.1 2017/12/03 11:36:45 jdolecek Exp $	*/

/*
 * Copyright (c) 2006-2010 Matthew R. Green
 * Copyright (c) 1996-2002 Eduardo Horvath
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College.
 *	All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.
 *	All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by Paul Kranenburg.
 * 4. Neither the name of the University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *	@(#)locore.s	8.4 (Berkeley) 12/10/93
 */


#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/param.h>
#include <machine/ctlreg.h>
#include <machine/asm.h>
#include <machine/locore.h>
#include <machine/psl.h>

#include "ksyms.h"

	.register	%g2,#scratch
	.register	%g3,#scratch

/*
 * copyinstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 */
ENTRY(copyinstr)
	! %o0 = fromaddr, %o1 = toaddr, %o2 = maxlen, %o3 = &lencopied
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	8f, %o0
	mov	%i0, %o1
	mov	%i1, %o2
	mov	%i2, %o3
	call	printf
	 mov	%i3, %o4
	restore
	.data
8:	.asciz	"copyinstr: from=%x to=%x max=%x &len=%x\n"
	_ALIGN
	.text
#endif
	brgz,pt	%o2, 1f					! Make sure len is valid
	 sethi	%hi(CPCB), %o4		! (first instr of copy)
	retl
	 mov	ENAMETOOLONG, %o0
1:
	LDPTR	[%o4 + %lo(CPCB)], %o4	! catch faults
	set	Lcsdone, %o5
	membar	#Sync
	STPTR	%o5, [%o4 + PCB_ONFAULT]

	mov	%o1, %o5		!	save = toaddr;
! XXX should do this in bigger chunks when possible
0:					! loop:
	ldsba	[%o0] ASI_AIUS, %g1	!	c = *fromaddr;
	stb	%g1, [%o1]		!	*toaddr++ = c;
	inc	%o1
	brz,a,pn	%g1, Lcsdone	!	if (c == NULL)
	 clr	%o0			!		{ error = 0; done; }
	deccc	%o2			!	if (--len > 0) {
	bg,pt	%icc, 0b		!		fromaddr++;
	 inc	%o0			!		goto loop;
	ba,pt	%xcc, Lcsdone		!	}
	 mov	ENAMETOOLONG, %o0	!	error = ENAMETOOLONG;
	NOTREACHED

/*
 * copyoutstr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from the kernel
 * address space to the user address space.
 */
ENTRY(copyoutstr)
	! %o0 = fromaddr, %o1 = toaddr, %o2 = maxlen, %o3 = &lencopied
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	8f, %o0
	mov	%i0, %o1
	mov	%i1, %o2
	mov	%i2, %o3
	call	printf
	 mov	%i3, %o4
	restore
	.data
8:	.asciz	"copyoutstr: from=%x to=%x max=%x &len=%x\n"
	_ALIGN
	.text
#endif
	brgz,pt	%o2, 1f					! Make sure len is valid
	 sethi	%hi(CPCB), %o4		! (first instr of copy)
	retl
	 mov	ENAMETOOLONG, %o0
1:
	LDPTR	[%o4 + %lo(CPCB)], %o4	! catch faults
	set	Lcsdone, %o5
	membar	#Sync
	STPTR	%o5, [%o4 + PCB_ONFAULT]

	mov	%o1, %o5		!	save = toaddr;
! XXX should do this in bigger chunks when possible
0:					! loop:
	ldsb	[%o0], %g1		!	c = *fromaddr;
	stba	%g1, [%o1] ASI_AIUS	!	*toaddr++ = c;
	inc	%o1
	brz,a,pn	%g1, Lcsdone	!	if (c == NULL)
	 clr	%o0			!		{ error = 0; done; }
	deccc	%o2			!	if (--len > 0) {
	bg,pt	%icc, 0b		!		fromaddr++;
	 inc	%o0			!		goto loop;
					!	}
	mov	ENAMETOOLONG, %o0	!	error = ENAMETOOLONG;
Lcsdone:				! done:
	sub	%o1, %o5, %o1		!	len = to - save;
	brnz,a	%o3, 1f			!	if (lencopied)
	 STPTR	%o1, [%o3]		!		*lencopied = len;
1:
	retl				! cpcb->pcb_onfault = 0;
	 STPTR	%g0, [%o4 + PCB_ONFAULT]! return (error);


/*
 * copyin(src, dst, len)
 *
 * Copy specified amount of data from user space into the kernel.
 *
 * This is a modified version of memcpy that uses ASI_AIUS.  When
 * memcpy is optimized to use block copy ASIs, this should be also.
 */

ENTRY(copyin)
!	flushw			! Make sure we don't have stack probs & lose hibits of %o
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i0, %o1
	mov	%i1, %o2
	call	printf
	 mov	%i2, %o3
	restore
	.data
1:	.asciz	"copyin: src=%x dest=%x len=%x\n"
	_ALIGN
	.text
#endif
	sethi	%hi(CPCB), %o3
	wr	%g0, ASI_AIUS, %asi
	LDPTR	[%o3 + %lo(CPCB)], %o3
	set	Lcopyfault, %o4
!	mov	%o7, %g7		! save return address
	membar	#Sync
	STPTR	%o4, [%o3 + PCB_ONFAULT]
	cmp	%o2, BCOPY_SMALL
Lcopyin_start:
	bge,a	Lcopyin_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
0:
	 inc	%o0
	ldsba	[%o0 - 1] %asi, %o4!	*dst++ = (++src)[-1];
	stb	%o4, [%o1]
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	ba	Lcopyin_done
	 clr	%o0
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lcopyin_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 .empty
	btst	7, %o1
	be,a	Lcopyin_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto copyin_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsba	[%o0] %asi, %o4	!	do {
	inc	%o0		!		(++dst)[-1] = *src++;
	inc	%o1
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 stb	%o4, [%o1 - 1]
	ba	Lcopyin_done
	 clr	%o0
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsba	[%o0] %asi, %o4	!	*dst++ = *src++;
	stb	%o4, [%o1]
	inc	%o0
	inc	%o1
	dec	%o2		!	len--;
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsha	[%o0] %asi, %o4	!	do {
	sth	%o4, [%o1]	!		*(short *)dst = *(short *)src;
	inc	2, %o0		!		dst += 2, src += 2;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lcopyin_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsha	[%o0] %asi, %o4	!	(*short *)dst = *(short *)src;
	sth	%o4, [%o1]
	inc	2, %o0		!	dst += 2;
	inc	2, %o1		!	src += 2;
	dec	2, %o2		!	len -= 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	lduwa	[%o0] %asi, %o4	!	do {
	st	%o4, [%o1]	!		*(int *)dst = *(int *)src;
	inc	4, %o0		!		dst += 4, src += 4;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lcopyin_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	lduwa	[%o0] %asi, %o4	!	*(int *)dst = *(int *)src;
	st	%o4, [%o1]
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	inc	4, %o1
	dec	4, %o2		! }
1:
Lcopyin_doubles:
	ldxa	[%o0] %asi, %g1	! do {
	stx	%g1, [%o1]	!	*(double *)dst = *(double *)src;
	inc	8, %o0		!	dst += 8, src += 8;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lcopyin_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lcopyin_done	!	goto copyin_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lcopyin_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	lduwa	[%o0] %asi, %o4	!	*(int *)dst = *(int *)src;
	st	%o4, [%o1]
	inc	4, %o0		!	dst += 4;
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lcopyin_mopw:
	be	Lcopyin_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsha	[%o0] %asi, %o4	! *(short *)dst = *(short *)src;
	be	Lcopyin_done	! if ((len & 1) == 0) goto done;
	 sth	%o4, [%o1]
	ldsba	[%o0 + 2] %asi, %o4	! dst[2] = src[2];
	stb	%o4, [%o1 + 2]
	ba	Lcopyin_done
	 clr	%o0
	NOTREACHED

	! mop up trailing byte (if present).
Lcopyin_mopb:
	be,a	Lcopyin_done
	 nop
	ldsba	[%o0] %asi, %o4
	stb	%o4, [%o1]

Lcopyin_done:
	sethi	%hi(CPCB), %o3
!	stb	%o4,[%o1]	! Store last byte -- should not be needed
	LDPTR	[%o3 + %lo(CPCB)], %o3
	membar	#Sync
	STPTR	%g0, [%o3 + PCB_ONFAULT]
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore ASI
	retl
	 clr	%o0			! return 0

/*
 * copyout(src, dst, len)
 *
 * Copy specified amount of data from kernel to user space.
 * Just like copyin, except that the `dst' addresses are user space
 * rather than the `src' addresses.
 *
 * This is a modified version of memcpy that uses ASI_AIUS.  When
 * memcpy is optimized to use block copy ASIs, this should be also.
 */
 /*
  * This needs to be reimplemented to really do the copy.
  */
ENTRY(copyout)
	/*
	 * ******NOTE****** this depends on memcpy() not using %g7
	 */
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	mov	%i0, %o1
	set	CTX_SECONDARY, %o4
	mov	%i1, %o2
	ldxa	[%o4] ASI_DMMU, %o4
	call	printf
	 mov	%i2, %o3
	restore
	.data
1:	.asciz	"copyout: src=%x dest=%x len=%x ctx=%d\n"
	_ALIGN
	.text
#endif
Ldocopy:
	sethi	%hi(CPCB), %o3
	wr	%g0, ASI_AIUS, %asi
	LDPTR	[%o3 + %lo(CPCB)], %o3
	set	Lcopyfault, %o4
!	mov	%o7, %g7		! save return address
	membar	#Sync
	STPTR	%o4, [%o3 + PCB_ONFAULT]
	cmp	%o2, BCOPY_SMALL
Lcopyout_start:
	membar	#StoreStore
	bge,a	Lcopyout_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 .empty
0:
	inc	%o0
	ldsb	[%o0 - 1], %o4	! (++dst)[-1] = *src++;
	stba	%o4, [%o1] %asi
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	ba	Lcopyout_done
	 clr	%o0
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lcopyout_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 .empty
	btst	7, %o1
	be,a	Lcopyout_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto copyout_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsb	[%o0], %o4	!	do {
	inc	%o0		!		(++dst)[-1] = *src++;
	inc	%o1
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 stba	%o4, [%o1 - 1] %asi
	ba	Lcopyout_done
	 clr	%o0
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	stba	%o4, [%o1] %asi
	inc	%o0
	inc	%o1
	dec	%o2		!	len--;
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsh	[%o0], %o4	!	do {
	stha	%o4, [%o1] %asi	!		*(short *)dst = *(short *)src;
	inc	2, %o0		!		dst += 2, src += 2;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lcopyout_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsh	[%o0], %o4	!	(*short *)dst = *(short *)src;
	stha	%o4, [%o1] %asi
	inc	2, %o0		!	dst += 2;
	inc	2, %o1		!	src += 2;
	dec	2, %o2		!	len -= 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	lduw	[%o0], %o4	!	do {
	sta	%o4, [%o1] %asi	!		*(int *)dst = *(int *)src;
	inc	4, %o0		!		dst += 4, src += 4;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lcopyout_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	lduw	[%o0], %o4	!	*(int *)dst = *(int *)src;
	sta	%o4, [%o1] %asi
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	inc	4, %o1
	dec	4, %o2		! }
1:
Lcopyout_doubles:
	ldx	[%o0], %g1	! do {
	stxa	%g1, [%o1] %asi	!	*(double *)dst = *(double *)src;
	inc	8, %o0		!	dst += 8, src += 8;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lcopyout_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lcopyout_done	!	goto copyout_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lcopyout_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	lduw	[%o0], %o4	!	*(int *)dst = *(int *)src;
	sta	%o4, [%o1] %asi
	inc	4, %o0		!	dst += 4;
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lcopyout_mopw:
	be	Lcopyout_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsh	[%o0], %o4	! *(short *)dst = *(short *)src;
	be	Lcopyout_done	! if ((len & 1) == 0) goto done;
	 stha	%o4, [%o1] %asi
	ldsb	[%o0 + 2], %o4	! dst[2] = src[2];
	stba	%o4, [%o1 + 2] %asi
	ba	Lcopyout_done
	 clr	%o0
	NOTREACHED

	! mop up trailing byte (if present).
Lcopyout_mopb:
	be,a	Lcopyout_done
	 nop
	ldsb	[%o0], %o4
	stba	%o4, [%o1] %asi

Lcopyout_done:
	sethi	%hi(CPCB), %o3
	LDPTR	[%o3 + %lo(CPCB)], %o3
	membar	#Sync
	STPTR	%g0, [%o3 + PCB_ONFAULT]
!	jmp	%g7 + 8		! Original instr
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore ASI
	membar	#StoreStore|#StoreLoad
	retl			! New instr
	 clr	%o0			! return 0

! Copyin or copyout fault.  Clear cpcb->pcb_onfault and return error.
! Note that although we were in memcpy, there is no state to clean up;
! the only special thing is that we have to return to [g7 + 8] rather than
! [o7 + 8].
Lcopyfault:
	sethi	%hi(CPCB), %o3
	LDPTR	[%o3 + %lo(CPCB)], %o3
	STPTR	%g0, [%o3 + PCB_ONFAULT]
	membar	#StoreStore|#StoreLoad
#ifdef NOTDEF_DEBUG
	save	%sp, -CC64FSZ, %sp
	set	1f, %o0
	call	printf
	 nop
	restore
	.data
1:	.asciz	"copyfault: fault occurred\n"
	_ALIGN
	.text
#endif
	retl
	 wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore ASI


STRONG_ALIAS(ucas_int,ucas_32)
#ifdef _LP64
STRONG_ALIAS(ucas_ptr,ucas_64)
#else
STRONG_ALIAS(ucas_ptr,ucas_32)
#endif

/*
 * Compare-and-swap the pointer in the user-space.
 *
 * int	ucas_32(volatile int32_t *uptr, int32_t old, int32_t new, int32_t *ret);
 */
ENTRY(ucas_32)
	sethi	%hi(CPCB), %o4
	wr	%g0, ASI_AIUS, %asi
	LDPTR	[%o4 + %lo(CPCB)], %o4
	set	Lcopyfault, %o5				! reusing copyin/copyout
	membar	#Sync					! fault handler
	STPTR	%o5, [%o4 + PCB_ONFAULT]

	casa	[%o0] %asi, %o1, %o2			! cas, result in %o2

	sethi	%hi(CPCB), %o4
	LDPTR	[%o4 + %lo(CPCB)], %o4
	membar	#Sync
	STPTR	%g0, [%o4 + PCB_ONFAULT]
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore ASI
	membar	#StoreStore|#StoreLoad
	stw	%o2, [%o3]		! store the cas result in *result
	retl
	 clr	%o0			! return 0

/*
 * Compare-and-swap the 64-bit integer in the user-space.
 *
 * int	ucas_64(volatile int64_t *uptr, int64_t old, int64_t new, int64_t *ret);
 */
ENTRY(ucas_64)
#ifndef _LP64
	COMBINE(%o1, %o2, %o1)
	COMBINE(%o3, %o4, %o2)
	mov	%o5, %o3
#endif
	sethi	%hi(CPCB), %o4
	wr	%g0, ASI_AIUS, %asi
	LDPTR	[%o4 + %lo(CPCB)], %o4
	set	Lcopyfault, %o5				! reusing copyin/copyout
	membar	#Sync					! fault handler
	STPTR	%o5, [%o4 + PCB_ONFAULT]

	casxa	[%o0] %asi, %o1, %o2			! cas, result in %o2

	sethi	%hi(CPCB), %o4
	LDPTR	[%o4 + %lo(CPCB)], %o4
	membar	#Sync
	STPTR	%g0, [%o4 + PCB_ONFAULT]
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore ASI
	membar	#StoreStore|#StoreLoad
	stx	%o2, [%o3]		! store the cas result in *result
	retl
	 clr	%o0			! return 0

/*
 * {fu,su}{,i}{byte,word}
 */
Lfserr:
	STPTR	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	membar	#StoreStore|#StoreLoad
Lfsbadaddr:
#ifndef _LP64
	mov	-1, %o1
#endif
	retl				! and return error indicator
	 mov	-1, %o0

	/*
	 * This is just like Lfserr, but it's a global label that allows
	 * mem_access_fault() to check to see that we don't want to try to
	 * page in the fault.  It's used by fuswintr() etc.
	 */
	.globl	_C_LABEL(Lfsbail)
_C_LABEL(Lfsbail):
	STPTR	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	membar	#StoreStore|#StoreLoad
	retl				! and return error indicator
	 mov	-1, %o0

	/*
	 * Like fusword but callable from interrupt context.
	 * Fails if data isn't resident.
	 */
ENTRY(fuswintr)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = _Lfsbail;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	_C_LABEL(Lfsbail), %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	lduha	[%o0] ASI_AIUS, %o0	! fetch the halfword
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault
	retl				! made it
	 membar	#StoreStore|#StoreLoad

ENTRY(fusword)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	lduha	[%o0] ASI_AIUS, %o0		! fetch the halfword
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault
	retl				! made it
	 membar	#StoreStore|#StoreLoad

ALTENTRY(fuibyte)
ENTRY(fubyte)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	lduba	[%o0] ASI_AIUS, %o0	! fetch the byte
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! but first clear onfault
	retl				! made it
	 membar	#StoreStore|#StoreLoad

ENTRY(suswintr)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = _Lfsbail;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	_C_LABEL(Lfsbail), %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	stha	%o1, [%o0] ASI_AIUS	! store the halfword
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	membar	#StoreStore|#StoreLoad
	retl				! and return 0
	 clr	%o0

ENTRY(susword)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	stha	%o1, [%o0] ASI_AIUS	! store the halfword
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	membar	#StoreStore|#StoreLoad
	retl				! and return 0
	 clr	%o0

ENTRY(subyte)
	sethi	%hi(CPCB), %o2		! cpcb->pcb_onfault = Lfserr;
	LDPTR	[%o2 + %lo(CPCB)], %o2
	set	Lfserr, %o3
	STPTR	%o3, [%o2 + PCB_ONFAULT]
	membar	#Sync
	stba	%o1, [%o0] ASI_AIUS	! store the byte
	membar	#Sync
	STPTR	%g0, [%o2 + PCB_ONFAULT]! made it, clear onfault
	membar	#StoreStore|#StoreLoad
	retl				! and return 0
	 clr	%o0

/* probeget and probeset are meant to be used during autoconfiguration */
/*
 * The following probably need to be changed, but to what I don't know.
 */

/*
 * uint64_t
 * probeget(addr, asi, size)
 *	paddr_t addr;
 *	int asi;
 *	int size;
 *
 * Read or write a (byte,word,longword) from the given address.
 * Like {fu,su}{byte,halfword,word} but our caller is supposed
 * to know what he is doing... the address can be anywhere.
 *
 * We optimize for space, rather than time, here.
 */
ENTRY(probeget)
#ifndef _LP64
	!! Shuffle the args around into LP64 format
	COMBINE(%o0, %o1, %o0)
	mov	%o2, %o1
	mov	%o3, %o2
#endif
	mov	%o2, %o4
	! %o0 = addr, %o1 = asi, %o4 = (1,2,4)
	sethi	%hi(CPCB), %o2
	LDPTR	[%o2 + %lo(CPCB)], %o2	! cpcb->pcb_onfault = Lfserr;
#ifdef _LP64
	set	_C_LABEL(Lfsbail), %o5
#else
	set	_C_LABEL(Lfsprobe), %o5
#endif
	STPTR	%o5, [%o2 + PCB_ONFAULT]
	or	%o0, 0x9, %o3		! if (PHYS_ASI(asi)) {
	sub	%o3, 0x1d, %o3
	brz,a	%o3, 0f
	 mov	%g0, %o5
	DLFLUSH(%o0,%o5)		!	flush cache line
					! }
0:
#ifndef _LP64
	rdpr	%pstate, %g1
	wrpr	%g1, PSTATE_AM, %pstate
#endif
	btst	1, %o4
	wr	%o1, 0, %asi
	membar	#Sync
	bz	0f			! if (len & 1)
	 btst	2, %o4
	ba,pt	%icc, 1f
	 lduba	[%o0] %asi, %o0		!	value = *(char *)addr;
0:
	bz	0f			! if (len & 2)
	 btst	4, %o4
	ba,pt	%icc, 1f
	 lduha	[%o0] %asi, %o0		!	value = *(short *)addr;
0:
	bz	0f			! if (len & 4)
	 btst	8, %o4
	ba,pt	%icc, 1f
	 lda	[%o0] %asi, %o0		!	value = *(int *)addr;
0:
	ldxa	[%o0] %asi, %o0		!	value = *(long *)addr;
1:	
#ifndef _LP64
	SPLIT(%o0, %o0, %o1)
#endif
	membar	#Sync
#ifndef _LP64
	wrpr	%g1, 0, %pstate
#endif
	brz	%o5, 1f			! if (cache flush addr != 0)
	 nop
	DLFLUSH2(%o5)			!	flush cache line again
1:
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore default ASI	
	STPTR	%g0, [%o2 + PCB_ONFAULT]
	retl				! made it, clear onfault and return
	 membar	#StoreStore|#StoreLoad

	/*
	 * Fault handler for probeget
	 */
_C_LABEL(Lfsprobe):
#ifndef _LP64
	wrpr	%g1, 0, %pstate
#endif
	STPTR	%g0, [%o2 + PCB_ONFAULT]! error in r/w, clear pcb_onfault
	mov	-1, %o1
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore default ASI	
	membar	#StoreStore|#StoreLoad
	retl				! and return error indicator
	 mov	-1, %o0

/*
 * probeset(addr, asi, size, val)
 *	paddr_t addr;
 *	int asi;
 *	int size;
 *	long val;
 *
 * As above, but we return 0 on success.
 */
ENTRY(probeset)
#ifndef _LP64
	!! Shuffle the args around into LP64 format
	COMBINE(%o0, %o1, %o0)
	mov	%o2, %o1
	mov	%o3, %o2
	COMBINE(%o4, %o5, %o3)
#endif
	mov	%o2, %o4
	! %o0 = addr, %o1 = asi, %o4 = (1,2,4), %o3 = val
	sethi	%hi(CPCB), %o2		! Lfserr requires CPCB in %o2
	LDPTR	[%o2 + %lo(CPCB)], %o2	! cpcb->pcb_onfault = Lfserr;
	set	_C_LABEL(Lfsbail), %o5
	STPTR	%o5, [%o2 + PCB_ONFAULT]
	btst	1, %o4
	wr	%o1, 0, %asi
	membar	#Sync
	bz	0f			! if (len & 1)
	 btst	2, %o4
	ba,pt	%icc, 1f
	 stba	%o3, [%o0] %asi		!	*(char *)addr = value;
0:
	bz	0f			! if (len & 2)
	 btst	4, %o4
	ba,pt	%icc, 1f
	 stha	%o3, [%o0] %asi		!	*(short *)addr = value;
0:
	bz	0f			! if (len & 4)
	 btst	8, %o4
	ba,pt	%icc, 1f
	 sta	%o3, [%o0] %asi		!	*(int *)addr = value;
0:
	bz	Lfserr			! if (len & 8)
	ba,pt	%icc, 1f
	 sta	%o3, [%o0] %asi		!	*(int *)addr = value;
1:	membar	#Sync
	clr	%o0			! made it, clear onfault and return 0
	wr	%g0, ASI_PRIMARY_NOFAULT, %asi		! Restore default ASI	
	STPTR	%g0, [%o2 + PCB_ONFAULT]
	retl
	 membar	#StoreStore|#StoreLoad


/*
 * kcopy() is exactly like bcopy except that it set pcb_onfault such that
 * when a fault occurs, it is able to return -1 to indicate this to the
 * caller.
 */
ENTRY(kcopy)
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	mov	%i0, %o1
	set	2f, %o0
	mov	%i1, %o2
	call	printf
	 mov	%i2, %o3
!	ta	1; nop
	restore
	.data
2:	.asciz	"kcopy(%p->%p,%x)\n"
	_ALIGN
	.text
3:
#endif
	sethi	%hi(CPCB), %o5		! cpcb->pcb_onfault = Lkcerr;
	LDPTR	[%o5 + %lo(CPCB)], %o5
	set	Lkcerr, %o3
	LDPTR	[%o5 + PCB_ONFAULT], %g1! save current onfault handler
	membar	#LoadStore
	STPTR	%o3, [%o5 + PCB_ONFAULT]
	membar	#StoreStore|#StoreLoad

	cmp	%o2, BCOPY_SMALL
Lkcopy_start:
	bge,a	Lkcopy_fancy	! if >= this many, go be fancy.
	 btst	7, %o0		! (part of being fancy)

	/*
	 * Not much to copy, just do it a byte at a time.
	 */
	deccc	%o2		! while (--len >= 0)
	bl	1f
	 .empty
0:
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	inc	%o0
	stb	%o4, [%o1]
	deccc	%o2
	bge	0b
	 inc	%o1
1:
	membar	#Sync		! Make sure all fauls are processed
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	membar	#StoreStore|#StoreLoad
	retl
	 clr	%o0
	NOTREACHED

	/*
	 * Plenty of data to copy, so try to do it optimally.
	 */
Lkcopy_fancy:
	! check for common case first: everything lines up.
!	btst	7, %o0		! done already
	bne	1f
	 .empty
	btst	7, %o1
	be,a	Lkcopy_doubles
	 dec	8, %o2		! if all lined up, len -= 8, goto kcopy_doubes

	! If the low bits match, we can make these line up.
1:
	xor	%o0, %o1, %o3	! t = src ^ dst;
	btst	1, %o3		! if (t & 1) {
	be,a	1f
	 btst	1, %o0		! [delay slot: if (src & 1)]

	! low bits do not match, must copy by bytes.
0:
	ldsb	[%o0], %o4	!	do {
	inc	%o0		!		*dst++ = *src++;
	stb	%o4, [%o1]
	deccc	%o2
	bnz	0b		!	} while (--len != 0);
	 inc	%o1
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	membar	#StoreStore|#StoreLoad
	retl
	 clr	%o0
	NOTREACHED

	! lowest bit matches, so we can copy by words, if nothing else
1:
	be,a	1f		! if (src & 1) {
	 btst	2, %o3		! [delay slot: if (t & 2)]

	! although low bits match, both are 1: must copy 1 byte to align
	ldsb	[%o0], %o4	!	*dst++ = *src++;
	inc	%o0
	stb	%o4, [%o1]
	dec	%o2		!	len--;
	inc	%o1
	btst	2, %o3		! } [if (t & 2)]
1:
	be,a	1f		! if (t & 2) {
	 btst	2, %o0		! [delay slot: if (src & 2)]
	dec	2, %o2		!	len -= 2;
0:
	ldsh	[%o0], %o4	!	do {
	inc	2, %o0		!		dst += 2, src += 2;
	sth	%o4, [%o1]	!		*(short *)dst = *(short *)src;
	deccc	2, %o2		!	} while ((len -= 2) >= 0);
	bge	0b
	 inc	2, %o1
	b	Lkcopy_mopb	!	goto mop_up_byte;
	 btst	1, %o2		! } [delay slot: if (len & 1)]
	NOTREACHED

	! low two bits match, so we can copy by longwords
1:
	be,a	1f		! if (src & 2) {
	 btst	4, %o3		! [delay slot: if (t & 4)]

	! although low 2 bits match, they are 10: must copy one short to align
	ldsh	[%o0], %o4	!	(*short *)dst = *(short *)src;
	inc	2, %o0		!	dst += 2;
	sth	%o4, [%o1]
	dec	2, %o2		!	len -= 2;
	inc	2, %o1		!	src += 2;
	btst	4, %o3		! } [if (t & 4)]
1:
	be,a	1f		! if (t & 4) {
	 btst	4, %o0		! [delay slot: if (src & 4)]
	dec	4, %o2		!	len -= 4;
0:
	ld	[%o0], %o4	!	do {
	inc	4, %o0		!		dst += 4, src += 4;
	st	%o4, [%o1]	!		*(int *)dst = *(int *)src;
	deccc	4, %o2		!	} while ((len -= 4) >= 0);
	bge	0b
	 inc	4, %o1
	b	Lkcopy_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! } [delay slot: if (len & 2)]
	NOTREACHED

	! low three bits match, so we can copy by doublewords
1:
	be	1f		! if (src & 4) {
	 dec	8, %o2		! [delay slot: len -= 8]
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	inc	4, %o0		!	dst += 4, src += 4, len -= 4;
	st	%o4, [%o1]
	dec	4, %o2		! }
	inc	4, %o1
1:
Lkcopy_doubles:
	ldx	[%o0], %g5	! do {
	inc	8, %o0		!	dst += 8, src += 8;
	stx	%g5, [%o1]	!	*(double *)dst = *(double *)src;
	deccc	8, %o2		! } while ((len -= 8) >= 0);
	bge	Lkcopy_doubles
	 inc	8, %o1

	! check for a usual case again (save work)
	btst	7, %o2		! if ((len & 7) == 0)
	be	Lkcopy_done	!	goto kcopy_done;

	 btst	4, %o2		! if ((len & 4)) == 0)
	be,a	Lkcopy_mopw	!	goto mop_up_word_and_byte;
	 btst	2, %o2		! [delay slot: if (len & 2)]
	ld	[%o0], %o4	!	*(int *)dst = *(int *)src;
	inc	4, %o0		!	dst += 4;
	st	%o4, [%o1]
	inc	4, %o1		!	src += 4;
	btst	2, %o2		! } [if (len & 2)]

1:
	! mop up trailing word (if present) and byte (if present).
Lkcopy_mopw:
	be	Lkcopy_mopb	! no word, go mop up byte
	 btst	1, %o2		! [delay slot: if (len & 1)]
	ldsh	[%o0], %o4	! *(short *)dst = *(short *)src;
	be	Lkcopy_done	! if ((len & 1) == 0) goto done;
	 sth	%o4, [%o1]
	ldsb	[%o0 + 2], %o4	! dst[2] = src[2];
	stb	%o4, [%o1 + 2]
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	membar	#StoreStore|#StoreLoad
	retl
	 clr	%o0
	NOTREACHED

	! mop up trailing byte (if present).
Lkcopy_mopb:
	bne,a	1f
	 ldsb	[%o0], %o4

Lkcopy_done:
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	membar	#StoreStore|#StoreLoad
	retl
	 clr	%o0
	NOTREACHED

1:
	stb	%o4, [%o1]
	membar	#Sync		! Make sure all traps are taken
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	membar	#StoreStore|#StoreLoad
	retl
	 clr	%o0
	NOTREACHED

Lkcerr:
#ifdef DEBUG
	set	pmapdebug, %o4
	ld	[%o4], %o4
	btst	0x80, %o4	! PDB_COPY
	bz,pt	%icc, 3f
	 nop
	save	%sp, -CC64FSZ, %sp
	set	2f, %o0
	call	printf
	 nop
!	ta	1; nop
	restore
	.data
2:	.asciz	"kcopy error\n"
	_ALIGN
	.text
3:
#endif
	STPTR	%g1, [%o5 + PCB_ONFAULT]! restore fault handler
	retl				! and return error indicator
	 membar	#StoreStore|#StoreLoad
	NOTREACHED

/*
 * copystr(fromaddr, toaddr, maxlength, &lencopied)
 *
 * Copy a null terminated string from one point to another in
 * the kernel address space.  (This is a leaf procedure, but
 * it does not seem that way to the C compiler.)
 */
ENTRY(copystr)
	brgz,pt	%o2, 0f	! Make sure len is valid
	 mov	%o1, %o5		!	to0 = to;
	retl
	 mov	ENAMETOOLONG, %o0
0:					! loop:
	ldsb	[%o0], %o4		!	c = *from;
	tst	%o4
	stb	%o4, [%o1]		!	*to++ = c;
	be	1f			!	if (c == 0)
	 inc	%o1			!		goto ok;
	deccc	%o2			!	if (--len > 0) {
	bg,a	0b			!		from++;
	 inc	%o0			!		goto loop;
	b	2f			!	}
	 mov	ENAMETOOLONG, %o0	!	ret = ENAMETOOLONG; goto done;
1:					! ok:
	clr	%o0			!	ret = 0;
2:
	sub	%o1, %o5, %o1		!	len = to - to0;
	tst	%o3			!	if (lencopied)
	bnz,a	3f
	 STPTR	%o1, [%o3]		!		*lencopied = len;
3:
	retl
	 nop
#ifdef DIAGNOSTIC
4:
	sethi	%hi(5f), %o0
	call	_C_LABEL(panic)
	 or	%lo(5f), %o0, %o0
	.data
5:
	.asciz	"copystr"
	_ALIGN
	.text
#endif

