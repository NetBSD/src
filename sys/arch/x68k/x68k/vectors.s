|	$NetBSD: vectors.s,v 1.7.6.4 1999/02/13 18:04:55 minoura Exp $

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

#define _mfptrap	_badtrap
#define _scctrap	_badtrap

	.data
	.globl	_vectab,_buserr,_addrerr
	.globl	_illinst,_zerodiv,_chkinst,_trapvinst,_privinst,_trace
	.globl	_badtrap
	.globl	_spurintr,_lev1intr,_lev2intr,_lev3intr
	.globl	_lev4intr,_lev5intr,_lev6intr,_lev7intr
	.globl	_trap0
#ifdef COMPAT_13
	.globl	_trap1
#endif
	.globl	_trap2,_trap15
	.globl	_fpfline, _fpunsupp, _fpfault
	.globl	_trap12

_vectab:
	.long	0x4ef80000	/* 0: jmp 0x0000:w (unused reset SSP) */
	.long	0		/* 1: NOT USED (reset PC) */
	.long	_buserr		/* 2: bus error */
	.long	_addrerr	/* 3: address error */
	.long	_illinst	/* 4: illegal instruction */
	.long	_zerodiv	/* 5: zero divide */
	.long	_chkinst	/* 6: CHK instruction */
	.long	_trapvinst	/* 7: TRAPV instruction */
	.long	_privinst	/* 8: privilege violation */
	.long	_trace		/* 9: trace */
	.long	_illinst	/* 10: line 1010 emulator */
	.long	_fpfline	/* 11: line 1111 emulator */
	.long	_badtrap	/* 12: unassigned, reserved */
	.long	_coperr		/* 13: coprocessor protocol violation */
	.long	_fmterr		/* 14: format error */
	.long	_badtrap	/* 15: uninitialized interrupt vector */
	.long	_intiotrap	/* 16: unassigned, reserved */
	.long	_intiotrap	/* 17: unassigned, reserved */
	.long	_intiotrap	/* 18: unassigned, reserved */
	.long	_intiotrap	/* 19: unassigned, reserved */
	.long	_intiotrap	/* 20: unassigned, reserved */
	.long	_intiotrap	/* 21: unassigned, reserved */
	.long	_intiotrap	/* 22: unassigned, reserved */
	.long	_intiotrap	/* 23: unassigned, reserved */
	.long	_spurintr	/* 24: spurious interrupt */
	.long	_lev1intr	/* 25: level 1 interrupt autovector */
	.long	_lev2intr	/* 26: level 2 interrupt autovector */
	.long	_lev3intr	/* 27: level 3 interrupt autovector */
	.long	_lev4intr	/* 28: level 4 interrupt autovector */
	.long	_lev5intr	/* 29: level 5 interrupt autovector */
	.long	_lev6intr	/* 30: level 6 interrupt autovector */
	.long	_lev7intr	/* 31: level 7 interrupt autovector */
	.long	_trap0		/* 32: syscalls */
#ifdef COMPAT_13
	.long	_trap1		/* 33: compat_13_sigreturn */
#else
	.long	_illinst
#endif
	.long	_trap2		/* 34: trace */
	.long	_trap3		/* 35: sigreturn special syscall */
	.long	_illinst	/* 36: TRAP instruction vector */
	.long	_illinst	/* 37: TRAP instruction vector */
	.long	_illinst	/* 38: TRAP instruction vector */
	.long	_illinst	/* 39: TRAP instruction vector */
	.long	_illinst	/* 40: TRAP instruction vector */
	.long	_illinst	/* 41: TRAP instruction vector */
	.long	_illinst	/* 42: TRAP instruction vector */
	.long	_illinst	/* 43: TRAP instruction vector */
	.long	_trap12		/* 44: TRAP instruction vector */
	.long	_illinst	/* 45: TRAP instruction vector */
	.long	_illinst	/* 46: TRAP instruction vector */
	.long	_trap15		/* 47: TRAP instruction vector */
#ifdef FPSP
	.globl	bsun, inex, dz, unfl, operr, ovfl, snan
	.long	bsun		/* 48: FPCP branch/set on unordered cond */
	.long	inex		/* 49: FPCP inexact result */
	.long	dz		/* 50: FPCP divide by zero */
	.long	unfl		/* 51: FPCP underflow */
	.long	operr		/* 52: FPCP operand error */
	.long	ovfl		/* 53: FPCP overflow */
	.long	snan		/* 54: FPCP signalling NAN */
#else
	.globl	_fpfault
	.long	_fpfault	/* 48: FPCP branch/set on unordered cond */
	.long	_fpfault	/* 49: FPCP inexact result */
	.long	_fpfault	/* 50: FPCP divide by zero */
	.long	_fpfault	/* 51: FPCP underflow */
	.long	_fpfault	/* 52: FPCP operand error */
	.long	_fpfault	/* 53: FPCP overflow */
	.long	_fpfault	/* 54: FPCP signalling NAN */
#endif

	.long	_fpunsupp	/* 55: FPCP unimplemented data type */
	.long	_badtrap	/* 56: MMU configuration error */
	.long	_intiotrap	/* 57: unassigned, reserved */
	.long	_intiotrap	/* 58: unassigned, reserved */
	.long	_intiotrap	/* 59: unassigned, reserved */
	.long	_intiotrap	/* 60: unassigned, reserved */
	.long	_intiotrap	/* 61: unassigned, reserved */
	.long	_intiotrap	/* 62: unassigned, reserved */
	.long	_intiotrap	/* 63: unassigned, reserved */
	.long	_intiotrap	/* 64: MFP GPIP0 RTC alarm */
	.long	_powtrap	/* 65: MFP GPIP1 ext. power switch */
	.long	_powtrap	/* 66: MFP GPIP2 front power switch */
	.long	_intiotrap	/* 67: MFP GPIP3 FM sound generator */
	.long	_intiotrap	/* 68: MFP timer-D */
	.long	_timertrap	/* 69: MFP timer-C */
	.long	_intiotrap	/* 70: MFP GPIP4 VBL */
	.long	_intiotrap	/* 71: MFP GPIP5 unassigned */
	.long	_kbdtimer	/* 72: MFP timer-B */
	.long	_intiotrap	/* 73: MFP MPSC send error */
	.long	_intiotrap	/* 74: MFP MPSC transmit buffer empty */
	.long	_intiotrap	/* 75: MFP MPSC receive error */
	.long	_intiotrap	/* 76: MFP MPSC receive buffer full */
	.long	_intiotrap	/* 77: MFP timer-A */
	.long	_intiotrap	/* 78: MFP CRTC raster */
	.long	_intiotrap	/* 79: MFP H-SYNC */
	.long	_intiotrap	/* 80: unassigned, reserved */
	.long	_intiotrap	/* 81: unassigned, reserved */
	.long	_intiotrap	/* 82: unassigned, reserved */
	.long	_intiotrap	/* 83: unassigned, reserved */
	.long	_intiotrap	/* 84: unassigned, reserved */
	.long	_intiotrap	/* 85: unassigned, reserved */
	.long	_intiotrap	/* 86: unassigned, reserved */
	.long	_intiotrap	/* 87: unassigned, reserved */
	.long	_intiotrap	/* 88: unassigned, reserved */
	.long	_intiotrap	/* 89: unassigned, reserved */
	.long	_intiotrap	/* 90: unassigned, reserved */
	.long	_intiotrap	/* 91: unassigned, reserved */
	.long	_intiotrap	/* 92: unassigned, reserved */
	.long	_intiotrap	/* 93: unassigned, reserved */
	.long	_intiotrap	/* 94: unassigned, reserved */
	.long	_intiotrap	/* 95: unassigned, reserved */
	.long	_intiotrap	/* 96: FDC */
	.long	_fdeject	/* 97: floppy ejection */
	.long	_intiotrap	/* 98: unassigned, reserved */
	.long	_partrap	/* 99: parallel port */
	.long	_intiotrap	/* 100: FDC DMA */
	.long	_intiotrap	/* 101: FDC DMA (error) */
	.long	_intiotrap	/* 102: unassigned, reserved */
	.long	_intiotrap	/* 103: unassigned, reserved */
	.long	_intiotrap	/* 104: unassigned, reserved */
	.long	_intiotrap	/* 105: unassigned, reserved */
	.long	_intiotrap	/* 106: ADPCM DMA */
	.long	_intiotrap	/* 107: ADPCM DMA */
	.long	_intiotrap	/* 108: internal SPC */
	.long	_intiotrap	/* 109: unassigned, reserved */
	.long	_intiotrap	/* 110: unassigned, reserved */
	.long	_intiotrap	/* 111: unassigned, reserved */
	.long	_intiotrap	/* 112: Z8530 SCC (onboard) */
	.long	_intiotrap	/* 113: Z8530 SCC */
	.long	_intiotrap	/* 114: Z8530 SCC */; 
	.long	_intiotrap	/* 115: unassigned, reserved */
	.long	_intiotrap	/* 116: unassigned, reserved */
	.long	_intiotrap	/* 117: unassigned, reserved */
	.long	_intiotrap	/* 118: unassigned, reserved */
	.long	_intiotrap	/* 119: unassigned, reserved */
	.long	_intiotrap	/* 129: unassigned, reserved */
	.long	_intiotrap	/* 121: unassigned, reserved */
	.long	_intiotrap	/* 122: unassigned, reserved */
	.long	_intiotrap	/* 123: unassigned, reserved */
	.long	_intiotrap	/* 124: unassigned, reserved */
	.long	_intiotrap	/* 125: unassigned, reserved */
	.long	_intiotrap	/* 126: unassigned, reserved */
	.long	_intiotrap	/* 127: unassigned, reserved */
#define BADTRAP16	.long	_intiotrap,_intiotrap,_intiotrap,_intiotrap,\
				_intiotrap,_intiotrap,_intiotrap,_intiotrap,\
				_intiotrap,_intiotrap,_intiotrap,_intiotrap,\
				_intiotrap,_intiotrap,_intiotrap,_intiotrap
	BADTRAP16		/* 128-143: user interrupt vectors */
	BADTRAP16		/* 144-159: user interrupt vectors */
	BADTRAP16		/* 160-175: user interrupt vectors */
	BADTRAP16		/* 176-191: user interrupt vectors */
	BADTRAP16		/* 192-207: user interrupt vectors */
	BADTRAP16		/* 208-223: user interrupt vectors */
	BADTRAP16		/* 224-239: user interrupt vectors */
	.long	_com0trap	/* 240: unassigned, reserved */
	.long	_com1trap	/* 241: unassigned, reserved */
	.long	_intiotrap	/* 242: unassigned, reserved */
	.long	_intiotrap	/* 243: unassigned, reserved */
	.long	_intiotrap	/* 244: unassigned, reserved */
	.long	_intiotrap	/* 245: unassigned, reserved */
	.long	_exspctrap	/* 246: external SPC */
	.long	_intiotrap	/* 247: unassigned, reserved */
	.long	_intiotrap	/* 248: unassigned, reserved */
	.long	_intiotrap	/* 249: Neptune-X */
	.long	_intiotrap	/* 250: unassigned, reserved */
	.long	_intiotrap	/* 251: unassigned, reserved */
	.long	_intiotrap	/* 252: unassigned, reserved */
	.long	_intiotrap	/* 253: unassigned, reserved */
	.long	_intiotrap	/* 254: unassigned, reserved */
	.long	_intiotrap	/* 255: unassigned, reserved */
