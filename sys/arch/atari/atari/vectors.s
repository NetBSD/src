/*	$NetBSD: vectors.s,v 1.13.12.3 2001/04/21 17:53:21 bouyer Exp $	*/

/*
 * Copyright (c) 1988 University of Utah
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)vectors.s	7.2 (Berkeley) 5/7/91
 */
	.data
GLOBAL(vectab)
	.long	0x4ef80400	| 0: jmp 0x400:w (unused reset SSP)
	.long	0		| 1: NOT USED (reset PC)
	VECTOR(buserr)		| 2: bus error
	VECTOR(addrerr)		| 3: address error
	VECTOR(illinst)		| 4: illegal instruction
	VECTOR(zerodiv)		| 5: zero divide
	VECTOR(chkinst)		| 6: CHK instruction
	VECTOR(trapvinst)	| 7: TRAPV instruction
	VECTOR(privinst)	| 8: privilege violation
	VECTOR(trace)		| 9: trace
	VECTOR(illinst)		| 10: line 1010 emulator
	VECTOR(fpfline)		| 11: line 1111 emulator
	VECTOR(badtrap)		| 12: unassigned, reserved
	VECTOR(coperr)		| 13: coprocessor protocol violation
	VECTOR(fmterr)		| 14: format error
	VECTOR(badtrap)		| 15: uninitialized interrupt vector
	VECTOR(badtrap)		| 16: unassigned, reserved
	VECTOR(badtrap)		| 17: unassigned, reserved
	VECTOR(badtrap)		| 18: unassigned, reserved
	VECTOR(badtrap)		| 19: unassigned, reserved
	VECTOR(badtrap)		| 20: unassigned, reserved
	VECTOR(badtrap)		| 21: unassigned, reserved
	VECTOR(badtrap)		| 22: unassigned, reserved
	VECTOR(badtrap)		| 23: unassigned, reserved
	VECTOR(spurintr)	| 24: spurious interrupt

GLOBAL(autovects)
#ifdef _ATARIHW_
	VECTOR(lev1intr)	| 25: level 1 interrupt autovector
#else
	VECTOR(badtrap)		| 25: Not supported by hardware
#endif
	VECTOR(lev2intr)	| 26: level 2 interrupt autovector
	VECTOR(lev3intr)	| 27: level 3 interrupt autovector
	VECTOR(lev4intr)	| 28: level 4 interrupt autovector
	VECTOR(lev5intr)	| 29: level 5 interrupt autovector
	VECTOR(lev6intr)	| 30: level 6 interrupt autovector
#ifdef _ATARIHW_
	VECTOR(lev7intr)	| 31: level 7 interrupt autovector
#else
	VECTOR(badtrap)		| 31: Not supported by hardware
#endif
	VECTOR(trap0)		| 32: syscalls
#ifdef COMPAT_13
	VECTOR(trap1)		| 33: compat_13_sigreturn
#else
	VECTOR(illinst
#endif
	VECTOR(trap2)		| 34: trace
	VECTOR(trap3)		| 35: sigreturn special syscall
	VECTOR(illinst)		| 36: TRAP instruction vector
	VECTOR(illinst)		| 37: TRAP instruction vector
	VECTOR(illinst)		| 38: TRAP instruction vector
	VECTOR(illinst)		| 39: TRAP instruction vector
	VECTOR(illinst)		| 40: TRAP instruction vector
	VECTOR(illinst)		| 41: TRAP instruction vector
	VECTOR(illinst)		| 42: TRAP instruction vector
	VECTOR(illinst)		| 43: TRAP instruction vector
	VECTOR(trap12)		| 44: TRAP instruction vector
	VECTOR(illinst)		| 45: TRAP instruction vector
	VECTOR(illinst)		| 46: TRAP instruction vector
	VECTOR(trap15)		| 47: TRAP instruction vector
#ifdef FPSP
	ASVECTOR(bsun)		| 48: FPCP branch/set on unordered cond
	ASVECTOR(inex)		| 49: FPCP inexact result
	ASVECTOR(dz)		| 50: FPCP divide by zero
	ASVECTOR(unfl)		| 51: FPCP underflow
	ASVECTOR(operr)		| 52: FPCP operand error
	ASVECTOR(ovfl)		| 53: FPCP overflow
	ASVECTOR(snan)		| 54: FPCP signalling NAN
#else
	VECTOR(fpfault)		| 48: FPCP branch/set on unordered cond
	VECTOR(fpfault)		| 49: FPCP inexact result
	VECTOR(fpfault)		| 50: FPCP divide by zero
	VECTOR(fpfault)		| 51: FPCP underflow
	VECTOR(fpfault)		| 52: FPCP operand error
	VECTOR(fpfault)		| 53: FPCP overflow
	VECTOR(fpfault)		| 54: FPCP signalling NAN
#endif

	VECTOR(fpunsupp)	| 55: FPCP unimplemented data type
	VECTOR(badtrap)		| 56: unassigned, reserved
	VECTOR(badtrap)		| 57: unassigned, reserved
	VECTOR(badtrap)		| 58: unassigned, reserved
	VECTOR(badtrap)		| 59: unassigned, reserved
	VECTOR(badtrap)		| 60: unassigned, reserved
	VECTOR(badtrap)		| 61: unassigned, reserved
	VECTOR(badtrap)		| 62: unassigned, reserved
	VECTOR(badtrap)		| 63: unassigned, reserved

GLOBAL(uservects)
	/*
	 * MFP 1 auto vectors (ipl 6)
	 */
	VECTOR(intr_glue)	|  64: parallel port - BUSY
	VECTOR(badmfpint)	|  65: modem port 1 - DCD
	VECTOR(badmfpint)	|  66: modem port 1 - CTS
	VECTOR(badmfpint)	|  67: ISA1 [ Hades only ]
	VECTOR(badmfpint)	|  68: modem port 1 baudgen (Timer D)
#ifdef STATCLOCK
	ASVECTOR(mfp_timc)	|  69: Timer C {stat,prof}clock
#else
	VECTOR(badmfpint)	|  69: Timer C
#endif /* STATCLOCK */
#if NKBD > 0
	ASVECTOR(mfp_kbd)	|  70: KBD/MIDI IRQ
#else
	VECTOR(badmfpint)	|  70:
#endif /* NKBD > 0 */
	VECTOR(intr_glue)	|  71: FDC/ACSI DMA
	VECTOR(badmfpint)	|  72: Display enable counter
	VECTOR(badmfpint)	|  73: modem port 1 - XMIT error
	VECTOR(badmfpint)	|  74: modem port 1 - XMIT buffer empty
	VECTOR(badmfpint)	|  75: modem port 1 - RCV error	
	VECTOR(badmfpint)	|  76: modem port 1 - RCV buffer full
	ASVECTOR(mfp_tima)	|  77: Timer A (System clock)
	VECTOR(badmfpint)	|  78: modem port 1 - RI
	VECTOR(badmfpint)	|  79: Monochrome detect (ISA2 [ Hades only ])

	/*
	 * MFP 2 auto vectors (ipl 6)
	 */
	VECTOR(badmfpint)	|  80: I/O pin 1 J602
	VECTOR(badmfpint)	|  81: I/O pin 3 J602
	VECTOR(badmfpint)	|  82: SCC-DMA
	VECTOR(badmfpint)	|  83: modem port 2 - RI
	VECTOR(badmfpint)	|  84: serial port 1 baudgen (Timer D)
	VECTOR(badmfpint)	|  85: TCCLC SCC (Timer C)
	VECTOR(badmfpint)	|  86: FDC Drive Ready
#if NNCRSCSI > 0
	ASVECTOR(mfp2_5380dm)	|  87: SCSI DMA
#else
	VECTOR(badmfpint)	|  87:
#endif /* NNCRSCSI > 0 */
	VECTOR(badmfpint)	|  88: Display enable (Timer B)
	VECTOR(badmfpint)	|  89: serial port 1 - XMIT error
	VECTOR(badmfpint)	|  90: serial port 1 - XMIT buffer empty
	VECTOR(badmfpint)	|  91: serial port 1 - RCV error
	VECTOR(badmfpint)	|  92: serial port 1 - RCV buffer full
	VECTOR(badmfpint)	|  93: Timer A
	VECTOR(badmfpint)	|  94: RTC
#if NNCRSCSI > 0
	ASVECTOR(mfp2_5380)	|  95: SCSI 5380
#else
	VECTOR(badmfpint)	|  95:
#endif /* NNCRSCSI > 0 */

#if NZS > 0
	/*
	 * Interrupts from the 8530 SCC
	 */
	ASVECTOR(sccint)	|  96: SCC Tx empty channel B
	VECTOR(badtrap)		|  97: Not used
	ASVECTOR(sccint)	|  98: SCC Ext./Status Channel B
	VECTOR(badtrap)		|  99: Not used
	ASVECTOR(sccint)	| 100: SCC Rx Channel B
	VECTOR(badtrap)		| 101: Not used
	ASVECTOR(sccint)	| 102: SCC Special Rx cond.  Channel B
	VECTOR(badtrap)		| 103: Not used
	ASVECTOR(sccint)	| 104: SCC Tx empty channel A
	VECTOR(badtrap)		| 105: Not used
	ASVECTOR(sccint)	| 106: SCC Ext./Status Channel A
	VECTOR(badtrap)		| 107: Not used
	ASVECTOR(sccint)	| 108: SCC Rx Channel A
	VECTOR(badtrap)		| 109: Not used
	ASVECTOR(sccint)	| 110: SCC Special Rx cond.  Channel A
	VECTOR(badtrap)		| 111: Not used
#else
	VECTOR(badtrap)		|  96: Not used
	VECTOR(badtrap)		|  97: Not used
	VECTOR(badtrap)		|  98: Not used
	VECTOR(badtrap)		|  99: Not used
	VECTOR(badtrap)		| 100: Not used
	VECTOR(badtrap)		| 101: Not used
	VECTOR(badtrap)		| 102: Not used
	VECTOR(badtrap)		| 103: Not used
	VECTOR(badtrap)		| 104: Not used
	VECTOR(badtrap)		| 105: Not used
	VECTOR(badtrap)		| 106: Not used
	VECTOR(badtrap)		| 107: Not used
	VECTOR(badtrap)		| 108: Not used
	VECTOR(badtrap)		| 109: Not used
	VECTOR(badtrap)		| 110: Not used
	VECTOR(badtrap)		| 111: Not used
#endif /* NZS > 0 */

#define BADTRAP16	VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ; \
			VECTOR(badtrap) ; VECTOR(badtrap) ;
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
	BADTRAP16		| 112-255: user interrupt vectors
