/* $NetBSD: if_eh.c,v 1.3 2002/05/22 22:43:16 bjh21 Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 */
/*
 * if_eh.c -- driver for i-cubed EtherLan 100-, 200- and 500-series cards.
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: if_eh.c,v 1.3 2002/05/22 22:43:16 bjh21 Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>

#include <net/if_ether.h>

#include <machine/bswap.h>
#include <machine/bus.h>
#include <machine/bus.h>
#include <machine/irq.h>

#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define	bus_space_write_stream_2	bus_space_write_2
#define	bus_space_write_multi_stream_2	bus_space_write_multi_2
#define	bus_space_read_multi_stream_2	bus_space_read_multi_2
#endif /* __BUS_SPACE_HAS_STREAM_METHODS */

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>
#include <arch/acorn26/podulebus/if_ehreg.h>

#if BYTE_ORDER == BIG_ENDIAN
#include <machine/bswap.h>
#endif

#define EH_MEDIA_2	0
#define EH_MEDIA_T	1
#define EH_MEDIA_2_T	2
#define EH_MEDIA_FL_T	3

struct eh_softc {
	struct	dp8390_softc sc_dp;
	bus_space_tag_t		sc_datat;
	bus_space_handle_t	sc_datah;
	bus_space_tag_t		sc_ctlt;
	bus_space_handle_t	sc_ctlh;
	bus_space_tag_t		sc_ctl2t;
	bus_space_handle_t	sc_ctl2h;
	void			*sc_ih;
	struct		evcnt	sc_intrcnt;
	int			sc_flags;
#define EHF_16BIT	0x01
#define EHF_MAU		0x02
	int			sc_type;
	int			sc_mediaset;
	u_int8_t		sc_ctrl; /* Current control reg state */
};

int	eh_write_mbuf(struct dp8390_softc *, struct mbuf *, int);
int	eh_ring_copy(struct dp8390_softc *, int, caddr_t, u_short);
void	eh_read_hdr(struct dp8390_softc *, int, struct dp8390_ring *);
int	eh_test_mem(struct dp8390_softc *);

void	eh_writemem(struct eh_softc *, u_int8_t *, int, size_t);
void	eh_readmem(struct eh_softc *, int, u_int8_t *, size_t);
static void eh_init_card(struct dp8390_softc *);
static int eh_availmedia(struct eh_softc *);
/*static*/ int eh_identifymau(struct eh_softc *);

void	eh_media_init(struct dp8390_softc *);

/* if_media glue */
static int eh_mediachange(struct dp8390_softc *);
static void eh_mediastatus(struct dp8390_softc *, struct ifmediareq *);

/* autoconfiguration glue */
static int eh_match(struct device *, struct cfdata *, void *);
static void eh_attach(struct device *, struct device *, void *);

struct cfattach eh_ca = {
	sizeof(struct eh_softc), eh_match, eh_attach
};

static int
eh_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	switch (pa->pa_product) {
	case PODULE_ETHERLAN100:
	case PODULE_ETHERLAN100AEH:
	case PODULE_ETHERLAN200:
	case PODULE_ETHERLAN200AEH:
	case PODULE_ETHERLAN500:
	case PODULE_ETHERLAN500AEH:
		return 1;
	}
	return 0;
}

/* XXX 10BASE-FL on E513 */
static int media_only2[] = { IFM_ETHER | IFM_10_2 };
static int media_onlyt[] = { IFM_ETHER | IFM_10_T };
static int media_2andt[] =
    { IFM_ETHER | IFM_AUTO, IFM_ETHER | IFM_10_2, IFM_ETHER | IFM_10_T };
static const struct {
	int nmedia;
	int *media;
} media_switch[] = {
	{ 1, media_only2 },
	{ 1, media_onlyt },
	{ 3, media_2andt }
};

static void
eh_attach(struct device *parent, struct device *self, void *aux)
{
	struct podulebus_attach_args *pa = aux;
	struct eh_softc *sc = (struct eh_softc *)self;
	struct dp8390_softc *dsc = &sc->sc_dp;
	int mediaset, mautype;
	int i;
	char *ptr;
	u_int8_t *myaddr;

	/* Canonicalise card type. */
	switch (pa->pa_product) {
	case PODULE_ETHERLAN100:
	case PODULE_ETHERLAN100AEH:
		sc->sc_type = PODULE_ETHERLAN100;
		break;
	case PODULE_ETHERLAN200:
	case PODULE_ETHERLAN200AEH:
		sc->sc_type = PODULE_ETHERLAN200;
		break;
	case PODULE_ETHERLAN500:
	case PODULE_ETHERLAN500AEH:
		sc->sc_type = PODULE_ETHERLAN500;
		break;
	}

	/* Memory size and width varies. */
	dsc->mem_start = 0;
	switch (sc->sc_type) {
	case PODULE_ETHERLAN200:
		sc->sc_flags |= EHF_MAU;
		/* FALLTHROUGH */
	case PODULE_ETHERLAN100:
		printf(": 8-bit, 32 KB RAM");
		dsc->mem_size  = 0x8000;
		break;
	case PODULE_ETHERLAN500:
		printf(": 16-bit, 64 KB RAM");
		sc->sc_flags |= EHF_16BIT;
		dsc->mem_size = 0x10000;
		break;
	}

	/* Set up bus spaces */
	dsc->sc_regt = pa->pa_mod_t;
	bus_space_subregion(dsc->sc_regt, pa->pa_mod_h, EH_DP8390, 0x10,
	    &dsc->sc_regh);
	sc->sc_datat = pa->pa_mod_t;
	bus_space_subregion(sc->sc_datat, pa->pa_mod_h, EH_DATA, 1,
	    &sc->sc_datah);
	sc->sc_ctlt = pa->pa_fast_t;
	bus_space_subregion(sc->sc_ctlt, pa->pa_fast_h, EH_CTRL, 1,
	    &sc->sc_ctlh);
	sc->sc_ctl2t = pa->pa_fast_t;
	bus_space_subregion(sc->sc_ctl2t, pa->pa_fast_h, EH_CTRL2, 1,
	    &sc->sc_ctl2h);

	/* dsc->cr_proto? */
	/* dsc->rcr_proto? */

	/* Follow NE2000 driver here. */
        dsc->dcr_reg = ED_DCR_FT1 | ED_DCR_LS;
	if (sc->sc_flags & EHF_16BIT)
		dsc->dcr_reg |= ED_DCR_WTS;

	/* Set up callbacks */
	dsc->test_mem = eh_test_mem;
	dsc->init_card = eh_init_card;
	dsc->read_hdr = eh_read_hdr;
	/* dsc->recv_int */
	dsc->ring_copy = eh_ring_copy;
	dsc->write_mbuf = eh_write_mbuf;
	/* dsc->sc_enable */
	/* dsc->sc_disable */
	dsc->sc_mediachange = eh_mediachange;
	dsc->sc_mediastatus = eh_mediastatus;
	dsc->sc_media_init = eh_media_init;
	/* dsc->sc_media_fini */

	for (i = 0; i < 16; i++)
		dsc->sc_reg_map[i] = i;

	sc->sc_ctrl = 0x00;

	if (sc->sc_flags & EHF_MAU) {
		mautype = eh_identifymau(sc);
		switch (mautype) {
		case EH200_MAUID_10_2:
			printf(", 10BASE2 MAU");
			mediaset = EH_MEDIA_2;
			break;
		case EH200_MAUID_10_T:
			printf(", 10BASE-T MAU");
			mediaset = EH_MEDIA_T;
			break;
		default:
			printf(", unknown MAU id %d", mautype);
			mediaset = EH_MEDIA_2; /* XXX */
			break;
		}
	} else {
		mediaset = eh_availmedia(sc);
		switch (mediaset) {
		case EH_MEDIA_2:
			printf(", 10BASE2 only");
			break;
		case EH_MEDIA_T:
			printf(", 10BASE-T only");
			break;
		case EH_MEDIA_2_T:
			printf(", combo 10BASE2/-T");
			break;
		}
	}
	sc->sc_mediaset = mediaset;
	printf("\n");

	/* i-cubed put everything behind the loader. */
	podulebus_initloader(pa);

	/*
	 * Get the Ethernet address from the device description string.
	 * This code is stolen from if_ea.c.  It should be shared.
	 */
	myaddr = dsc->sc_enaddr;
	if (pa->pa_descr == NULL) {
		printf(": No description for Ethernet address\n");
		return;
	}
	ptr = strchr(pa->pa_descr, '(');
	if (ptr == NULL) {
		printf(": Ethernet address not found in description\n");
		return;
	}
	ptr++;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		myaddr[i] = strtoul(ptr, &ptr, 16);
		if (*ptr++ != (i == ETHER_ADDR_LEN - 1 ? ')' : ':')) {
			printf(": Bad Ethernet address found in "
			       "description\n");
			return;
		}
	}

	dp8390_config(dsc);
	dp8390_stop(dsc);

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "intr");
	sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_NET, dp8390_intr,
	    self, &sc->sc_intrcnt);
	if (bootverbose)
		printf("%s: interrupting at %s\n",
		       self->dv_xname, irq_string(sc->sc_ih));
	sc->sc_ctrl |= EH_CTRL_IE;
	bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
}

static void
eh_init_card(struct dp8390_softc *sc)
{

	eh_mediachange(sc);
}

/*
 * Write an mbuf chain to the destination NIC memory address using programmed
 * I/O.
 */
int
eh_write_mbuf(struct dp8390_softc *dsc, struct mbuf *m, int buf)
{
	struct eh_softc *sc = (struct eh_softc *)dsc;
	bus_space_tag_t nict = dsc->sc_regt;
	bus_space_handle_t nich = dsc->sc_regh;
	bus_space_tag_t datat = sc->sc_datat;
	bus_space_handle_t datah = sc->sc_datah;
	int savelen;
	int maxwait = 100;	/* about 120us */

	savelen = m->m_pkthdr.len;

	/*
	 * Set-up procedure per DP83902A data sheet:
	 *   I) Write a non-zero value into RBCR0.
	 *  II) Set bits RD2, RD1, and RD0 to 0, 0, and 1.
	 * III) Set RBCR0, 1 and RSAR0, 1.
	 *  IV) Issue the Remote Write DMA Command (RD2, RD1, RD0 = 0, 1, 0).
	 */

	/* Select page 0 registers. */

	NIC_BARRIER(nict, nich);
	bus_space_write_1(nict, nich, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	/* Write random number.  Linux writes several, follow them. */
	bus_space_write_1(nict, nich, ED_P0_RBCR0, 123);
	bus_space_write_1(nict, nich, ED_P0_RBCR1, 123);
	bus_space_write_1(nict, nich, ED_P0_RSAR0, 123);
	bus_space_write_1(nict, nich, ED_P0_RSAR1, 123);
	NIC_BARRIER(nict, nich);

	/* Linux has a 1us pause here.  Follow them. */
	DELAY(1);

	/* Select DMA read (see above) */
	bus_space_write_1(nict, nich, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	/* Reset remote DMA complete flag. */
	bus_space_write_1(nict, nich, ED_P0_ISR, ED_ISR_RDC);
	NIC_BARRIER(nict, nich);

	/* Set up DMA byte count. */
	bus_space_write_1(nict, nich, ED_P0_RBCR0, savelen);
	bus_space_write_1(nict, nich, ED_P0_RBCR1, savelen >> 8);

	/* Set up destination address in NIC mem. */
	bus_space_write_1(nict, nich, ED_P0_RSAR0, buf);
	bus_space_write_1(nict, nich, ED_P0_RSAR1, buf >> 8);

	/* Set remote DMA write. */
	NIC_BARRIER(nict, nich);
	bus_space_write_1(nict, nich,
	    ED_P0_CR, ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	/*
	 * Transfer the mbuf chain to the NIC memory.  NE2000 cards
	 * require that data be transferred as words, and only words,
	 * so that case requires some extra code to patch over odd-length
	 * mbufs.
	 */
	if ((sc->sc_flags & EHF_16BIT) == 0) {
		/* NE1000s are easy. */
		for (; m != 0; m = m->m_next)
			if (m->m_len)
				bus_space_write_multi_1(datat, datah, 0,
				    mtod(m, u_int8_t *), m->m_len);
	} else {
		/* NE2000s are a bit trickier. */
		u_int8_t *data, savebyte[2];
		int l, leftover;
#ifdef DIAGNOSTIC
		u_int8_t *lim;
#endif
		/* Start out with no leftover data. */
		leftover = 0;
		savebyte[0] = savebyte[1] = 0;

		for (; m != 0; m = m->m_next) {
			l = m->m_len;
			if (l == 0)
				continue;
			data = mtod(m, u_int8_t *);
#ifdef DIAGNOSTIC
			lim = data + l;
#endif
			while (l > 0) {
				if (leftover) {
					/*
					 * Data left over (from mbuf or
					 * realignment).  Buffer the next
					 * byte, and write it and the
					 * leftover data out.
					 */
					savebyte[1] = *data++;
					l--;
					bus_space_write_stream_2(datat, datah,
					    0,
					    *(u_int16_t *)savebyte);
					leftover = 0;
				} else if (BUS_SPACE_ALIGNED_POINTER(data,
					   u_int16_t) == 0) {
					/*
					 * Unaligned data; buffer the next
					 * byte.
					 */
					savebyte[0] = *data++;
					l--;
					leftover = 1;
				} else {
					/*
					 * Aligned data; output contiguous
					 * words as much as we can, then
					 * buffer the remaining byte, if any.
					 */
					leftover = l & 1;
					l &= ~1;
					bus_space_write_multi_stream_2(datat,
					    datah, 0,
					    (u_int16_t *)data, l >> 1);
					data += l;
					if (leftover)
						savebyte[0] = *data++;
					l = 0;
				}
			}
			if (l < 0)
				panic("eh_write_mbuf: negative len");
#ifdef DIAGNOSTIC
			if (data != lim)
				panic("eh_write_mbuf: data != lim");
#endif
		}
		if (leftover) {
			savebyte[1] = 0;
			bus_space_write_stream_2(datat, datah, 0,
			    *(u_int16_t *)savebyte);
		}
	}
	NIC_BARRIER(nict, nich);

	/*
	 * Wait for remote DMA to complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts, and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC wedging
	 * the bus.
	 */
	while (((bus_space_read_1(nict, nich, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait) {
		bus_space_read_1(nict, nich, ED_P0_CRDA1);
		bus_space_read_1(nict, nich, ED_P0_CRDA0);
		NIC_BARRIER(nict, nich);
		DELAY(1);
	}

	if (maxwait == 0) {
		log(LOG_WARNING,
		    "%s: remote transmit DMA failed to complete "
		    "(RSAR=0x%04x, RBCR=0x%04x, CRDA=0x%02x%02x)\n",
		    dsc->sc_dev.dv_xname, buf, savelen,
		    bus_space_read_1(nict, nich, ED_P0_CRDA1),
		    bus_space_read_1(nict, nich, ED_P0_CRDA0));
		dp8390_reset(dsc);
	}

	return (savelen);
}

/*
 * Given a source and destination address, copy 'amout' of a packet from
 * the ring buffer into a linear destination buffer.  Takes into account
 * ring-wrap.
 */
int
eh_ring_copy(struct dp8390_softc *dsc, int src, caddr_t dst, u_short amount)
{
	struct eh_softc *sc = (struct eh_softc *)dsc;
	u_short tmp_amount;

	/* Does copy wrap to lower addr in ring buffer? */
	if (src + amount > dsc->mem_end) {
		tmp_amount = dsc->mem_end - src;

		/* Copy amount up to end of NIC memory. */
		eh_readmem(sc, src, (u_int8_t *)dst, tmp_amount);

		amount -= tmp_amount;
		src = dsc->mem_ring;
		dst += tmp_amount;
	}

	eh_readmem(sc, src, (u_int8_t *)dst, amount);

	return (src + amount);
}

void
eh_read_hdr(struct dp8390_softc *dsc, int buf, struct dp8390_ring *hdr)
{
	struct eh_softc *sc = (struct eh_softc *)dsc;

	eh_readmem(sc, buf, (u_int8_t *)hdr, sizeof(struct dp8390_ring));
#if BYTE_ORDER == BIG_ENDIAN
	hdr->count = bswap16(hdr->count);
#endif
}

int
eh_test_mem(struct dp8390_softc *dsc)
{
	struct eh_softc *sc = (struct eh_softc *)dsc;
	u_int8_t block[256];
	int ptr, mem_end;

	mem_end = dsc->mem_start + dsc->mem_size;
	memset(block, 0, 256);
	for (ptr = dsc->mem_start; ptr < mem_end; ptr+=256)
		eh_writemem(sc, block, ptr, 256);
	return 0;
}

/*
 * Given a NIC memory source address and a host memory destination address,
 * copy 'amount' from NIC to host using programmed i/o.  The 'amount' is
 * rounded up to a word - ok as long as mbufs are word sized.
 */
void
eh_readmem(struct eh_softc *sc, int src, u_int8_t *dst, size_t amount)
{
	bus_space_tag_t nict = sc->sc_dp.sc_regt;
	bus_space_handle_t nich = sc->sc_dp.sc_regh;
	bus_space_tag_t datat = sc->sc_datat;
	bus_space_handle_t datah = sc->sc_datah;

	/* Select page 0 registers. */
	NIC_BARRIER(nict, nich);
	bus_space_write_1(nict, nich, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	/* Round up to a word if necessary. */
	if ((sc->sc_flags & EHF_16BIT) && (amount & 1))
		++amount;

	/* Set up DMA byte count. */
	bus_space_write_1(nict, nich, ED_P0_RBCR0, amount);
	bus_space_write_1(nict, nich, ED_P0_RBCR1, amount >> 8);

	/* Set up source address in NIC mem. */
	bus_space_write_1(nict, nich, ED_P0_RSAR0, src);
	bus_space_write_1(nict, nich, ED_P0_RSAR1, src >> 8);

	NIC_BARRIER(nict, nich);
	bus_space_write_1(nict, nich, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	if (sc->sc_flags & EHF_16BIT) {
#ifdef DIAGNOSTIC
		if (!ALIGNED_POINTER(dst, u_int16_t))
			panic("eh_readmem");
#endif
		bus_space_read_multi_stream_2(datat, datah, 0,
		    (u_int16_t *)dst, amount >> 1);
	} else
		bus_space_read_multi_1(datat, datah, 0, dst, amount);
}

void
eh_writemem(struct eh_softc *sc, u_int8_t *src, int dst, size_t len)
{
	bus_space_tag_t nict = sc->sc_dp.sc_regt;
	bus_space_handle_t nich = sc->sc_dp.sc_regh;
	bus_space_tag_t datat = sc->sc_datat;
	bus_space_handle_t datah = sc->sc_datah;
	int maxwait = 100;	/* about 120us */

	/*
	 * Set-up procedure per DP83902A data sheet:
	 *   I) Write a non-zero value into RBCR0.
	 *  II) Set bits RD2, RD1, and RD0 to 0, 0, and 1.
	 * III) Set RBCR0, 1 and RSAR0, 1.
	 *  IV) Issue the Remote Write DMA Command (RD2, RD1, RD0 = 0, 1, 0).
	 */

	/* Select page 0 registers. */

	NIC_BARRIER(nict, nich);
	bus_space_write_1(nict, nich, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	/* Write random number.  Linux writes several, follow them. */
	bus_space_write_1(nict, nich, ED_P0_RBCR0, 123);
	bus_space_write_1(nict, nich, ED_P0_RBCR1, 123);
	bus_space_write_1(nict, nich, ED_P0_RSAR0, 123);
	bus_space_write_1(nict, nich, ED_P0_RSAR1, 123);
	NIC_BARRIER(nict, nich);

	/* Select DMA read (see above) */
	bus_space_write_1(nict, nich, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	/* Linux has a 1us pause here.  Follow them. */
	DELAY(1);

	/* Reset remote DMA complete flag. */
	bus_space_write_1(nict, nich, ED_P0_ISR, ED_ISR_RDC);
	NIC_BARRIER(nict, nich);

	/* Set up DMA byte count. */
	bus_space_write_1(nict, nich, ED_P0_RBCR0, len);
	bus_space_write_1(nict, nich, ED_P0_RBCR1, len >> 8);

	/* Set up destination address in NIC mem. */
	bus_space_write_1(nict, nich, ED_P0_RSAR0, dst);
	bus_space_write_1(nict, nich, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_BARRIER(nict, nich);
	bus_space_write_1(nict, nich,
	    ED_P0_CR, ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);
	NIC_BARRIER(nict, nich);

	if (sc->sc_flags & EHF_16BIT) {
#ifdef DIAGNOSTIC
		if (!ALIGNED_POINTER(dst, u_int16_t))
			panic("eh_writemem");
#endif
		bus_space_write_multi_stream_2(datat, datah, 0,
		    (u_int16_t *)src, len >> 1);
	} else
		bus_space_write_multi_1(datat, datah, 0, src, len);

	/*
	 * Wait for remote DMA to complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts, and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC wedging
	 * the bus.
	 */
	while (((bus_space_read_1(nict, nich, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait)
		DELAY(1);

	if (maxwait == 0)
		printf("eh_writemem: failed to complete "
		    "(RSAR=0x%04x, RBCR=0x%04x, CRDA=0x%02x%02x)\n",
		    dst, (u_int)len,
		    bus_space_read_1(nict, nich, ED_P0_CRDA1),
		    bus_space_read_1(nict, nich, ED_P0_CRDA0));
}

/*
 * Work out the media types available on the current card.
 *
 * We try to switch to each of 10BASE2 and 10BASE-T in turn.  If the card
 * only supports one type, the media select line will be tied to select
 * that, so it won't move when we push it.
 *
 * The media select jumpers (at least on the EtherLan 100) have the same
 * effect.
 */

int
eh_availmedia(struct eh_softc *sc)
{

	/* Set the card to use AUI (10BASE2 or 10BASE-FL) */
	bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0,
	    sc->sc_ctrl & ~EH_CTRL_MEDIA);
	/* Check whether that worked */
	if ((bus_space_read_1(sc->sc_ctl2t, sc->sc_ctl2h, 0) &
	    EH_CTRL2_10B2) == 0) {
		bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
		return EH_MEDIA_T;
	}

	/* Try 10BASE-T and see if that works */
	bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0,
	    sc->sc_ctrl | EH_CTRL_MEDIA);
	if ((bus_space_read_1(sc->sc_ctl2t, sc->sc_ctl2h, 0) &
	    EH_CTRL2_10B2)) {
		bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
		return EH_MEDIA_2;
	}

	/* If both of them worked, this is a combo card. */
	bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
	return EH_MEDIA_2_T;
}

/*
 * Identify the MAU attached to an EtherLan 200-series network slot card.
 *
 * Follows the protocol described at the back of the Acorn A3020 and A4000
 * Network Interface Specification.
 */
int
eh_identifymau(struct eh_softc *sc)
{
	int id;
	bus_space_tag_t ctlt;
	bus_space_handle_t ctlh;

	ctlt = sc->sc_ctlt;
	ctlh = sc->sc_ctlh;
	/* Reset: Output 1 for a nominal 100us. */
	/* XXX For some reason, a read is necessary between writes. */
	bus_space_read_1(ctlt, ctlh, 0);
	bus_space_write_1(ctlt, ctlh, 0, EH200_CTRL_MAU);
	DELAY(200000);
	for (id = 0; id < 128; id++) {
		/* Output 0 for 10us. */
		/* XXX For some reason, a read is necessary between writes. */
		bus_space_read_1(ctlt, ctlh, 0);
		bus_space_write_1(ctlt, ctlh, 0, 0);
		DELAY(10);
		/* Read state. */
		if (bus_space_read_1(ctlt, ctlh, 0) & EH200_CTRL_MAU)
			break;
		/* Output 1 for 10us. */
		bus_space_write_1(ctlt, ctlh, 0, EH200_CTRL_MAU);
		DELAY(10);
	}
	return id;
}

void
eh_media_init(struct dp8390_softc *dsc)
{
	struct eh_softc *sc = (struct eh_softc *) dsc;
	int i;

	ifmedia_init(&dsc->sc_media, 0, dp8390_mediachange, dp8390_mediastatus);
	for (i = 0; i < media_switch[sc->sc_mediaset].nmedia; i++)
		ifmedia_add(&dsc->sc_media,
		    media_switch[sc->sc_mediaset].media[i], 0, NULL);
	ifmedia_set(&dsc->sc_media, media_switch[sc->sc_mediaset].media[0]);
}

/*
 * Medium selection has changed.
 */
int
eh_mediachange(struct dp8390_softc *dsc)
{
	struct eh_softc *sc = (struct eh_softc *)dsc;
	struct ifmedia *ifm = &dsc->sc_media;

	switch (IFM_SUBTYPE(ifm->ifm_cur->ifm_media)) {
	case IFM_AUTO:
		/* Auto-media logic from Linux */
		sc->sc_ctrl |= EH_CTRL_MEDIA;
		bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
		DELAY(1000); /* XXX Long enough for hub to respond? */
		if ((bus_space_read_1(sc->sc_ctlt, sc->sc_ctlh, 0) &
		    EH_CTRL_NOLINK) == 0)
			break;
		/* FALLTHROUGH */
	case IFM_10_2:
		sc->sc_ctrl &= ~EH_CTRL_MEDIA;
		bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
		break;
	case IFM_10_T:
		sc->sc_ctrl |= EH_CTRL_MEDIA;
		bus_space_write_1(sc->sc_ctlt, sc->sc_ctlh, 0, sc->sc_ctrl);
		break;
#ifdef DIAGNOSTIC
	default:
		panic("eh_mediachange");
#endif
	}
	return 0;
}

/*
 * Return current medium and status.
 */
void
eh_mediastatus(struct dp8390_softc *dsc, struct ifmediareq *ifmr)
{
	struct eh_softc *sc = (struct eh_softc *)dsc;
	int ctrl2;

	/* XXX 10BASE-FL on E513? */
	/* Read the actual medium currently in use. */
	ctrl2 = bus_space_read_1(sc->sc_ctl2t, sc->sc_ctl2h, 0);
	if (ctrl2 & EH_CTRL2_10B2) {
		ifmr->ifm_active = IFM_ETHER | IFM_10_2;
	} else {
		ifmr->ifm_active = IFM_ETHER | IFM_10_T;
		ifmr->ifm_status = IFM_AVALID;
		if ((bus_space_read_1(sc->sc_ctlt, sc->sc_ctlh, 0) &
		    EH_CTRL_NOLINK) == 0)
			ifmr->ifm_status |= IFM_ACTIVE;
	}

}
