|	$NetBSD: vectors.s,v 1.17.2.1 2014/08/20 00:03:29 tls Exp $

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
| 3. Neither the name of the University nor the names of its contributors
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

	.data
GLOBAL(vectab)
	.long	0x4ef80000	/* 0: jmp 0x0000:w (unused reset SSP) */
	VECTOR_UNUSED		/* 1: NOT USED (reset PC) */
	VECTOR_UNUSED		/* 2: bus error (set per CPU types) */
	VECTOR_UNUSED		/* 3: address error (set per CPU types) */
	VECTOR(illinst)		/* 4: illegal instruction */
	VECTOR(zerodiv)		/* 5: zero divide */
	VECTOR(chkinst)		/* 6: CHK instruction */
	VECTOR(trapvinst)	/* 7: TRAPV instruction */
	VECTOR(privinst)	/* 8: privilege violation */
	VECTOR(trace)		/* 9: trace */
	VECTOR(illinst)		/* 10: line 1010 emulator */
	VECTOR(fpfline)		/* 11: line 1111 emulator */
	VECTOR(badtrap)		/* 12: unassigned, reserved */
	VECTOR(coperr)		/* 13: coprocessor protocol violation */
	VECTOR(fmterr)		/* 14: format error */
	VECTOR(badtrap)		/* 15: uninitialized interrupt vector */
	VECTOR(intiotrap)	/* 16: unassigned, reserved */
	VECTOR(intiotrap)	/* 17: unassigned, reserved */
	VECTOR(intiotrap)	/* 18: unassigned, reserved */
	VECTOR(intiotrap)	/* 19: unassigned, reserved */
	VECTOR(intiotrap)	/* 20: unassigned, reserved */
	VECTOR(intiotrap)	/* 21: unassigned, reserved */
	VECTOR(intiotrap)	/* 22: unassigned, reserved */
	VECTOR(intiotrap)	/* 23: unassigned, reserved */
	VECTOR(spurintr)	/* 24: spurious interrupt */
	VECTOR(lev1intr)	/* 25: level 1 interrupt autovector */
	VECTOR(lev2intr)	/* 26: level 2 interrupt autovector */
	VECTOR(lev3intr)	/* 27: level 3 interrupt autovector */
	VECTOR(lev4intr)	/* 28: level 4 interrupt autovector */
	VECTOR(lev5intr)	/* 29: level 5 interrupt autovector */
	VECTOR(lev6intr)	/* 30: level 6 interrupt autovector */
	VECTOR(lev7intr)	/* 31: level 7 interrupt autovector */
	VECTOR(trap0)		/* 32: syscalls */
#ifdef COMPAT_13
	VECTOR(trap1)		/* 33: compat_13_sigreturn */
#else
	VECTOR(illinst)
#endif
	VECTOR(trap2)		/* 34: trace */
#ifdef COMPAT_16
	VECTOR(trap3)		/* 35: compat_16_sigreturn */
#else
	VECTOR(illinst)	
#endif
	VECTOR(illinst)		/* 36: TRAP instruction vector */
	VECTOR(illinst)		/* 37: TRAP instruction vector */
	VECTOR(illinst)		/* 38: TRAP instruction vector */
	VECTOR(illinst)		/* 39: TRAP instruction vector */
	VECTOR(illinst)		/* 40: TRAP instruction vector */
	VECTOR(illinst)		/* 41: TRAP instruction vector */
	VECTOR(illinst)		/* 42: TRAP instruction vector */
	VECTOR(illinst)		/* 43: TRAP instruction vector */
	VECTOR(trap12)		/* 44: TRAP instruction vector */
	VECTOR(illinst)		/* 45: TRAP instruction vector */
	VECTOR(illinst)		/* 46: TRAP instruction vector */
	VECTOR(trap15)		/* 47: TRAP instruction vector */
#ifdef FPSP
	ASVECTOR(bsun)		/* 48: FPCP branch/set on unordered cond */
	ASVECTOR(inex)		/* 49: FPCP inexact result */
	ASVECTOR(dz)		/* 50: FPCP divide by zero */
	ASVECTOR(unfl)		/* 51: FPCP underflow */
	ASVECTOR(operr)		/* 52: FPCP operand error */
	ASVECTOR(ovfl)		/* 53: FPCP overflow */
	ASVECTOR(snan)		/* 54: FPCP signalling NAN */
#else
	VECTOR(fpfault)		/* 48: FPCP branch/set on unordered cond */
	VECTOR(fpfault)		/* 49: FPCP inexact result */
	VECTOR(fpfault)		/* 50: FPCP divide by zero */
	VECTOR(fpfault)		/* 51: FPCP underflow */
	VECTOR(fpfault)		/* 52: FPCP operand error */
	VECTOR(fpfault)		/* 53: FPCP overflow */
	VECTOR(fpfault)		/* 54: FPCP signalling NAN */
#endif

	VECTOR(fpunsupp)	/* 55: FPCP unimplemented data type */
	VECTOR(badtrap)		/* 56: MMU configuration error */
	VECTOR(intiotrap)	/* 57: unassigned, reserved */
	VECTOR(intiotrap)	/* 58: unassigned, reserved */
	VECTOR(intiotrap)	/* 59: unassigned, reserved */
	VECTOR(intiotrap)	/* 60: unassigned, reserved */
	VECTOR(intiotrap)	/* 61: unassigned, reserved */
	VECTOR(intiotrap)	/* 62: unassigned, reserved */
	VECTOR(intiotrap)	/* 63: unassigned, reserved */
	VECTOR(intiotrap)	/* 64: MFP GPIP0 RTC alarm */
	VECTOR(intiotrap)	/* 65: MFP GPIP1 ext. power switch */
	VECTOR(intiotrap)	/* 66: MFP GPIP2 front power switch */
	VECTOR(intiotrap)	/* 67: MFP GPIP3 FM sound generator */
	VECTOR(intiotrap)	/* 68: MFP timer-D */
	VECTOR(timertrap)	/* 69: MFP timer-C */
	VECTOR(intiotrap)	/* 70: MFP GPIP4 VBL */
	VECTOR(intiotrap)	/* 71: MFP GPIP5 unassigned */
	VECTOR(kbdtimer)	/* 72: MFP timer-B */
	VECTOR(intiotrap)	/* 73: MFP MPSC send error */
	VECTOR(intiotrap)	/* 74: MFP MPSC transmit buffer empty */
	VECTOR(intiotrap)	/* 75: MFP MPSC receive error */
	VECTOR(intiotrap)	/* 76: MFP MPSC receive buffer full */
	VECTOR(intiotrap)	/* 77: MFP timer-A */
	VECTOR(intiotrap)	/* 78: MFP CRTC raster */
	VECTOR(intiotrap)	/* 79: MFP H-SYNC */
	VECTOR(intiotrap)	/* 80: unassigned, reserved */
	VECTOR(intiotrap)	/* 81: unassigned, reserved */
	VECTOR(intiotrap)	/* 82: unassigned, reserved */
	VECTOR(intiotrap)	/* 83: unassigned, reserved */
	VECTOR(intiotrap)	/* 84: unassigned, reserved */
	VECTOR(intiotrap)	/* 85: unassigned, reserved */
	VECTOR(intiotrap)	/* 86: unassigned, reserved */
	VECTOR(intiotrap)	/* 87: unassigned, reserved */
	VECTOR(intiotrap)	/* 88: unassigned, reserved */
	VECTOR(intiotrap)	/* 89: unassigned, reserved */
	VECTOR(intiotrap)	/* 90: unassigned, reserved */
	VECTOR(intiotrap)	/* 91: unassigned, reserved */
	VECTOR(intiotrap)	/* 92: unassigned, reserved */
	VECTOR(intiotrap)	/* 93: unassigned, reserved */
	VECTOR(intiotrap)	/* 94: unassigned, reserved */
	VECTOR(intiotrap)	/* 95: unassigned, reserved */
	VECTOR(intiotrap)	/* 96: FDC */
	VECTOR(fdeject)		/* 97: floppy ejection */
	VECTOR(intiotrap)	/* 98: unassigned, reserved */
	VECTOR(intiotrap)	/* 99: parallel port */
	VECTOR(intiotrap)	/* 100: FDC DMA */
	VECTOR(intiotrap)	/* 101: FDC DMA (error) */
	VECTOR(intiotrap)	/* 102: unassigned, reserved */
	VECTOR(intiotrap)	/* 103: unassigned, reserved */
	VECTOR(intiotrap)	/* 104: unassigned, reserved */
	VECTOR(intiotrap)	/* 105: unassigned, reserved */
	VECTOR(intiotrap)	/* 106: ADPCM DMA */
	VECTOR(intiotrap)	/* 107: ADPCM DMA */
	VECTOR(intiotrap)	/* 108: internal SPC */
	VECTOR(intiotrap)	/* 109: unassigned, reserved */
	VECTOR(intiotrap)	/* 110: unassigned, reserved */
	VECTOR(intiotrap)	/* 111: unassigned, reserved */
	VECTOR(intiotrap)	/* 112: Z8530 SCC (onboard) */
	VECTOR(intiotrap)	/* 113: Z8530 SCC */
	VECTOR(intiotrap)	/* 114: Z8530 SCC */
	VECTOR(intiotrap)	/* 115: unassigned, reserved */
	VECTOR(intiotrap)	/* 116: unassigned, reserved */
	VECTOR(intiotrap)	/* 117: unassigned, reserved */
	VECTOR(intiotrap)	/* 118: unassigned, reserved */
	VECTOR(intiotrap)	/* 119: unassigned, reserved */
	VECTOR(intiotrap)	/* 129: unassigned, reserved */
	VECTOR(intiotrap)	/* 121: unassigned, reserved */
	VECTOR(intiotrap)	/* 122: unassigned, reserved */
	VECTOR(intiotrap)	/* 123: unassigned, reserved */
	VECTOR(intiotrap)	/* 124: unassigned, reserved */
	VECTOR(intiotrap)	/* 125: unassigned, reserved */
	VECTOR(intiotrap)	/* 126: unassigned, reserved */
	VECTOR(intiotrap)	/* 127: unassigned, reserved */

#define BADTRAP16	\
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ; \
	VECTOR(intiotrap) ; VECTOR(intiotrap) ;

	BADTRAP16		/* 128-143: user interrupt vectors */
	BADTRAP16		/* 144-159: user interrupt vectors */
	BADTRAP16		/* 160-175: user interrupt vectors */
	BADTRAP16		/* 176-191: user interrupt vectors */
	BADTRAP16		/* 192-207: user interrupt vectors */
	BADTRAP16		/* 208-223: user interrupt vectors */
	BADTRAP16		/* 224-239: user interrupt vectors */
	VECTOR(intiotrap)	/* 240: PSX16550, port1 */
	VECTOR(intiotrap)	/* 241: PSX16550, port2 */
	VECTOR(intiotrap)	/* 242: unassigned, reserved */
	VECTOR(intiotrap)	/* 243: unassigned, reserved */
	VECTOR(intiotrap)	/* 244: unassigned, reserved */
	VECTOR(intiotrap)	/* 245: unassigned, reserved */
	VECTOR(intiotrap)	/* 246: external SPC */
	VECTOR(intiotrap)	/* 247: unassigned, reserved */
	VECTOR(intiotrap)	/* 248: unassigned, reserved */
	VECTOR(intiotrap)	/* 249: Neptune-X */
	VECTOR(intiotrap)	/* 250: unassigned, reserved */
	VECTOR(intiotrap)	/* 251: unassigned, reserved */
	VECTOR(intiotrap)	/* 252: unassigned, reserved */
	VECTOR(intiotrap)	/* 253: unassigned, reserved */
	VECTOR(intiotrap)	/* 254: unassigned, reserved */
	VECTOR(intiotrap)	/* 255: unassigned, reserved */
