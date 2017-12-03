/*	$NetBSD: octeon_pipvar.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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

#ifndef _OCTEON_PIPVAR_H_
#define _OCTEON_PIPVAR_H_


/* XXX */
struct octeon_pip_softc {
	int			sc_port;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	int			sc_tag_type;
	int			sc_receive_group;
	size_t			sc_ip_offset;
#ifdef OCTEON_ETH_DEBUG
	struct evcnt		sc_ev_pipbeperr;
	struct evcnt		sc_ev_pipfeperr;
	struct evcnt		sc_ev_pipskprunt;
	struct evcnt		sc_ev_pipbadtag;
	struct evcnt		sc_ev_pipprtnxa;
	struct evcnt		sc_ev_pippktdrp;
#endif
};

/* XXX */
struct octeon_pip_attach_args {
	int			aa_port;
	bus_space_tag_t		aa_regt;
	int			aa_tag_type;
	int			aa_receive_group;
	size_t			aa_ip_offset;
};

void			octeon_pip_init(struct octeon_pip_attach_args *,
			    struct octeon_pip_softc **);
void			octeon_pip_gbl_ctl_debug(struct octeon_pip_softc *);
int			octeon_pip_port_config(struct octeon_pip_softc *);
void			octeon_pip_prt_cfg_enable(struct octeon_pip_softc *,
			    uint64_t, int);
void			octeon_pip_stats(struct octeon_pip_softc *,
			    struct ifnet *, int);
#ifdef OCTEON_ETH_DEBUG
void			octeon_pip_int_enable(struct octeon_pip_softc *, int);
void			octeon_pip_dump(void);
void			octeon_pip_dump_regs(void);
void			octeon_pip_dump_stats(void);
#endif /* OCTEON_ETH_DEBUG */

#ifdef OCTEON_ETH_DEBUG
void			octeon_pip_int_enable(struct octeon_pip_softc *, int);
uint64_t		octeon_pip_int_summary(struct octeon_pip_softc *);
#endif /* OCTEON_ETH_DEBUG */


#endif
