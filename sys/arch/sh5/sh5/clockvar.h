/*	$NetBSD: clockvar.h,v 1.1 2002/07/05 13:32:03 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_CLOCKVAR_H
#define _SH5_CLOCKVAR_H

#include <dev/clock_subr.h>

struct clock_attach_args {
	u_int	ca_rate;		/* Timer ticks per second */
	int	ca_has_stat_clock;	/* Non-zero if supports stat clock */
	void	*ca_arg;		/* Back-end cookie */
	void	(*ca_start)(void *, int, u_int);	/* Start/Reset timer */
	long	(*ca_microtime)(void *);	/* uS since last hardclock() */
};

/*
 * Clock types
 */
#define	CLK_HARDCLOCK	0		/* Interrupts hz times per second */
#define	CLK_STATCLOCK	1		/* Interrupts stathz times per second */

extern	void	clock_config(struct device *, struct clock_attach_args *,
		    struct evcnt *);
extern	void	clock_rtc_config(todr_chip_handle_t);
extern	void	clock_hardint(struct clockframe *);
extern	void	clock_statint(struct clockframe *);

#endif	/* _SH5_CLOCKVAR_H */
