/*	$NetBSD: cgsixvar.h,v 1.1.6.1 2002/03/16 16:01:33 jdolecek Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * color display (cgsix) driver; common definitions.
 */

union cursor_cmap {		/* colormap, like bt_cmap, but tiny */
	u_char	cm_map[2][3];	/* 2 R/G/B entries */
	u_int	cm_chip[2];	/* 2 chip equivalents */
};

struct cg6_cursor {		/* cg6 hardware cursor status */
	short	cc_enable;		/* cursor is enabled */
	struct	fbcurpos cc_pos;	/* position */
	struct	fbcurpos cc_hot;	/* hot-spot */
	struct	fbcurpos cc_size;	/* size of mask & image fields */
	u_int	cc_bits[2][32];		/* space for mask & image bits */
	union	cursor_cmap cc_color;	/* cursor colormap */
};

/* per-display variables */
struct cgsix_softc {
	struct device	sc_dev;		/* base device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	bus_addr_t	sc_paddr;	/* phys address for device mmap() */

	volatile struct bt_regs *sc_bt;		/* Brooktree registers */
	volatile int *sc_fhc;			/* FHC register */
	volatile struct cg6_thc *sc_thc;	/* THC registers */
	volatile struct cg6_tec_xxx *sc_tec;	/* TEC registers */
	volatile struct cg6_fbc *sc_fbc;	/* FBC registers */
	short	sc_fhcrev;		/* hardware rev */
	short	sc_blanked;		/* true if blanked */
	struct	cg6_cursor sc_cursor;	/* software cursor info */
	union	bt_cmap sc_cmap;	/* Brooktree color map */
};

#ifdef RASTERCONSOLE
extern int cgsix_use_rasterconsole;
#else
#define cgsix_use_rasterconsole 0
#endif

/* XXX - export sbus attach struct for overloaded obio bus */
extern struct cfattach cgsix_sbus_ca;

void	cg6attach(struct cgsix_softc *, char *, int);
