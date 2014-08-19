|	$NetBSD: vectors.s,v 1.2.8.2 2014/08/20 00:02:57 tls Exp $

| Copyright (c) 1997 Jason R. Thorpe.  All rights reserved.
| Copyright (c) 1988 University of Utah
| Copyright (c) 1990, 1993
|	The Regents of the University of California.  All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions
| are met:
| 1. Redistributions of source code must retain the above copyright
|    notice, this list of conditions and the following disclaimer.
| 2. Redistributions in binary form must reproduce the above copyright
|    notice, this list of conditions and the following disclaimer in the
|    documentation and/or other materials provided with the distribution.
| 3. All advertising materials mentioning features or use of this software
|    must display the following acknowledgement:
|	This product includes software developed by the University of
|	California, Berkeley and its contributors.
| 4. Neither the name of the University nor the names of its contributors
|    may be used to endorse or promote products derived from this software
|    without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
| ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
| IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
| ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
| FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
| DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
| OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
| HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
| OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
| SUCH DAMAGE.
|
|	@(#)vectors.s	8.2 (Berkeley) 1/21/94
|

#define	BADTRAP16	\
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap)

	/*
	 * bus error and address error vectors are initialized
	 * in locore.s once we know our CPU type.
	 */

	.data
GLOBAL(vectab)
	VECTOR_UNUSED		/* 0: (unused reset SSP) */
	VECTOR_UNUSED		/* 1: NOT USED (reset PC) */
	VECTOR_UNUSED		/* 2: bus error */
	VECTOR_UNUSED		/* 3: address error */
	VECTOR(illinst)		/* 4: illegal instruction */
	VECTOR(zerodiv)		/* 5: zero divide */
	VECTOR_UNUSED		/* 6: CHK instruction */
	VECTOR_UNUSED	/* 7: TRAPV instruction */
	VECTOR(privinst)	/* 8: privilege violation */
	VECTOR(trace)		/* 9: trace */
	VECTOR(illinst)		/* 10: line 1010 emulator */
	VECTOR(fpfline)		/* 11: line 1111 emulator */
	VECTOR(badtrap)		/* 12: non-PC breakpoint detected */
	VECTOR(coperr)		/* 13: PC breakpoint detected */
	VECTOR(fmterr)		/* 14: format error */
	VECTOR(badtrap)		/* 15: uninitialized interrupt vector */
	VECTOR(badtrap)		/* 16: unassigned, reserved */
	VECTOR(badtrap)		/* 17: unassigned, reserved */
	VECTOR(badtrap)		/* 18: unassigned, reserved */
	VECTOR(badtrap)		/* 19: unassigned, reserved */
	VECTOR(badtrap)		/* 20: unassigned, reserved */
	VECTOR(badtrap)		/* 21: unassigned, reserved */
	VECTOR(badtrap)		/* 22: unassigned, reserved */
	VECTOR(badtrap)		/* 23: unassigned, reserved */
	VECTOR(intrhand_autovec)/* 24: spurious autovector interrupt */
	VECTOR(intrhand_autovec)/* 25: level 1 interrupt autovector */
	VECTOR(intrhand_autovec)/* 26: level 2 interrupt autovector */
	VECTOR(intrhand_autovec)/* 27: level 3 interrupt autovector */
	VECTOR(intrhand_autovec)/* 28: level 4 interrupt autovector */
	VECTOR(intrhand_autovec)/* 29: level 5 interrupt autovector */
	VECTOR(intrhand_autovec)/* 30: level 6 interrupt autovector */
	VECTOR(intrhand_autovec)/* 31: level 7 interrupt autovector */
	VECTOR(trap0)		/* 32: trap 0 syscalls */
#ifdef COMPAT_13
	VECTOR(trap1)		/* 33: trap 1 compat_13_sigreturn */
#else
	VECTOR(illinst)
#endif
	VECTOR(trap2)		/* 34: trap 2 trace */
#ifdef COMPAT_16
	VECTOR(trap3)		/* 35: trap 3 compat_16_sigreturn */
#else
	VECTOR(illinst)	
#endif
	VECTOR(illinst)		/* 36: trap 4 instruction vector */
	VECTOR(illinst)		/* 37: trap 5 instruction vector */
	VECTOR(illinst)		/* 38: trap 6 instruction vector */
	VECTOR(illinst)		/* 39: trap 7 instruction vector */
	VECTOR(illinst)		/* 40: trap 8 instruction vector */
	VECTOR(illinst)		/* 41: trap 9 instruction vector */
	VECTOR(illinst)		/* 42: trap 10 instruction vector */
	VECTOR(illinst)		/* 43: trap 11 instruction vector */
	VECTOR(trap12)		/* 44: trap 12 instruction vector */
	VECTOR(illinst)		/* 45: trap 13 instruction vector */
	VECTOR(illinst)		/* 46: trap 14 instruction vector */
	VECTOR(trap15)		/* 47: trap 15 instruction vector */
	VECTOR(badtrap)		/* 48: unassigned, reserved */
	VECTOR(badtrap)		/* 49: unassigned, reserved */
	VECTOR(badtrap)		/* 50: unassigned, reserved */
	VECTOR(badtrap)		/* 51: unassigned, reserved */
	VECTOR(badtrap)		/* 52: unassigned, reserved */
	VECTOR(badtrap)		/* 53: unassigned, reserved */
	VECTOR(badtrap)		/* 54: unassigned, reserved */
	VECTOR(badtrap)		/* 55: unassigned, reserved */
	VECTOR(badtrap)		/* 56: unassigned, reserved */
	VECTOR(badtrap)		/* 57: unassigned, reserved */
	VECTOR(badtrap)		/* 58: unassigned, reserved */
	VECTOR(badtrap)		/* 59: unassigned, reserved */
	VECTOR(badtrap)		/* 60: unassigned, reserved */
	VECTOR(illinst)		/* 61: unsupported instruction */
	VECTOR(badtrap)		/* 62: unassigned, reserved */
	VECTOR(badtrap)		/* 63: unassigned, reserved */

	/*
	 * Vectors are installed from 64-255 by the *intr_extablish() functions.
	 */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
