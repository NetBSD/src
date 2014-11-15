/*	$NetBSD: zsvar.h,v 1.17 2014/11/15 19:20:01 christos Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)zsvar.h	8.1 (Berkeley) 6/11/93
 */

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc. (Atari modifications)
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Register layout is machine-dependent...
 */

struct zschan {
	uint8_t zc_xxx0;
	volatile uint8_t zc_csr;	/* ctrl,status, and indirect access */
	uint8_t zc_xxx1;
	volatile uint8_t zc_data;	/* data */
};

struct zsdevice {
	struct	zschan zs_chan_a;
	struct	zschan zs_chan_b;
};

/*
 * Software state, per zs channel.
 *
 * The zs chip has insufficient buffering, so we provide a software
 * buffer using a two-level interrupt scheme.  The hardware (high priority)
 * interrupt simply grabs the `cause' of the interrupt and stuffs it into
 * a ring buffer.  It then schedules a software interrupt; the latter
 * empties the ring as fast as it can, hoping to avoid overflow.
 *
 * Interrupts can happen because of:
 *	- received data;
 *	- transmit pseudo-DMA done; and
 *	- status change.
 * These are all stored together in the (single) ring.  The size of the
 * ring is a power of two, to make % operations fast.  Since we need two
 * bits to distinguish the interrupt type, and up to 16 for the received
 * data plus RR1 status, we use 32 bits per ring entry.
 *
 * When the value is a character + RR1 status, the character is in the
 * upper 8 bits of the RR1 status.
 */
#define ZLRB_RING_SIZE		8192		/* ZS line ring buffer size */
#define	ZLRB_RING_MASK		8191		/* mask for same */

/* 0 is reserved (means "no interrupt") */
#define	ZRING_RINT		1		/* receive data interrupt */
#define	ZRING_XINT		2		/* transmit done interrupt */
#define	ZRING_SINT		3		/* status change interrupt */

#define	ZRING_TYPE(x)		((x) & 3)
#define	ZRING_VALUE(x)		((x) >> 8)
#define	ZRING_MAKE(t, v)	((t) | (v) << 8)

struct zs_chanstate {
	struct zschan		*cs_zc;		/* points to hardware regs */
	int			cs_unit;	/* unit number */
	struct	tty		*cs_ttyp;	/* ### */

	/*
	 * We must keep a copy of the write registers as they are
	 * mostly write-only and we sometimes need to set and clear
	 * individual bits (e.g., in WR3).  Not all of these are
	 * needed but 16 bytes is cheap and this makes the addressing
	 * simpler.  Unfortunately, we can only write to some registers
	 * when the chip is not actually transmitting, so whenever
	 * we are expecting a `transmit done' interrupt the preg array
	 * is allowed to `get ahead' of the current values.  In a
	 * few places we must change the current value of a register,
	 * rather than (or in addition to) the pending value; for these
	 * cs_creg[] contains the current value.
	 */
	uint8_t	cs_creg[16];		/* current values */
	uint8_t	cs_preg[16];		/* pending values */
	uint8_t	cs_heldchange;		/* change pending (creg != preg) */
	uint8_t	cs_rr0;			/* last rr0 processed */

	/* pure software data, per channel */
	char	cs_softcar;		/* software carrier */
	char	cs_xxx;			/* (spare) */

	/*
	 * The transmit byte count and address are used for pseudo-DMA
	 * output in the hardware interrupt code.  PDMA can be suspended
	 * to get pending changes done; heldtbc is used for this.  It can
	 * also be stopped for ^S; this sets TS_TTSTOP in tp->t_state.
	 */
	int	cs_tbc;			/* transmit byte count */
	uint8_t *cs_tba;		/* transmit buffer address */
	int	cs_heldtbc;		/* held tbc while xmission stopped */

	/*
	 * Printing an overrun error message often takes long enough to
	 * cause another overrun, so we only print one per second.
	 */
	long	cs_rotime;		/* time of last ring overrun */
	long	cs_fotime;		/* time of last fifo overrun */

	/*
	 * The ring buffer.
	 */
	u_int		cs_rbget;	/* ring buffer `get' index	*/
	volatile u_int	cs_rbput;	/* ring buffer `put' index	*/
	int		*cs_rbuf;	/* type, value pairs	*/
};

/*
 * Macros to read and write individual registers (except 0) in a channel.
 */
#define	ZS_READ(c, r)		((c)->zc_csr = (r), (c)->zc_csr)
#define	ZS_WRITE(c, r, v)	((c)->zc_csr = (r), (c)->zc_csr = (v))

/*
 * Split minor into unit, dialin/dialout & flag nibble.
 */
#define	ZS_UNIT(dev)		(TTUNIT(dev) >> 4) 
#define	ZS_FLAGS(dev)		(TTUNIT(dev) & 0xf) 
#define	ZS_DIALOUT(dev)		TTDIALOUT(dev)
