/*	$NetBSD: twevar.h,v 1.10.2.1 2002/05/26 16:17:23 perry Exp $	*/

/*-
 * Copyright (c) 2000, 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _PCI_TWEVAR_H_
#define	_PCI_TWEVAR_H_

#include "locators.h"

#define	TWE_MAX_QUEUECNT	129

/* Per-controller state. */
struct twe_softc {
	struct device		sc_dv;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmamap;
	void			*sc_ih;
	caddr_t			sc_cmds;
	bus_addr_t		sc_cmds_paddr;
	int			sc_nccbs;
	struct twe_ccb		*sc_ccbs;
	SIMPLEQ_HEAD(, twe_ccb)	sc_ccb_queue;
	SLIST_HEAD(, twe_ccb)	sc_ccb_freelist;
	int			sc_flags;
	int			sc_nunits;
	u_int			sc_dsize[TWE_MAX_UNITS];
};
#define	TWEF_AEN	0x01	/* retrieving an AEN */

/* Optional per-command context. */
struct twe_context {
	void	(*tx_handler)(struct twe_ccb *, int);
	void 	*tx_context;
	struct	device	*tx_dv;
};

/* Command control block. */
struct twe_ccb {
	union {
		SIMPLEQ_ENTRY(twe_ccb) simpleq;
		SLIST_ENTRY(twe_ccb) slist;
	} ccb_chain;
	struct twe_cmd	*ccb_cmd;
	int		ccb_cmdid;
	int		ccb_flags;
	void		*ccb_data;
	int		ccb_datasize;
	vaddr_t		ccb_abuf;
	bus_dmamap_t	ccb_dmamap_xfer;
	struct twe_context ccb_tx;
};
#define	TWE_CCB_DATA_IN		0x01	/* Map describes inbound xfer */
#define	TWE_CCB_DATA_OUT	0x02	/* Map describes outbound xfer */
#define	TWE_CCB_COMPLETE	0x04	/* Command completed */
#define	TWE_CCB_ACTIVE		0x08	/* Command active */
#define	TWE_CCB_PARAM		0x10	/* For parameter retrieval */
#define	TWE_CCB_ALLOCED		0x20	/* CCB allocated */

struct twe_attach_args {
	int		twea_unit;
};

#define	tweacf_unit	cf_loc[TWECF_UNIT]

int	twe_ccb_alloc(struct twe_softc *, struct twe_ccb **, int);
void	twe_ccb_enqueue(struct twe_softc *sc, struct twe_ccb *ccb);
void	twe_ccb_free(struct twe_softc *sc, struct twe_ccb *);
int	twe_ccb_map(struct twe_softc *, struct twe_ccb *);
int	twe_ccb_poll(struct twe_softc *, struct twe_ccb *, int);
int	twe_ccb_submit(struct twe_softc *, struct twe_ccb *);
void	twe_ccb_unmap(struct twe_softc *, struct twe_ccb *);

static __inline__ size_t twe_get_maxsegs(void) {
	size_t max_segs = ((MAXPHYS + PAGE_SIZE - 1) / PAGE_SIZE) + 1;
#ifdef TWE_SG_SIZE
	if (TWE_SG_SIZE < max_segs)
	    max_segs = TWE_SG_SIZE;
#endif
	return max_segs;
}

static __inline__ size_t twe_get_maxxfer(size_t maxsegs) {
	return (maxsegs - 1) * PAGE_SIZE;
}

#endif	/* !_PCI_TWEVAR_H_ */
