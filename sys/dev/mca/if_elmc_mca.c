/*	$NetBSD: if_elmc_mca.c,v 1.4.2.2 2001/04/09 01:56:46 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rafal K. Boni and Jaromir Dolecek.
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
 * 3Com 3c523 EtherLink/MC Ethernet card driver (uses i82586 Ethernet chip).
 *
 * The 3c523-specific hooks were derived from Linux driver (file
 * drivers/net/3c523.[ch]).
 *
 * This driver uses generic i82586 stuff. See also ai(4), ef(4), ix(4).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <machine/bus.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>
#include <dev/mca/mcadevs.h>
#include <dev/mca/mcavar.h>

#include <dev/mca/3c523reg.h>

struct elmc_mca_softc {
	struct ie_softc sc_ie;

	bus_space_tag_t sc_regt;	/* space tag for registers */
	bus_space_handle_t sc_regh;	/* space handle for registers */

	void		*sc_ih;		/* interrupt handle */
};

int	elmc_mca_match __P((struct device *, struct cfdata *, void *));
void	elmc_mca_attach __P((struct device *, struct device *, void *));

static void	elmc_mca_copyin __P((struct ie_softc *, void *, int, size_t));
static void	elmc_mca_copyout __P((struct ie_softc *, const void *, int, size_t));
static u_int16_t elmc_mca_read_16 __P((struct ie_softc *, int));
static void	elmc_mca_write_16 __P((struct ie_softc *, int, u_int16_t));
static void	elmc_mca_write_24 __P((struct ie_softc *, int, int));
static void	elmc_mca_attn __P((struct ie_softc *, int));
static void	elmc_mca_hwreset __P((struct ie_softc *, int));
static int	elmc_mca_intrhook __P((struct ie_softc *, int));

int
elmc_mca_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mca_attach_args *ma = aux;

	switch (ma->ma_id) {
	case MCA_PRODUCT_3C523:
		return 1;
	}

	return 0;
}

void
elmc_mca_attach(struct device *parent, struct device *self, void *aux)
{
	struct elmc_mca_softc *asc = (void *) self;
	struct ie_softc *sc = &asc->sc_ie;
	struct mca_attach_args *ma = aux;
	int pos2, pos3, i, revision;
	int iobase, irq, pbram_addr;
	bus_space_handle_t ioh, memh;
	u_int8_t myaddr[ETHER_ADDR_LEN];

	printf(" slot %d: 3Com EtherLink/MC Ethernet Adapter (3C523)\n",
		ma->ma_slot + 1);

	pos2 = mca_conf_read(ma->ma_mc, ma->ma_slot, 2);
	pos3 = mca_conf_read(ma->ma_mc, ma->ma_slot, 3);

	/*
	 * POS register 2: (adf pos0)
	 * 
	 * 7 6 5 4 3 2 1 0
	 *     \ \_/ \_/ \__ enable: 0=adapter disabled, 1=adapter enabled
	 *      \  \   \____ I/O Address Range: 00=300-307, 01=1300-1307,
	 *       \  \                           10=2300-2307, 11=3300-3307
	 *        \  \______ Packet Buffer RAM Address Range:
	 *         \            00=0x0c0000-0x0c5fff 01=0x0c8000-0x0cdfff
	 *          \           10=0x0d0000-0x0d5fff 11=0x0d8000-0x0ddfff
	 *           \______ Transceiver Type: 0=onboard(BNC) 1=ext(DIX)
	 *
	 * POS register 3: (adf pos1)
	 * 
	 * 7 6 5 4 3 2 1 0
	 *          \____/
	 *               \__ Interrupt level: 0100=3, 0010=7, 1000=9, 0001=12
	 */

	iobase = ELMC_IOADDR_BASE + (0x1000 * ((pos2 & 0x6) >> 1));

	/* get irq */
	switch (pos3 & 0x1f) {
	case 4: irq = 3; break;
	case 2: irq = 7; break;
	case 8: irq = 9; break;
	case 1: irq = 12; break;
	}

	pbram_addr = ELMC_MADDR_BASE + (((pos2 & 24) >> 3) * 0x8000);

	/* map the pio registers */
	if (bus_space_map(ma->ma_iot, iobase, ELMC_IOADDR_SIZE, 0, &ioh)) {
		printf("%s: unable to map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * 3c523 has a 24K memory. The first 16K is the shared memory, while
	 * the last 8K is for the EtherStart BIOS ROM, which we don't care
	 * about. Just use the first 16K.
	 */
	if (bus_space_map(ma->ma_memt, pbram_addr, ELMC_MADDR_SIZE, 0, &memh)) {
		printf("%s: unable to map memory space\n", sc->sc_dev.dv_xname);
		if (pbram_addr == 0xc0000) {
			printf("%s: memory space 0xc0000 may conflict with vga\n",
				sc->sc_dev.dv_xname);
		}
				
		bus_space_unmap(ma->ma_iot, ioh, ELMC_IOADDR_SIZE);
		return;
	}

	asc->sc_regt = ma->ma_iot;
	asc->sc_regh = ioh;

	sc->hwinit = NULL;
	sc->intrhook = elmc_mca_intrhook;
	sc->hwreset = elmc_mca_hwreset;
	sc->chan_attn = elmc_mca_attn;

	sc->ie_bus_barrier = NULL;

	sc->memcopyin = elmc_mca_copyin;
	sc->memcopyout = elmc_mca_copyout;
	sc->ie_bus_read16 = elmc_mca_read_16;
	sc->ie_bus_write16 = elmc_mca_write_16;
	sc->ie_bus_write24 = elmc_mca_write_24;

	sc->do_xmitnopchain = 0;

	sc->sc_mediachange = NULL;
	sc->sc_mediastatus = NULL;

	sc->bt = ma->ma_memt;
	sc->bh = memh;

	/* Map i/o space. */
	sc->sc_msize = ELMC_MADDR_SIZE;
	sc->sc_maddr = (void *)memh;
	sc->sc_iobase = (char *)sc->sc_maddr + sc->sc_msize - (1 << 24);

	/* set up pointers to important on-card control structures */
	sc->iscp = 0;
	sc->scb = IE_ISCP_SZ;
	sc->scp = sc->sc_msize + IE_SCP_ADDR - (1 << 24);

	sc->buf_area = sc->scb + IE_SCB_SZ;
	sc->buf_area_sz = sc->sc_msize - IE_ISCP_SZ - IE_SCB_SZ - IE_SCP_SZ;

	/*
	 * According to docs, we might need to read the interrupt number and
	 * write it back to the IRQ select register, since the POST might not
	 * configure the IRQ properly.
	 */
	(void) mca_conf_write(ma->ma_mc, ma->ma_slot, 3, pos3 & 0x1f);

	/* reset the card first */
	elmc_mca_hwreset(sc, CARD_RESET);
	delay(1000000 / ( 1<< 5));

	/* zero card memory */
	bus_space_set_region_1(sc->bt, sc->bh, 0, 0, sc->sc_msize);

	/* set card to 16-bit bus mode */
	bus_space_write_1(sc->bt, sc->bh, IE_SCP_BUS_USE((u_long)sc->scp), 0);

	/* set up pointers to key structures */
	elmc_mca_write_24(sc, IE_SCP_ISCP((u_long)sc->scp), (u_long) sc->iscp);
	elmc_mca_write_16(sc, IE_ISCP_SCB((u_long)sc->iscp), (u_long) sc->scb);
	elmc_mca_write_24(sc, IE_ISCP_BASE((u_long)sc->iscp), (u_long) sc->iscp);

	/* flush setup of pointers, check if chip answers */
	bus_space_barrier(sc->bt, sc->bh, 0, sc->sc_msize,
			  BUS_SPACE_BARRIER_WRITE);
	if (!i82586_proberam(sc)) {
		printf("%s: can't talk to i82586!\n", sc->sc_dev.dv_xname);

		bus_space_unmap(asc->sc_regt, asc->sc_regh, ELMC_IOADDR_SIZE);
		bus_space_unmap(sc->bt, sc->bh, ELMC_MADDR_SIZE);
		return;
	}

	/* revision is stored in the first 4 bits of the revision register */
	revision = (int) bus_space_read_1(asc->sc_regt, asc->sc_regh,
				ELMC_REVISION) & ELMC_REVISION_MASK;

	/* dump known info */
	printf("%s: rev %d, i/o %#04x-%#04x, mem %#06x-%#06x, %sternal xcvr\n",
		sc->sc_dev.dv_xname, revision,
		iobase, iobase + ELMC_IOADDR_SIZE - 1,
		pbram_addr, pbram_addr + ELMC_MADDR_SIZE - 1,
		(pos2 & 0x20) ? "ex" : "in");

	/*
	 * Hardware ethernet address is stored in the first six bytes
	 * of the IO space.
	 */
	for(i=0; i < MIN(6, ETHER_ADDR_LEN); i++)
		myaddr[i] = bus_space_read_1(asc->sc_regt, asc->sc_regh, i);

	printf("%s:", sc->sc_dev.dv_xname);
	i82586_attach((void *)sc, "3C523", myaddr, NULL, 0, 0);

	/* establish interrupt handler */
	asc->sc_ih = mca_intr_establish(ma->ma_mc, irq, IPL_NET, i82586_intr,
			sc);
	if (asc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		       sc->sc_dev.dv_xname);
	else
		printf("%s: interrupting at irq %d\n", sc->sc_dev.dv_xname,irq);

	return;
}

static void
elmc_mca_copyin (sc, dst, offset, size)
        struct ie_softc *sc;
        void *dst;
        int offset;
        size_t size;
{
	int dribble;
	u_int8_t* bptr = dst;

	bus_space_barrier(sc->bt, sc->bh, offset, size,
			  BUS_SPACE_BARRIER_READ);

	if (offset % 2) {
		*bptr = bus_space_read_1(sc->bt, sc->bh, offset);
		offset++; bptr++; size--;
	}

	dribble = size % 2;
	bus_space_read_region_2(sc->bt, sc->bh, offset, (u_int16_t *) bptr,
				size >> 1);

	if (dribble) {
		bptr += size - 1;
		offset += size - 1;
		*bptr = bus_space_read_1(sc->bt, sc->bh, offset);
	}
}

static void
elmc_mca_copyout (sc, src, offset, size)
        struct ie_softc *sc;
        const void *src;
        int offset;
        size_t size;
{
	int dribble;
	int osize = size;
	int ooffset = offset;
	const u_int8_t* bptr = src;

	if (offset % 2) {
		bus_space_write_1(sc->bt, sc->bh, offset, *bptr);
		offset++; bptr++; size--;
	}

	dribble = size % 2;
	bus_space_write_region_2(sc->bt, sc->bh, offset, (u_int16_t *)bptr,
				 size >> 1);
	if (dribble) {
		bptr += size - 1;
		offset += size - 1;
		bus_space_write_1(sc->bt, sc->bh, offset, *bptr);
	}

	bus_space_barrier(sc->bt, sc->bh, ooffset, osize,
			  BUS_SPACE_BARRIER_WRITE);
}

static u_int16_t
elmc_mca_read_16 (sc, offset)
        struct ie_softc *sc;
        int offset;
{
	bus_space_barrier(sc->bt, sc->bh, offset, 2, BUS_SPACE_BARRIER_READ);
        return bus_space_read_2(sc->bt, sc->bh, offset);
}

static void
elmc_mca_write_16 (sc, offset, value)
        struct ie_softc *sc;
        int offset;
        u_int16_t value;
{
        bus_space_write_2(sc->bt, sc->bh, offset, value);
	bus_space_barrier(sc->bt, sc->bh, offset, 2, BUS_SPACE_BARRIER_WRITE);
}

static void
elmc_mca_write_24 (sc, offset, addr)
        struct ie_softc *sc;
        int offset, addr;
{
        bus_space_write_4(sc->bt, sc->bh, offset, addr +
                                (u_long) sc->sc_maddr - (u_long) sc->sc_iobase);
	bus_space_barrier(sc->bt, sc->bh, offset, 4, BUS_SPACE_BARRIER_WRITE);
}

/*
 * Channel attention hook.
 */
static void
elmc_mca_attn(sc, why)
	struct ie_softc *sc;
	int why;
{
    struct elmc_mca_softc* asc = (struct elmc_mca_softc *) sc;
    int intr = 0;

    switch (why) {
    case CHIP_PROBE:
	intr = 0;
	break;
    case CARD_RESET:
	intr = ELMC_CTRL_INT;
	break;
    }

    bus_space_write_1(asc->sc_regt, asc->sc_regh, ELMC_CTRL,
		ELMC_CTRL_RST | ELMC_CTRL_BS3 | ELMC_CTRL_CHA | intr);
    delay(16);	/* should be > 500 ns */
    bus_space_write_1(asc->sc_regt, asc->sc_regh, ELMC_CTRL,
		ELMC_CTRL_RST | ELMC_CTRL_BS3 | intr);
}

/*
 * Do full card hardware reset. 
 */
static void
elmc_mca_hwreset(sc, why)
	struct ie_softc *sc;
	int why;
{
    struct elmc_mca_softc* asc = (struct elmc_mca_softc *) sc;
    int intr = 0;

    switch (why) {
    case CHIP_PROBE:
	intr = 0;
	break;
    case CARD_RESET:
	intr = ELMC_CTRL_INT;
	break;
    }

    /* toggle the RST bit low then high */
    bus_space_write_1(asc->sc_regt, asc->sc_regh, ELMC_CTRL,
		ELMC_CTRL_BS3 | ELMC_CTRL_LOOP);
    delay(16);	/* should be > 500 ns */
    bus_space_write_1(asc->sc_regt, asc->sc_regh, ELMC_CTRL,
		ELMC_CTRL_BS3 | ELMC_CTRL_LOOP | ELMC_CTRL_RST);

    elmc_mca_attn(sc, why);
}

/*
 * Interrupt hook.
 */
static int
elmc_mca_intrhook(sc, why)
	struct ie_softc *sc;
	int why;
{
	switch (why) {
	case INTR_ACK:
		elmc_mca_attn(sc, CHIP_PROBE);
		break;
	default:
		/* do nothing */
		break;
	}

	return (0);
}

struct cfattach elmc_mca_ca = {
	sizeof(struct elmc_mca_softc), elmc_mca_match, elmc_mca_attach
};
