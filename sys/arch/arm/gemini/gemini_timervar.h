/*	$NetBSD: gemini_timervar.h,v 1.1.2.1 2009/01/19 13:15:58 skrll Exp $	*/


/*
 * adapted from:
 *	NetBSD: omap2_mputmrvar.h,v 1.2 2008/04/27 18:58:45 matt Exp
 */

/*
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _GEMINI_TIMER_H
#define _GEMINI_TIMER_H

#ifndef STATHZ
# define STATHZ	HZ
#endif

typedef struct timer_factors {
	uint32_t tf_counts_per_usec;
	uint32_t tf_tmcr;
	uint32_t tf_counter;
	uint32_t tf_reload;
	uint32_t tf_match1;
	uint32_t tf_match2;
} timer_factors_t;

typedef struct geminitmr_softc {
	struct device		sc_dev;
	uint			sc_timerno;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_addr;
	size_t			sc_size;
	int			sc_intr;
	timer_factors_t		sc_tf;
} geminitmr_softc_t;



extern uint32_t hardref;
extern struct timeval hardtime;
extern struct geminitmr_softc *clock_sc;
extern struct geminitmr_softc *stat_sc;
extern struct geminitmr_softc *ref_sc;

extern void gemini_microtime_init(void);

extern int	clockintr(void *);
extern int	statintr(void *);
extern void	rtcinit(void);

#endif	/* _GEMINI_TIMER_H */
