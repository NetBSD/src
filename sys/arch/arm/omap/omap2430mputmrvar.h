/* $NetBSD: omap2430mputmrvar.h,v 1.1.24.1 2008/05/16 02:22:01 yamt Exp $ */
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
/*
 * derived from omapmputmrvar.h
 */

#ifndef _OMAP2430GPTMRVAR_H
#define _OMAP2430GPTMRVAR_H


#ifndef STATHZ
# define STATHZ	64
#endif

typedef struct timer_factors {
	uint32_t ptv;
	uint32_t reload;
	uint32_t counts_per_usec;
} timer_factors;

struct omap2430mputmr_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_intr;
};



extern uint32_t counts_per_usec, counts_per_hz;
extern uint32_t hardref;
extern struct timeval hardtime;
extern struct omap2430mputmr_softc *clock_sc;
extern struct omap2430mputmr_softc *stat_sc;
extern struct omap2430mputmr_softc *ref_sc;

extern void calc_timer_factors(int, struct timer_factors *);
extern int	clockintr(void *);
extern int	statintr(void *);
extern void	rtcinit(void);

#endif	/* _OMAP2430GPTMRVAR_H */
