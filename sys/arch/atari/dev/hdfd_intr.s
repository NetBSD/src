/*	$NetBSD: hdfd_intr.s,v 1.6.20.1 2001/10/01 12:38:10 fvdl Exp $

/*
 * Copyright (c) 1996 Leo Weppelman.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by Leo Weppelman.
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
 */

#include "assym.h"
#include <machine/asm.h>
#define ASSEMBLER /* XXX */
#include <atari/dev/hdfdreg.h>

	.text
	.globl	_C_LABEL(fddmaaddr)
	.globl	_C_LABEL(fdio_addr),_C_LABEL(fddmalen)

/*
 * Entry point when there is no fifo. Handles the read/write
 * interrupts a bit faster because it *knows* that there is only
 * one character waiting.
 */
ENTRY_NOPROFILE(mfp_hdfd_nf)
	addql	#1,nintr		|  add another interrupt

	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	movl	_C_LABEL(fdio_addr),%a0	|  Get base of fdc registers
	movb	%a0@(fdsts),%d0		|  Get fdsts
	btst	#5,%d0			|  Dma active?
	jeq	hdfdc_norm		|  No, normal interrupt
	tstl	_C_LABEL(fddmalen)	|  Bytecount zero?
	jeq	hdfdc_norm		|  Yes -> normal interrupt

	movl	_C_LABEL(fddmaaddr),%a1	|  a1 = dmabuffer
	btst	#6,%d0			|  Read?
	jeq	hdfd_wrt_nf		|  No, write
hdfd_rd_nf:
	movb	%a0@(fddata),%a1@+	|  Get a byte
1:
	subql	#1, _C_LABEL(fddmalen)	|  decrement bytecount
	movl	%a1,_C_LABEL(fddmaaddr)	|  update dma pointer
|	addql	#1,_cnt+V_INTR		|  chalk up another interrupt
	moveml	%sp@+,%d0-%d1/%a0-%a1
	rte
hdfd_wrt_nf:
	movb	%a1@+,%a0@(fddata)	|  Push a byte
	jra	1b			|  And get out...

/*
 * Systems *with* fifo's enter here.
 */
ENTRY_NOPROFILE(mfp_hdfd_fifo)
	addql	#1,_C_LABEL(intrcnt_user)+88	|  add another interrupt

	moveml	%d0-%d1/%a0-%a1,%sp@-	|  Save scratch registers
	movl	_C_LABEL(fdio_addr),%a0	|  Get base of fdc registers
	movb	%a0@(fdsts),%d0		|  Get fdsts
	btst	#5,%d0			|  Dma active?
	jeq	hdfdc_norm		|  No, normal interrupt
	movl	_C_LABEL(fddmaaddr),%a1	|  a1 = dmabuffer
	btst	#6,%d0			|  Read?
	jeq	hdfd_wrt		|  No, write

hdfd_rd:
	tstl	_C_LABEL(fddmalen)	|  Bytecount zero?
	jeq	hdfdc1			|  Yes -> done
	movb	%a0@(fddata),%a1@+	|  Get a byte
	subql	#1, _C_LABEL(fddmalen)	|  decrement bytecount
	movb	%a0@(fdsts),%d0		|  Get fdsts
	andb	#0xa0,%d0		|  both NE7_NDM and NE7_RQM active?
	cmpb	#0xa0,%d0
	jne	hdfdc1			|  No, end of this batch
	jra	hdfd_rd

hdfd_wrt:
	tstl	_C_LABEL(fddmalen)	|  Bytecount zero?
	jeq	hdfdc1			|  Yes -> done
	movb	%a1@+,%a0@(fddata)	|  Push a byte
	subql	#1, _C_LABEL(fddmalen)	|  decrement bytecount
	jra	hdfdc1
	movb	%a0@(fdsts),%d0		|  Get fdsts
	andb	#0xa0,%d0		|  both NE7_NDM and NE7_RQM active?
	cmpb	#0xa0,%d0
	jne	hdfdc1			|  No, end of this batch
	jra	hdfd_wrt

hdfdc1:
	movl	%a1,_C_LABEL(fddmaaddr)	|  update buffer pointer
	btst	#5,%d0			|  Dma still active?
	jeq	hdfdc_norm		|  No -> take normal interrupt

	/*
	 * Exit for read/write interrupts. Calling 'rei' this often
	 * seems wrong....
	 */
hdfdc_xit:
	addql	#1,_C_LABEL(uvmexp)+UVMEXP_INTRS
	moveml	%sp@+,%d0-%d1/%a0-%a1
	rte

	/*
	 * No (more) data transfer interrupts. Do the normal
	 * stuff.
	 */
hdfdc_norm:
	tstl	nintr
	jeq	0f
	movl	nintr,%d0
	clrl	nintr
	addl	%d0, _C_LABEL(intrcnt_user)+88	|  add another interrupt
	addl	%d0,_C_LABEL(uvmexp)+UVMEXP_INTRS
0:	jbsr	_C_LABEL(fdc_ctrl_intr)		|  handle interrupt
	moveml	%sp@+,%d0-%d1/%a0-%a1	|    and saved registers
	jra	_ASM_LABEL(rei)

	.data
nintr:
	.long	0
