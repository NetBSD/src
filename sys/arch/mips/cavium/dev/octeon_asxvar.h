/*	$NetBSD: octeon_asxvar.h,v 1.1.2.2 2015/06/06 14:40:01 skrll Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 */

#ifndef _OCTEON_ASXVAR_H_
#define _OCTEON_ASXVAR_H_

/* XXX */
struct octeon_asx_softc {
	int			sc_port;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
#if defined(OCTEON_DEBUG) || defined(OCTEON_ETH_DEBUG)
	struct evcnt		sc_ev_asxrxpsh;
	struct evcnt		sc_ev_asxtxpop;
	struct evcnt		sc_ev_asxovrflw;
#endif
};

/* XXX */
struct octeon_asx_attach_args {
	int			aa_port;
	bus_space_tag_t		aa_regt;
};

void			octeon_asx_init(struct octeon_asx_attach_args *,
			    struct octeon_asx_softc **);
int			octeon_asx_enable(struct octeon_asx_softc *, int);
int			octeon_asx_clk_set(struct octeon_asx_softc *, int, int);
uint64_t		octeon_asx_int_summary(struct octeon_asx_softc *sc);

#endif
