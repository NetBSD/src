/*	$NetBSD: uhavar.h,v 1.11.10.1 1999/10/19 17:47:42 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/queue.h>

#define UHA_MSCP_MAX	32	/* store up to 32 MSCPs at one time */
#define	MSCP_HASH_SIZE	32	/* hash table size for phystokv */
#define	MSCP_HASH_SHIFT	9
#define MSCP_HASH(x)	((((long)(x))>>MSCP_HASH_SHIFT) & (MSCP_HASH_SIZE - 1))

struct uha_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t	sc_dmat;
	void *sc_ih;

	int sc_dmaflags;	/* bus-specific DMA map creation flags */

	void (*start_mbox) __P((struct uha_softc *, struct uha_mscp *));
	int (*poll) __P((struct uha_softc *, struct scsipi_xfer *, int));
	void (*init) __P((struct uha_softc *));

	bus_dmamap_t sc_dmamap_mscp;	/* maps the mscps */
	struct uha_mscp *sc_mscps;	/* all our mscps */

	struct uha_mscp *sc_mscphash[MSCP_HASH_SIZE];
	TAILQ_HEAD(, uha_mscp) sc_free_mscp;
	int sc_nummscps;

	struct scsipi_adapter sc_adapter;
	struct scsipi_channel sc_channel;
};

/*
 * Offset of an MSCP from the beginning of the MSCP DMA mapping.
 */
#define	UHA_MSCP_OFF(m)	(((u_long)(m)) - ((u_long)&sc->sc_mscps[0]))

struct uha_probe_data {
	int sc_irq, sc_drq;
	int sc_scsi_dev;
};

void	uha_attach __P((struct uha_softc *, struct uha_probe_data *));
void	uha_timeout __P((void *arg));
struct	uha_mscp *uha_mscp_phys_kv __P((struct uha_softc *, u_long));
void	uha_done __P((struct uha_softc *, struct uha_mscp *));
