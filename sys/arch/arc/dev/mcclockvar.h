/* $NetBSD: mcclockvar.h,v 1.1 2001/06/13 15:02:13 soda Exp $ */
/* NetBSD: mcclockvar.h,v 1.4 1997/06/22 08:02:19 jonathan Exp  */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

struct mcclock_softc {
	struct device sc_dev;

	const struct mcclock_busfns *sc_busfns;

	/* the followings may be used by bus-dependent frontend */
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

struct mcclock_busfns {
	u_int   (*mc_bf_read) __P((struct mcclock_softc *, u_int));
	void    (*mc_bf_write) __P((struct mcclock_softc *, u_int, u_int));
};

#define	mc146818_read(dev, reg)						\
	    (*(sc)->sc_busfns->mc_bf_read)(sc, reg)
#define	mc146818_write(sc, reg, datum)					\
	    (*(sc)->sc_busfns->mc_bf_write)(sc, reg, datum)

void	mcclock_attach __P((struct mcclock_softc *,
	    const struct mcclock_busfns *, int));
