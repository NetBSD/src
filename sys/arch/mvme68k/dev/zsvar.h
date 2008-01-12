/*	$NetBSD: zsvar.h,v 1.11 2008/01/12 09:54:28 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Non-exported definitons common to the different attachment
 * types for the SCC on the Motorola MVME series of computers.
 */

/*
 * The MVME-147 provides a 5 MHz clock to the SCC chips.
 */
#define PCLK_147	5000000		/* PCLK pin input clock rate */

/*
 * The MVME-162 provides a 10 MHz clock to the SCC chips.
 */
#define PCLK_162	10000000	/* PCLK pin input clock rate */

/*
 * SCC should interrupt host at level 4.
 */
#define ZSHARD_PRI	4

/*
 * No delay needed when writing SCC registers.
 */
#define ZS_DELAY()

/*
 * XXX Make cnprobe a little easier.
 */
#define NZSC	2

/*
 * The layout of this is hardware-dependent (padding, order).
 */
struct zschan {
	volatile u_char *zc_csr;	/* ctrl,status, and indirect access */
	volatile u_char *zc_data;	/* data */
};

struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

/* Globals exported from zs.c */
extern	u_char zs_init_reg[];

/* Functions exported to ASIC-specific drivers. */
void	zs_config(struct zsc_softc *, struct zsdevice *, int, int);
void	zs_cnconfig(int, int, struct zsdevice *, int);
#ifdef MVME147
int	zshard_shared(void *);
#endif
#if defined(MVME162) || defined(MVME172)
int	zshard_unshared(void *);
#endif
