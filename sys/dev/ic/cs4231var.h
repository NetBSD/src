/*	$NetBSD: cs4231var.h,v 1.1 1999/06/05 14:29:10 mrg Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
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
 * Software state, per CS4231 audio chip.
 */
struct cs4231_softc {
	struct ad1848_softc sc_ad1848;	/* base device */
	/* XXX */
	struct sbusdev	sc_sd;		/* sbus device */
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
	struct evcnt	sc_intrcnt;	/* statistics */

	struct cs_dma	*sc_dmas;
	struct cs_dma	*sc_nowplaying;	/*XXX*/
	u_long		sc_playsegsz;	/*XXX*/
	u_long		sc_playcnt;
	u_long		sc_blksz;

	int	sc_open;		/* single use device */
	int	sc_locked;		/* true when transfering data */
	struct	apc_dma	*sc_dmareg;	/* DMA registers */

	/* interfacing with the interrupt handlers */
	void	(*sc_rintr)(void*);	/* input completion intr handler */
	void	*sc_rarg;		/* arg for sc_rintr() */
	void	(*sc_pintr)(void*);	/* output completion intr handler */
	void	*sc_parg;		/* arg for sc_pintr() */
};

void	cs4231_init __P((struct cs4231_softc *));
int	cs4231_read __P((struct ad1848_softc *, int));
void	cs4231_write __P((struct ad1848_softc *, int, int));
int	cs4231_intr __P((void *));
extern	struct audio_hw_if audiocs_hw_if;
