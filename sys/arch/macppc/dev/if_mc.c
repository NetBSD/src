/*	$NetBSD: if_mc.c,v 1.28 2022/02/16 23:49:26 riastradh Exp $	*/

/*-
 * Copyright (c) 1997 David Huang <khym@bga.com>
 * All rights reserved.
 *
 * Portions of this code are based on code by Denton Gentry <denny1@home.com>
 * and Yanagisawa Takeshi <yanagisw@aa.ap.titech.ac.jp>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Bus attachment and DMA routines for the mc driver (Centris/Quadra
 * 660av and Quadra 840av onboard ethernet, based on the AMD Am79C940
 * MACE ethernet chip). Also uses the PSC (Peripheral Subsystem
 * Controller) for DMA to and from the MACE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mc.c,v 1.28 2022/02/16 23:49:26 riastradh Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/ofw/openfirm.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/pio.h>

#include <macppc/dev/am79c950reg.h>
#include <macppc/dev/if_mcvar.h>

#define MC_BUFSIZE 0x800

hide int	mc_match(device_t, cfdata_t, void *);
hide void	mc_attach(device_t, device_t, void *);
hide void	mc_init(struct mc_softc *);
hide void	mc_putpacket(struct mc_softc *, u_int);
hide int	mc_dmaintr(void *);
hide void	mc_reset_rxdma(struct mc_softc *);
hide void	mc_reset_txdma(struct mc_softc *);
hide void	mc_select_utp(struct mc_softc *);
hide void	mc_select_aui(struct mc_softc *);
hide int	mc_mediachange(struct mc_softc *);
hide void	mc_mediastatus(struct mc_softc *, struct ifmediareq *);

int mc_supmedia[] = {
	IFM_ETHER | IFM_10_T,
	IFM_ETHER | IFM_10_5,
	/*IFM_ETHER | IFM_AUTO,*/
};

#define N_SUPMEDIA __arraycount(mc_supmedia)

CFATTACH_DECL_NEW(mc, sizeof(struct mc_softc),
    mc_match, mc_attach, NULL, NULL);

hide int
mc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "mace") != 0)
		return 0;

	/* requires 6 regs */
	if (ca->ca_nreg / sizeof(int) != 6)
		return 0;

	/* requires 3 intrs */
	if (ca->ca_nintr / sizeof(int) != 3)
		return 0;

	return 1;
}

hide void
mc_attach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct mc_softc *sc = device_private(self);
	uint8_t myaddr[ETHER_ADDR_LEN];
	u_int *reg;
	char intr_xname[INTRDEVNAMEBUF];

	sc->sc_dev = self;
	sc->sc_node = ca->ca_node;
	sc->sc_regt = ca->ca_tag;

	reg  = ca->ca_reg;
	reg[0] += ca->ca_baseaddr;
	reg[2] += ca->ca_baseaddr;
	reg[4] += ca->ca_baseaddr;

	sc->sc_txdma = mapiodev(reg[2], reg[3], false);
	sc->sc_rxdma = mapiodev(reg[4], reg[5], false);
	bus_space_map(sc->sc_regt, reg[0], reg[1], 0, &sc->sc_regh);

	sc->sc_tail = 0;
	sc->sc_txdmacmd = dbdma_alloc(sizeof(dbdma_command_t) * 2, NULL);
	sc->sc_rxdmacmd = (void *)dbdma_alloc(sizeof(dbdma_command_t) * 8,
	    NULL);
	memset(sc->sc_txdmacmd, 0, sizeof(dbdma_command_t) * 2);
	memset(sc->sc_rxdmacmd, 0, sizeof(dbdma_command_t) * 8);

	printf(": irq %d,%d,%d",
		ca->ca_intr[0], ca->ca_intr[1], ca->ca_intr[2]);

	if (OF_getprop(sc->sc_node, "local-mac-address", myaddr, 6) != 6) {
		printf(": failed to get MAC address.\n");
		return;
	}

	/* Allocate memory for transmit buffer and mark it non-cacheable */
	sc->sc_txbuf = malloc(PAGE_SIZE, M_DEVBUF, M_WAITOK);
	sc->sc_txbuf_phys = kvtop(sc->sc_txbuf);
	memset(sc->sc_txbuf, 0, PAGE_SIZE);

	/*
	 * Allocate memory for receive buffer and mark it non-cacheable
	 * XXX This should use the bus_dma interface, since the buffer
	 * needs to be physically contiguous. However, it seems that
	 * at least on my system, malloc() does allocate contiguous
	 * memory. If it's not, suggest reducing the number of buffers
	 * to 2, which will fit in one 4K page.
	 */
	sc->sc_rxbuf = malloc(MC_NPAGES * PAGE_SIZE, M_DEVBUF, M_WAITOK);
	sc->sc_rxbuf_phys = kvtop(sc->sc_rxbuf);
	memset(sc->sc_rxbuf, 0, MC_NPAGES * PAGE_SIZE);

	if ((int)sc->sc_txbuf & PGOFSET)
		printf("txbuf is not page-aligned\n");
	if ((int)sc->sc_rxbuf & PGOFSET)
		printf("rxbuf is not page-aligned\n");

	sc->sc_bus_init = mc_init;
	sc->sc_putpacket = mc_putpacket;

	/* Disable receive DMA */
	dbdma_reset(sc->sc_rxdma);

	/* Disable transmit DMA */
	dbdma_reset(sc->sc_txdma);

	/* Install interrupt handlers */

	/*intr_establish(ca->ca_intr[1], IST_EDGE, IPL_NET, mc_dmaintr, sc);*/

	snprintf(intr_xname, sizeof(intr_xname), "%s dma", device_xname(self));
	intr_establish_xname(ca->ca_intr[2], IST_EDGE, IPL_NET, mc_dmaintr, sc,
	    intr_xname);

	snprintf(intr_xname, sizeof(intr_xname), "%s pio", device_xname(self));
	intr_establish_xname(ca->ca_intr[0], IST_EDGE, IPL_NET, mcintr, sc,
	    intr_xname);

	sc->sc_biucc = XMTSP_64;
	sc->sc_fifocc = XMTFW_16 | RCVFW_64 | XMTFWU | RCVFWU |
	    XMTBRST | RCVBRST;
	/*sc->sc_plscc = PORTSEL_10BT;*/
	sc->sc_plscc = PORTSEL_GPSI | ENPLSIO;

	/* mcsetup returns 1 if something fails */
	if (mcsetup(sc, myaddr)) {
		printf("mcsetup returns non zero\n");
		return;
	}
#ifdef NOTYET
	sc->sc_mediachange = mc_mediachange;
	sc->sc_mediastatus = mc_mediastatus;
	sc->sc_supmedia = mc_supmedia;
	sc->sc_nsupmedia = N_SUPMEDIA;
	sc->sc_defaultmedia = IFM_ETHER | IFM_10_T;
#endif
}

/* Bus-specific initialization */
hide void
mc_init(struct mc_softc *sc)
{
	mc_reset_rxdma(sc);
	mc_reset_txdma(sc);
}

hide void
mc_putpacket(struct mc_softc *sc, u_int len)
{
	dbdma_command_t *cmd = sc->sc_txdmacmd;

	DBDMA_BUILD(cmd, DBDMA_CMD_OUT_LAST, 0, len, sc->sc_txbuf_phys,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

	dbdma_start(sc->sc_txdma, sc->sc_txdmacmd);
}

/*
 * Interrupt handler for the MACE DMA completion interrupts
 */
int
mc_dmaintr(void *arg)
{
	struct mc_softc *sc = arg;
	int status, offset, statoff;
	int datalen, resid;
	int i, n;
	dbdma_command_t *cmd;

	/* We've received some packets from the MACE */

	/* Loop through, processing each of the packets */
	i = sc->sc_tail;
	for (n = 0; n < MC_RXDMABUFS; n++, i++) {
		if (i == MC_RXDMABUFS)
			i = 0;

		cmd = &sc->sc_rxdmacmd[i];
		/* flushcache(cmd, sizeof(dbdma_command_t)); */
		status = in16rb(&cmd->d_status);
		resid = in16rb(&cmd->d_resid);

		/*if ((status & D_ACTIVE) == 0)*/
		if ((status & 0x40) == 0)
			continue;

#if 1
		if (in16rb(&cmd->d_count) != ETHERMTU + 22)
			printf("bad d_count\n");
#endif

		datalen = in16rb(&cmd->d_count) - resid;
		datalen -= 4;	/* 4 == status bytes */

		if (datalen < 4 + sizeof(struct ether_header)) {
			printf("short packet len=%d\n", datalen);
			/* continue; */
			goto next;
		}

		offset = i * MC_BUFSIZE;
		statoff = offset + datalen;

		DBDMA_BUILD_CMD(cmd, DBDMA_CMD_STOP, 0, 0, 0, 0);
		__asm volatile("eieio" ::: "memory");

		/* flushcache(sc->sc_rxbuf + offset, datalen + 4); */

		sc->sc_rxframe.rx_rcvcnt = sc->sc_rxbuf[statoff + 0];
		sc->sc_rxframe.rx_rcvsts = sc->sc_rxbuf[statoff + 1];
		sc->sc_rxframe.rx_rntpc	 = sc->sc_rxbuf[statoff + 2];
		sc->sc_rxframe.rx_rcvcc	 = sc->sc_rxbuf[statoff + 3];
		sc->sc_rxframe.rx_frame	 = sc->sc_rxbuf + offset;

		mc_rint(sc);

next:
		DBDMA_BUILD_CMD(cmd, DBDMA_CMD_IN_LAST, 0, DBDMA_INT_ALWAYS,
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		__asm volatile("eieio" ::: "memory");
		cmd->d_status = 0;
		cmd->d_resid = 0;
		sc->sc_tail = i + 1;
	}

	dbdma_continue(sc->sc_rxdma);

	return 1;
}

hide void
mc_reset_rxdma(struct mc_softc *sc)
{
	dbdma_command_t *cmd = sc->sc_rxdmacmd;
	dbdma_regmap_t *dmareg = sc->sc_rxdma;
	int i;
	uint8_t maccc;

	/* Disable receiver, reset the DMA channels */
	maccc = NIC_GET(sc, MACE_MACCC);
	NIC_PUT(sc, MACE_MACCC, maccc & ~ENRCV);

	dbdma_reset(dmareg);

	for (i = 0; i < MC_RXDMABUFS; i++) {
		DBDMA_BUILD(cmd, DBDMA_CMD_IN_LAST, 0, ETHERMTU + 22,
			sc->sc_rxbuf_phys + MC_BUFSIZE * i, DBDMA_INT_ALWAYS,
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		cmd++;
	}

	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_ALWAYS);
	out32rb(&cmd->d_cmddep, kvtop((void *)sc->sc_rxdmacmd));
	cmd++;

	dbdma_start(dmareg, sc->sc_rxdmacmd);

	sc->sc_tail = 0;

	/* Reenable receiver, reenable DMA */
	NIC_PUT(sc, MACE_MACCC, maccc);
}

hide void
mc_reset_txdma(struct mc_softc *sc)
{
	dbdma_command_t *cmd = sc->sc_txdmacmd;
	dbdma_regmap_t *dmareg = sc->sc_txdma;
	uint8_t maccc;

	/* Disable transmitter */
	maccc = NIC_GET(sc, MACE_MACCC);
	NIC_PUT(sc, MACE_MACCC, maccc & ~ENXMT);

	dbdma_reset(dmareg);

	DBDMA_BUILD(cmd, DBDMA_CMD_OUT_LAST, 0, 0, sc->sc_txbuf_phys,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	cmd++;
	DBDMA_BUILD(cmd, DBDMA_CMD_STOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

	out32rb(&dmareg->d_cmdptrhi, 0);
	out32rb(&dmareg->d_cmdptrlo, kvtop((void *)sc->sc_txdmacmd));

	/* Restore old value */
	NIC_PUT(sc, MACE_MACCC, maccc);
}

void
mc_select_utp(struct mc_softc *sc)
{

	sc->sc_plscc = PORTSEL_GPSI | ENPLSIO;
}

void
mc_select_aui(struct mc_softc *sc)
{

	sc->sc_plscc = PORTSEL_AUI;
}

int
mc_mediachange(struct mc_softc *sc)
{
	struct ifmedia *ifm = &sc->sc_media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return EINVAL;

	switch (IFM_SUBTYPE(ifm->ifm_media)) {

	case IFM_10_T:
		mc_select_utp(sc);
		break;

	case IFM_10_5:
		mc_select_aui(sc);
		break;

	default:
		return EINVAL;
	}

	return 0;
}

void
mc_mediastatus(struct mc_softc *sc, struct ifmediareq *ifmr)
{

	if (sc->sc_plscc == PORTSEL_AUI)
		ifmr->ifm_active = IFM_ETHER | IFM_10_5;
	else
		ifmr->ifm_active = IFM_ETHER | IFM_10_T;
}
