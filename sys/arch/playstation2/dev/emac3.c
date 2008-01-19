/*	$NetBSD: emac3.c,v 1.6 2008/01/19 22:10:15 dyoung Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * EMAC3 (Ethernet Media Access Controller)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: emac3.c,v 1.6 2008/01/19 22:10:15 dyoung Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/dev/emac3reg.h>
#include <playstation2/dev/emac3var.h>

#ifdef EMAC3_DEBUG
#define STATIC
int	emac3_debug = 0;
#define	DPRINTF(fmt, args...)						\
	if (emac3_debug)						\
		printf("%s: " fmt, __func__ , ##args) 
#define	DPRINTFN(n, arg)						\
	if (emac3_debug > (n))						\
n		printf("%s: " fmt, __func__ , ##args) 
#else
#define STATIC			static
#define	DPRINTF(arg...)		((void)0)
#define DPRINTFN(n, arg...)	((void)0)
#endif

/* SMAP specific EMAC3 define */
#define EMAC3_BASE			MIPS_PHYS_TO_KSEG1(0x14002000)
static inline u_int32_t
_emac3_reg_read_4(int ofs)
{
	bus_addr_t a_ = EMAC3_BASE + ofs;

	return (_reg_read_2(a_) << 16) | _reg_read_2(a_ + 2);
}

static inline void
_emac3_reg_write_4(int ofs, u_int32_t v)
{
	bus_addr_t a_ = EMAC3_BASE + ofs;

	_reg_write_2(a_, (v >> 16) & 0xffff);
	_reg_write_2(a_ + 2, v & 0xffff);
}

STATIC int emac3_phy_ready(void);
STATIC int emac3_soft_reset(void);
STATIC void emac3_config(const u_int8_t *);

int
emac3_init(struct emac3_softc *sc)
{
	u_int32_t r;

	/* save current mode before reset */
	r = _emac3_reg_read_4(EMAC3_MR1);

	if (emac3_soft_reset() != 0) {
		printf("%s: reset failed.\n", sc->dev.dv_xname);
		return (1);
	}

	/* set operation mode */
	r |= MR1_RFS_2KB | MR1_TFS_1KB | MR1_TR0_SINGLE | MR1_TR1_SINGLE;
	_emac3_reg_write_4(EMAC3_MR1, r);
	    
	sc->mode1_reg = _emac3_reg_read_4(EMAC3_MR1);

	emac3_intr_clear();
	emac3_intr_disable();

	emac3_config(sc->eaddr);

	return (0);
}

void
emac3_exit(struct emac3_softc *sc)
{
	int retry = 10000;
	
	/* wait for kicked transmission */
	while (((_emac3_reg_read_4(EMAC3_TMR0) & TMR0_GNP0) != 0) &&
	    --retry > 0)
		;

	if (retry == 0)
		printf("%s: still running.\n", sc->dev.dv_xname);
}

int
emac3_reset(struct emac3_softc *sc)
{

	if (emac3_soft_reset() != 0) {
		printf("%s: reset failed.\n", sc->dev.dv_xname);
		return (1);
	}

	/* restore previous mode */
	_emac3_reg_write_4(EMAC3_MR1, sc->mode1_reg);

	emac3_config(sc->eaddr);

	return (0);
}

void
emac3_enable()
{

	_emac3_reg_write_4(EMAC3_MR0, MR0_TXE | MR0_RXE);
}

void
emac3_disable()
{
	int retry = 10000;

	_emac3_reg_write_4(EMAC3_MR0,
	    _emac3_reg_read_4(EMAC3_MR0) & ~(MR0_TXE | MR0_RXE));

	/* wait for idling state */
	while (((_emac3_reg_read_4(EMAC3_MR0) & (MR0_RXI | MR0_TXI)) !=
	    (MR0_RXI | MR0_TXI)) && --retry > 0)
		;

	if (retry == 0)
		printf("emac3 running.\n");
}

void
emac3_intr_enable()
{

	_emac3_reg_write_4(EMAC3_ISER, ~0);
}

void
emac3_intr_disable()
{

	_emac3_reg_write_4(EMAC3_ISER, 0);
}

void
emac3_intr_clear()
{

	_emac3_reg_write_4(EMAC3_ISR, _emac3_reg_read_4(EMAC3_ISR));
}

int
emac3_intr(void *arg)
{
	u_int32_t r = _emac3_reg_read_4(EMAC3_ISR);

	DPRINTF("%08x\n", r);
	_emac3_reg_write_4(EMAC3_ISR, r);

	return (1);
}

void
emac3_tx_kick()
{
	
	_emac3_reg_write_4(EMAC3_TMR0, TMR0_GNP0);
}

int
emac3_tx_done()
{

	return (_emac3_reg_read_4(EMAC3_TMR0) & TMR0_GNP0);
}

void
emac3_setmulti(struct emac3_softc *sc, struct ethercom *ec)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	struct ifnet *ifp = &ec->ec_if;
	u_int32_t r;

	r = _emac3_reg_read_4(EMAC3_RMR);
	r &= ~(RMR_PME | RMR_PMME | RMR_MIAE);

	if (ifp->if_flags & IFF_PROMISC) {
allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		r |= RMR_PME;
		_emac3_reg_write_4(EMAC3_RMR, r);

		return;
	}

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0)
			goto allmulti;

		ETHER_NEXT_MULTI(step, enm)
	}

	/* XXX always multicast promiscuous mode. XXX use hash table.. */
	ifp->if_flags |= IFF_ALLMULTI;
	r |= RMR_PMME; 
	_emac3_reg_write_4(EMAC3_RMR, r);
}

int
emac3_soft_reset()
{
	int retry = 10000;

	_emac3_reg_write_4(EMAC3_MR0, MR0_SRST);

	while ((_emac3_reg_read_4(EMAC3_MR0) & MR0_SRST) == MR0_SRST &&
	    --retry > 0)
		;

	return (retry == 0);
}

void
emac3_config(const u_int8_t *eaddr)
{

	/* set ethernet address */
	_emac3_reg_write_4(EMAC3_IAHR, (eaddr[0] << 8) | eaddr[1]);
	_emac3_reg_write_4(EMAC3_IALR, (eaddr[2] << 24) | (eaddr[3] << 16) |
	    (eaddr[4] << 8) | eaddr[5]);

	/* inter-frame GAP */
	_emac3_reg_write_4(EMAC3_IPGVR, 4);

	/* RX mode */
	_emac3_reg_write_4(EMAC3_RMR,
	    RMR_SP |	/* strip padding */
	    RMR_SFCS |	/* strip FCS */
	    RMR_IAE |	/* individual address enable */
	    RMR_BAE);	/* boradcast address enable */

	/* TX mode */
	_emac3_reg_write_4(EMAC3_TMR1, 
	    ((7 << TMR1_TLR_SHIFT) & TMR1_TLR_MASK) | /* 16 word burst */
	    ((15 << TMR1_TUR_SHIFT) & TMR1_TUR_MASK));
	
	/* TX threshold */
	_emac3_reg_write_4(EMAC3_TRTR,
	    (12 << TRTR_SHIFT) & TRTR_MASK); /* 832 bytes */

	/* RX watermark */
	_emac3_reg_write_4(EMAC3_RWMR,
	    ((16 << RWMR_RLWM_SHIFT) & RWMR_RLWM_MASK) |
	    ((128 << RWMR_RHWM_SHIFT) & RWMR_RHWM_MASK));
}

/*
 * PHY/MII
 */
void
emac3_phy_writereg(struct device *self, int phy, int reg, int data)
{

	if (emac3_phy_ready() != 0)
		return;

	_emac3_reg_write_4(EMAC3_STACR, STACR_WRITE |
	    ((phy << STACR_PCDASHIFT) & STACR_PCDA)  | /* command dest addr*/
	    ((reg << STACR_PRASHIFT) & STACR_PRA) |   /* register addr */
	    ((data << STACR_PHYDSHIFT) & STACR_PHYD)); /* data */

	if (emac3_phy_ready() != 0)
		return;
}

int
emac3_phy_readreg(struct device *self, int phy, int reg)
{

	if (emac3_phy_ready() != 0)
		return (0);

	_emac3_reg_write_4(EMAC3_STACR, STACR_READ |
	    ((phy << STACR_PCDASHIFT) & STACR_PCDA)  | /* command dest addr*/
	    ((reg << STACR_PRASHIFT) & STACR_PRA));   /* register addr */

	if (emac3_phy_ready() != 0)
		return (0);

	return ((_emac3_reg_read_4(EMAC3_STACR) >> STACR_PHYDSHIFT) & 0xffff);
}

void
emac3_phy_statchg(struct device *dev)
{
#define EMAC3_FDX	(MR1_FDE | MR1_EIFC | MR1_APP)
	struct emac3_softc *sc = (void *)dev;
	int media;
	u_int32_t r;
	
	media = sc->mii.mii_media_active;

	r = _emac3_reg_read_4(EMAC3_MR1);

	r &= ~(MR1_MF_MASK | MR1_IST | EMAC3_FDX);

	switch (media & 0x1f) {
	default:
		printf("unknown media type. %08x", media);
		/* FALLTHROUGH */
	case IFM_100_TX:
		r |= (MR1_MF_100MBS | MR1_IST);
		if (media & IFM_FDX)
			r |= EMAC3_FDX;

		break;
	case IFM_10_T:
		r |= MR1_MF_10MBS;
		if (media & IFM_FDX)
			r |= (EMAC3_FDX | MR1_IST);
		break;
	}
	
	_emac3_reg_write_4(EMAC3_MR1, r);

	/* store current state for re-initialize */
	sc->mode1_reg = _emac3_reg_read_4(EMAC3_MR1);
#undef EMAC3_FDX
}

int
emac3_phy_ready()
{
	int retry = 10000;

	while ((_emac3_reg_read_4(EMAC3_STACR) & STACR_OC) == 0 &&
	    --retry > 0)
		;
	if (retry == 0) {
		printf("emac3: phy busy.\n");
		return (1);
	}

	return (0);
}

