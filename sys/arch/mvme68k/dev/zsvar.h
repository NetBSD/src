/*	$NetBSD: zsvar.h,v 1.8 2000/11/09 19:51:57 scw Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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

#ifndef _MVME68K_ZSVAR_H
#define _MVME68K_ZSVAR_H

/*
 * Non-exported definitons common to the different attachment
 * types for the SCC on the Motorola MVME series of computers.
 */

/*
 * The MVME-147 provides a 5 MHz clock to the SCC chips.
 */
#define ZSMVME_PCLK_147	5000000		/* PCLK pin input clock rate */

/*
 * The MVME-162 provides a 10 MHz clock to the SCC chips.
 */
#define ZSMVME_PCLK_162	10000000	/* PCLK pin input clock rate */

/*
 * ZS should interrupt host at level 4.
 */
#define ZSMVME_HARD_PRI	4

/*
 * XXX Make cnprobe a little easier.
 */
#define NZSMVMEC	2


struct zsmvme_softc {
	struct zsc_softc	sc_zsc;
	struct zs_chanstate	sc_cs_store[2];
	void			*sc_softintr_cookie;
};

struct zsmvme_config {
	bus_space_tag_t	zc_bt;
	struct {
		bus_space_handle_t zc_csrbh;
		bus_space_handle_t zc_databh;
	} zc_s[2];
	int		zc_vector;
	int		zc_pclk;
};

/* Globals exported from zs.c */
extern	u_char zsmvme_init_reg[];
extern	struct zs_chanstate *zsmvme_conschan;

/* Functions exported to ASIC-specific drivers. */
void	zsmvme_config __P((struct zsmvme_softc *, struct zsmvme_config *));
void	zsmvme_cnconfig __P((int, int, struct zsmvme_config *));
int	zsmvme_getc(void *);
void	zsmvme_putc(void *, int);

#endif /* _MVME68K_ZSVAR_H */
