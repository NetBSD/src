/*	$NetBSD: amrvar.h,v 1.1.4.2 2002/02/28 04:13:57 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef	_PCI_AMRVAR_H_
#define	_PCI_AMRVAR_H_

#include "locators.h"

#define	AMR_MAX_UNITS	16

/*
 * Logical drive information.
 */
struct amr_logdrive {
	u_int		al_size;
	u_short		al_state;
	u_short		al_properties;
};

/*
 * Per-controller state.
 */
struct amr_softc {
	/* Generic device data. */
	struct device		amr_dv;
	bus_space_tag_t		amr_iot;
	bus_space_handle_t	amr_ioh;
	bus_dma_tag_t		amr_dmat;
	bus_dmamap_t		amr_dmamap;
	void			*amr_ih;

	/* Command mailbox. */
	struct amr_mailbox	*amr_mbox;
	bus_addr_t		amr_mbox_paddr;

	/* Scatter-gather lists. */
	struct amr_sgentry	*amr_sgls;
	bus_addr_t		amr_sgls_paddr;

	/* Command management. */
	struct amr_ccb		*amr_ccbs;
	SLIST_HEAD(, amr_ccb)	amr_ccb_freelist;
	SIMPLEQ_HEAD(, amr_ccb)	amr_ccb_queue;
	int			amr_maxqueuecnt;

	/* Hardware-specific linkage. */
	int	(*amr_get_work)(struct amr_softc *, struct amr_mailbox *);
	int	(*amr_submit)(struct amr_softc *sc, struct amr_ccb *);

	/* Logical drive information. */
	int			amr_numdrives;
	struct amr_logdrive	amr_drive[AMR_MAX_UNITS];
};

/*
 * Command control block.
 */
struct amr_ccb {
	union {
		SIMPLEQ_ENTRY(amr_ccb) simpleq;
		SLIST_ENTRY(amr_ccb) slist;
	} ac_chain;

	u_int		ac_flags;
	u_int		ac_status;
	u_int		ac_ident;

	u_int		ac_xfer_size;
	bus_dmamap_t	ac_xfer_map;

	struct amr_mailbox ac_mbox;

	void		(*ac_handler)(struct amr_ccb *);
	void 		*ac_context;
	struct device	*ac_dv;
};
#define	AC_XFER_IN	0x01	/* Map describes inbound xfer */
#define	AC_XFER_OUT	0x02	/* Map describes outbound xfer */
#define	AC_COMPLETE	0x04	/* Command completed */
#define	AC_ACTIVE	0x08	/* Command active */
#define	AC_NOSGL	0x10	/* No scatter/gather list */

struct amr_attach_args {
	int		amra_unit;
};

#define	amracf_unit	cf_loc[AMRCF_UNIT]

int	amr_ccb_alloc(struct amr_softc *, struct amr_ccb **);
void	amr_ccb_enqueue(struct amr_softc *, struct amr_ccb *);
void	amr_ccb_free(struct amr_softc *, struct amr_ccb *);
int	amr_ccb_map(struct amr_softc *, struct amr_ccb *, void *, int, int);
int	amr_ccb_poll(struct amr_softc *, struct amr_ccb *, int);
void	amr_ccb_unmap(struct amr_softc *, struct amr_ccb *);

#endif	/* !_PCI_AMRVAR_H_ */
