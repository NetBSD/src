/*	$NetBSD: if_le.c,v 1.25 2001/05/31 18:46:07 scw Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <mvme68k/dev/if_lereg.h>
#include <mvme68k/dev/if_levar.h>


int le_pcc_match __P((struct device *, struct cfdata *, void *));
void le_pcc_attach __P((struct device *, struct device *, void *));

struct cfattach le_pcc_ca = {
	sizeof(struct le_softc), le_pcc_match, le_pcc_attach
};

extern struct cfdriver le_cd;

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#ifdef DDB
#define hide
#else
#define hide	static
#endif

hide void le_pcc_wrcsr __P((struct lance_softc *, u_int16_t, u_int16_t));
hide u_int16_t le_pcc_rdcsr __P((struct lance_softc *, u_int16_t));

hide void
le_pcc_wrcsr(sc, port, val)
	struct lance_softc *sc;
	u_int16_t port;
	u_int16_t val;
{
	struct le_softc *lsc;

	lsc = (struct le_softc *) sc;
	bus_space_write_2(lsc->sc_bust, lsc->sc_bush, LEPCC_RAP, port);
	bus_space_write_2(lsc->sc_bust, lsc->sc_bush, LEPCC_RDP, val);
}

hide u_int16_t
le_pcc_rdcsr(sc, port)
	struct lance_softc *sc;
	u_int16_t port;
{
	struct le_softc *lsc;

	lsc = (struct le_softc *) sc;
	bus_space_write_2(lsc->sc_bust, lsc->sc_bush, LEPCC_RAP, port);
	return (bus_space_read_2(lsc->sc_bust, lsc->sc_bush, LEPCC_RDP));
}

/* ARGSUSED */
int
le_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcc_attach_args *pa = aux;

	if (strcmp(pa->pa_name, le_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcccf_ipl;
	return (1);
}

/* ARGSUSED */
void
le_pcc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct le_softc *lsc;
	struct lance_softc *sc;
	struct pcc_attach_args *pa;
	bus_dma_segment_t seg;
	int rseg;

	lsc = (struct le_softc *) self;
	sc = &lsc->sc_am7990.lsc;
	pa = aux;

	/* Map control registers. */
	lsc->sc_bust = pa->pa_bust;
	bus_space_map(pa->pa_bust, pa->pa_offset, 4, 0, &lsc->sc_bush);

	/* Get contiguous DMA-able memory for the lance */
	if (bus_dmamem_alloc(pa->pa_dmat, ether_data_buff_size, NBPG, 0,
	    &seg, 1, &rseg,
	    BUS_DMA_NOWAIT | BUS_DMA_ONBOARD_RAM | BUS_DMA_24BIT)) {
		printf("%s: Failed to allocate ether buffer\n", self->dv_xname);
		return;
	}
	if (bus_dmamem_map(pa->pa_dmat, &seg, rseg, ether_data_buff_size,
	    (caddr_t *) & sc->sc_mem, BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) {
		printf("%s: Failed to map ether buffer\n", self->dv_xname);
		bus_dmamem_free(pa->pa_dmat, &seg, rseg);
		return;
	}
	sc->sc_addr = seg.ds_addr;
	sc->sc_memsize = ether_data_buff_size;
	sc->sc_conf3 = LE_C3_BSWP;

	memcpy(sc->sc_enaddr, mvme_ea, ETHER_ADDR_LEN);

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = le_pcc_rdcsr;
	sc->sc_wrcsr = le_pcc_wrcsr;
	sc->sc_hwinit = NULL;

	am7990_config(&lsc->sc_am7990);

	evcnt_attach_dynamic(&lsc->sc_evcnt, EVCNT_TYPE_INTR,
	    pccintr_evcnt(pa->pa_ipl), "ether", sc->sc_dev.dv_xname);

	pccintr_establish(PCCV_LE, am7990_intr, pa->pa_ipl, sc, &lsc->sc_evcnt);

	pcc_reg_write(sys_pcc, PCCREG_LANCE_INTR_CTRL,
	    pa->pa_ipl | PCC_IENABLE);
}
