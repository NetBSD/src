/*	$NetBSD: sbusvar.h,v 1.1.1.1.2.1 1998/07/30 14:03:51 eeh Exp $ */

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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)sbusvar.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _SBUS_VAR_H_
#define _SBUS_VAR_H_

/*
 * Most devices are configured according to information kept in
 * the FORTH PROMs.  In particular, we extract the `name', `reg',
 * and `address' properties of each device attached to the mainbus;
 * other drives may also use this information.  The mainbus itself
 * (which `is' the CPU, in some sense) gets just the node, with a
 * fake name ("mainbus").
 */

/* Device register space description */
struct sbus_reg {
	u_int32_t	rr_iospace;	/* register space (obio, etc) */
	u_int32_t	rr_paddr;	/* register physical address */
	u_int32_t	rr_len;		/* register length */
};

/* Interrupt information */
struct sbus_intr {
	u_int32_t	int_pri;	/* priority (IPL) */
	u_int32_t	int_vec;	/* vector (always 0?) */
};

/* Address translation accross busses */
struct sbus_range {		/* Only used on v3 PROMs */
	u_int32_t	cspace;		/* Client space */
	u_int32_t	coffset;	/* Client offset */
	u_int32_t	pspace;		/* Parent space */
	u_int32_t	poffset;	/* Parent offset */
	u_int32_t	size;		/* Size in bytes of this range */
};

/*
 * S-bus variables.
 */
struct sbusdev {
	struct	device *sd_dev;		/* backpointer to generic */
	struct	sbusdev *sd_bchain;	/* forward link in bus chain */
	void	(*sd_reset) __P((struct device *));
};

/* variables per Sbus */
struct sbus_softc {
	struct	device sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
	int	sc_clockfreq;		/* clock frequency (in Hz) */
	struct	sbusdev *sc_sbdev;	/* list of all children */
	struct	sbus_range *sc_range;
	int	sc_nrange;
	int	sc_burst;		/* burst transfer sizes supported */
	int	*sc_intr2ipl;		/* Interrupt level translation */
	int	*sc_intr_compat;	/* `intr' property to sbus compat */

	struct sysioreg* sc_sysio;	/* SBUS control registers */
	u_int		sc_pagesize;
	u_int		sc_dvmabase;
        int64_t		*sc_tsb;	/* TSB vaddr */
	int		sc_tsbsize;
	paddr_t		sc_ptsb;	/* TSB physaddr */
	int		sc_hasiocache;
	int		sc_ign;		/* Interrupt group number for this sysio */

	int32_t		sc_flushpa;	/* used to flush the SBUS */
	int64_t		sc_flush;
};

/*
 * Sbus driver attach arguments.
 */
struct sbus_attach_args {
	bus_space_tag_t	sa_bustag;
	bus_dma_tag_t	sa_dmatag;
	char		*sa_name;	/* PROM node name */
	int		sa_node;	/* PROM handle */
	int		sa_slot;	/* Sbus slot number */
	int		sa_offset;	/* Sbus slot offset */
	int		sa_size;	/* Sbus space size */
	int		sa_pri;		/* priority (IPL) */
	void		*sa_promvaddr;	/* PROM virtual address, if any */
	struct bootpath *sa_bp;		/* used for locating boot device */
};

/* sbus_attach() is also used from obio.c */
#if 0
void	sbus_attach __P((struct sbus_softc *, char *, int, struct bootpath *,
			 const char * const *));
#endif
int	sbus_print __P((void *, const char *));

int	sbusdev_match __P((struct cfdata *, void *));
void	sbus_establish __P((struct sbusdev *, struct device *));

int	sbus_setup_attach_args __P((
		struct sbus_softc *,
		bus_space_tag_t,
		bus_dma_tag_t,
		int,			/*node*/
		struct bootpath *,
		struct sbus_attach_args *));

#define sbus_bus_map(t, b, a, s, f, v, hp) \
	bus_space_map2(t, b, a, s, f, v, hp)

#endif /* _SBUS_VAR_H_ */
