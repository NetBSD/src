/*	$NetBSD: lptvar.h,v 1.5 2001/05/31 18:46:07 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Common front-end for mvme68k parallel printer ports
 */

#ifndef __mvme68k_lptvar_h
#define __mvme68k_lptvar_h

#include <sys/callout.h>

struct lpt_funcs;


struct lpt_softc {
	struct device		sc_dev;
	struct callout		sc_wakeup_ch;
	struct lpt_funcs	*sc_funcs;
	void			*sc_arg;
	size_t			sc_count;
	struct buf		*sc_inbuf;
	u_char			*sc_cp;
	int			sc_spinmax;
	u_char			sc_state;
#define	LPT_OPEN	0x01	/* device is open */
#define	LPT_OBUSY	0x02	/* printer is busy doing output */
#define	LPT_INIT	0x04	/* waiting to initialize for open */
	u_char			sc_flags;
#define	LPT_FAST_STROBE	0x10	/* Select 1.6uS strobe pulse */
#define	LPT_AUTOLF	0x20	/* automatic LF on CR */
#define	LPT_NOPRIME	0x40	/* don't prime on open */
#define	LPT_NOINTR	0x80	/* do not use interrupt */

	/* Back-end specific stuff */
	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_bush;
	int			sc_ipl;
	u_char			sc_icr;
	u_char			sc_laststatus;
	struct evcnt		sc_evcnt;
};


struct lpt_funcs {
	void	(*lf_open) __P((struct lpt_softc *, int));
	void	(*lf_close) __P((struct lpt_softc *));
	void	(*lf_iprime) __P((struct lpt_softc *));
	void	(*lf_speed) __P((struct lpt_softc *, int));
	int	(*lf_notrdy) __P((struct lpt_softc *, int));
	void	(*lf_wrdata) __P((struct lpt_softc *, u_char));
};

#define	LPT_STROBE_FAST	0
#define LPT_STROBE_SLOW	1

extern	void	lpt_attach_subr	__P((struct lpt_softc *));
extern	int	lpt_intr	__P((struct lpt_softc *));

#endif	/* __mvme68k_lptvar_h */
