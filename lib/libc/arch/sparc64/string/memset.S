/*	$NetBSD: memset.S,v 1.3 2001/08/01 16:45:20 eeh Exp $	*/

/*
 * Copyright (c) 2001, Eduardo E. Horvath
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Header: bzero.s,v 1.1 92/06/25 12:52:46 torek Exp
 */

#include <machine/asm.h>
#ifndef _LOCORE
#define _LOCORE
#endif
#include <machine/ctlreg.h>
#include <machine/frame.h>
#include <machine/psl.h>

#if defined(LIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: memset.S,v 1.3 2001/08/01 16:45:20 eeh Exp $")
#endif  /* LIBC_SCCS and not lint */

/*
 * memset(addr, c, len)
 *
 * Duplicate the pattern so it fills 64-bits, then swap around the
 * arguments and call bzero.
 */
ENTRY(memset)
	and	%o1, 0x0ff, %o3
	mov	%o2, %o1
	sllx	%o3, 8, %o2
	or	%o2, %o3, %o2
	mov	%o0, %o4		! Save original pointer
	sllx	%o2, 16, %o3
	or	%o2, %o3, %o2
	sllx	%o2, 32, %o3
	ba,pt	%icc, Lbzero_internal
	 or	%o2, %o3, %o2
/*
 * bzero(addr, len)
 *
 * We want to use VIS instructions if we're clearing out more than
 * 256 bytes, but to do that we need to properly save and restore the
 * FP registers.  Unfortunately the code to do that in the kernel needs
 * to keep track of the current owner of the FPU, hence the different
 * code.
 *
 */
ENTRY(bzero)
	! %o0 = addr, %o1 = len
	clr	%o2			! Initialize our pattern
Lbzero_internal:
	brz,pn	%o1, Lbzero_done	! No bytes to copy??
	 cmp	%o1, 16			! <16 bytes?  use byte ops.
	bge,pn	%xcc, 1f
	 nop
0:	
	stb	%o2, [%o0]		! Small clear.
	inc	%o0
	deccc	%o1
	bg,pt	%icc, 0b
	 nop
	ba,pt	%icc, Lbzero_done
	 nop
	
1:	
	btst	7, %o0			! 64-bit aligned?  Optimization
	bz,pt	%xcc, 2f
	 nop
	btst	3, %o0			! 32-bit aligned?
	bz,pt	%xcc, 1f
	 nop
	btst	1, %o0			! 16-bit aligned?
	bz,pt	%xcc, 0f
	 nop
	
	!! unaligned -- store 1 byte
	stb	%o2, [%o0]
	dec	1, %o1			! Record storing 1 byte
	inc	%o0
	cmp	%o1, 2
	bl,a,pn	%icc, 7f		! 1 or 0 left
	 dec	8, %o1			! Fixup count -8
0:
	btst	3, %o0
	bz,pt	%xcc, 1f
	 btst	7, %o0			! 64-bit aligned?

	!! 16-bit aligned -- store half word
	sth	%o2, [%o0]
	dec	2, %o1			! Prepare to store 2 bytes
	inc	2, %o0
	cmp	%o1, 4
	bl,a,pn	%icc, 5f		! Less than 4 left
	 dec	8, %o1			! Fixup count -8
1:
	btst	7, %o0			! 64-bit aligned?
	bz,pt	%xcc, 2f
	 nop
	!! 32-bit aligned -- store word
	stw	%o2, [%o0]
	dec	4, %o1
	inc	4, %o0	
	cmp	%o1, 8
	bl,a,pn	%icc, Lbzero_cleanup	! Less than 8 left
	 dec	8, %o1			! Fixup count -8
2:
#if 1
	!! Now we are 64-bit aligned
	cmp	%o1, 256		! Use block clear if len > 256
	bge,pt	%xcc, Lbzero_block	! use block store insns
#endif	
	 deccc	8, %o1
Lbzero_longs:
	bl,pn	%xcc, Lbzero_cleanup	! Less than 8 bytes left
	 nop
3:	
	stx	%o2, [%o0]		! Do 1 longword at a time
	deccc	8, %o1
	bge,pt	%xcc, 3b
	 inc	8, %o0

	/*
	 * Len is in [-8..-1] where -8 => done, -7 => 1 byte to zero,
	 * -6 => two bytes, etc.  Mop up this remainder, if any.
	 */
Lbzero_cleanup:	
	btst	4, %o1
	bz,pt	%xcc, 6f		! if (len & 4) {
	 btst	2, %o1
	stw	%o2, [%o0]		!	*(int *)addr = 0;
	inc	4, %o0			!	addr += 4;
5:	
	btst	2, %o1
6:
	bz,pt	%xcc, 8f		! if (len & 2) {
	 btst	1, %o1
	sth	%o2, [%o0]		!	*(short *)addr = 0;
	inc	2, %o0			!	addr += 2;
7:	
	btst	1, %o1
8:	
	bnz,a	%icc, Lbzero_done	! if (len & 1)
	 stb	%o2, [%o0]		!	*addr = 0;
Lbzero_done:
	retl
	 mov	%o4, %o0		! Restore ponter for memset (ugh)

	/*
	 * Len is in [-8..-1] where -8 => done, -7 => 1 byte to zero,
	 * -6 => two bytes, etc. but we're potentially unaligned.
	 * Do byte stores since it's easiest.
	 */
Lbzero_small:
	inccc	8, %o1
	bz,pn	%icc, Lbzero_done
1:	
	 deccc	%o1
	stb	%o2, [%o0]
	bge,pt	%icc, 1b
	 inc	%o0
	ba,a,pt	%icc, Lbzero_done
	 nop				! XXX spitfire bug?
#if 1	
Lbzero_block:
/*
 * Userland:
 *
 * Floating point registers are volatile.  What luck.
 *
 * See locore.s for the kernel version.
 *
 */	
!	wr	%g0, FPRS_FEF, %fprs			! Enable FPU

	!! We are now 8-byte aligned.  We need to become 64-byte aligned.
	btst	63, %o0
	bz,pt	%xcc, 2f
	 nop
1:	
	stx	%o2, [%o0]
	inc	8, %o0
	btst	63, %o0
	bnz,pt	%xcc, 1b
	 dec	8, %o1

2:
	brz	%o2, 3f					! Skip the memory op
	 fzero	%f0					! for bzero
	
	stx	%o2, [%o0]				! Flush this puppy to RAM
	membar	#StoreLoad
	ldd	[%o0], %f0
3:	
	fmovd	%f0, %f2				! Duplicate the pattern
	fmovd	%f0, %f4
	fmovd	%f0, %f6
	fmovd	%f0, %f8
	fmovd	%f0, %f10
	fmovd	%f0, %f12
	fmovd	%f0, %f14
	
	!! Remember: we were 8 bytes too far
	dec	56, %o1					! Go one iteration too far
5:
	stda	%f0, [%o0] ASI_BLK_P			! Store 64 bytes
	deccc	64, %o1
	ble,pn	%xcc, 6f
	 inc	64, %o0

	stda	%f0, [%o0] ASI_BLK_P			! Store 64 bytes
	deccc	64, %o1
	bg,pn	%xcc, 5b
	 inc	64, %o0
6:
/*
 * Now we're done we need to load the FPU state from where
 * we put it.
 */
	ba,pt	%xcc, Lbzero_longs	! Finish up the remainder
	 addcc	%o1, 56, %o1		! Restore the count
#endif
