/*	$NetBSD: ioasicvar.h,v 1.3.4.3 1999/03/05 02:59:25 nisimura Exp $	*/

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
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

struct ioasic_softc {
	struct	device sc_dv;
	void	*sc_cookie;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_lance_dmam;

	/* XXX XXX XXX */
	tc_addr_t sc_base;
	unsigned sc_ioasic_imsk;
	unsigned sc_ioasic_intr;
	unsigned sc_ioasic_rtc;
};
extern struct cfdriver ioasic_cd;

/* Attachment arguments. */
struct ioasicdev_attach_args {
	char	iada_modname[TC_ROM_LLEN];
	tc_offset_t iada_offset;
	tc_addr_t iada_addr;
	void	*iada_cookie;
};

/* Device locators. */
#include "locators.h"
#define	ioasiccf_offset	cf_loc[IOASICCF_OFFSET]		/* offset */

#define	IOASIC_OFFSET_UNKNOWN	IOASICCF_OFFSET_DEFAULT

/*
 * XXX Some drivers need direct access to IOASIC registers.
 */
extern tc_addr_t ioasic_base;


/*
 * Interrupt establishment/disestablishment functions
 */
void    ioasic_intr_establish __P((struct device *, void *, tc_intrlevel_t,
	    int (*)(void *), void *));
void    ioasic_intr_disestablish __P((struct device *, void *));


/*
 * Miscellaneous helper functions.
 */
char	*ioasic_lance_ether_address __P((void));
void	ioasic_init __P((int));
