/*	$NetBSD: if_ie_vme.c,v 1.1 1997/11/01 22:56:21 pk Exp $	*/

/*-
 * Copyright (c) 1995 Charles D. Cranor
 * All rights reserved.
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
 *      This product includes software developed by Charles D. Cranor.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * Converted to SUN ie driver by Charles D. Cranor,
 *		October 1994, January 1995.
 */

/*
 * The i82586 is a very painful chip, found in sun3's, sun-4/100's
 * sun-4/200's, and VME based suns.  The byte order is all wrong for a
 * SUN, making life difficult.  Programming this chip is mostly the same,
 * but certain details differ from system to system.  This driver is
 * written so that different "ie" interfaces can be controled by the same
 * driver.
 */

/*
 * programming notes:
 *
 * the ie chip operates in a 24 bit address space.
 *
 * most ie interfaces appear to be divided into two parts:
 *	 - generic 586 stuff
 *	 - board specific
 *
 * generic:
 *	the generic stuff of the ie chip is all done with data structures
 * 	that live in the chip's memory address space.   the chip expects
 * 	its main data structure (the sys conf ptr -- SCP) to be at a fixed
 * 	address in its 24 bit space: 0xfffff4
 *
 *      the SCP points to another structure called the ISCP.
 *      the ISCP points to another structure called the SCB.
 * 	the SCB has a status field, a linked list of "commands", and
 * 	a linked list of "receive buffers".   these are data structures that
 * 	live in memory, not registers.
 *
 * board:
 * 	to get the chip to do anything, you first put a command in the
 * 	command data structure list.   then you have to signal "attention"
 * 	to the chip to get it to look at the command.   how you
 * 	signal attention depends on what board you have... on PC's
 * 	there is an i/o port number to do this, on sun's there is a
 * 	register bit you toggle.
 *
 * 	to get data from the chip you program it to interrupt...
 *
 *
 * sun issues:
 *
 *      there are 3 kinds of sun "ie" interfaces:
 *        1 - a VME/multibus card
 *        2 - an on-board interface (sun3's, sun-4/100's, and sun-4/200's)
 *        3 - another VME board called the 3E
 *
 * 	the VME boards lives in vme16 space.   only 16 and 8 bit accesses
 * 	are allowed, so functions that copy data must be aware of this.
 *
 * 	the chip is an intel chip.  this means that the byte order
 * 	on all the "short"s in the chip's data structures is wrong.
 * 	so, constants described in the intel docs are swapped for the sun.
 * 	that means that any buffer pointers you give the chip must be
 * 	swapped to intel format.   yuck.
 *
 *   VME/multibus interface:
 * 	for the multibus interface the board ignores the top 4 bits
 * 	of the chip address.   the multibus interface seems to have its
 * 	own MMU like page map (without protections or valid bits, etc).
 * 	there are 256 pages of physical memory on the board (each page
 * 	is 1024 bytes).   there are 1024 slots in the page map.  so,
 * 	a 1024 byte page takes up 10 bits of address for the offset,
 * 	and if there are 1024 slots in the page that is another 10 bits
 * 	of the address.   that makes a 20 bit address, and as stated
 * 	earlier the board ignores the top 4 bits, so that accounts
 * 	for all 24 bits of address.
 *
 * 	note that the last entry of the page map maps the top of the
 * 	24 bit address space and that the SCP is supposed to be at
 * 	0xfffff4 (taking into account allignment).   so,
 *	for multibus, that entry in the page map has to be used for the SCP.
 *
 * 	the page map effects BOTH how the ie chip sees the
 * 	memory, and how the host sees it.
 *
 * 	the page map is part of the "register" area of the board
 *
 *   on-board interface:
 *
 *	<fill in useful info later>
 *
 *
 *   VME3E interface:
 *
 *	<fill in useful info later>
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <dev/vme/vmevar.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>


/*
 * VME/multibus definitions
 */
#define IEVME_PAGESIZE 1024	/* bytes */
#define IEVME_PAGSHIFT 10	/* bits */
#define IEVME_NPAGES   256	/* number of pages on chip */
#define IEVME_MAPSZ    1024	/* number of entries in the map */

/*
 * PTE for the page map
 */
#define IEVME_SBORDR 0x8000	/* sun byte order */
#define IEVME_IBORDR 0x0000	/* intel byte ordr */

#define IEVME_P2MEM  0x2000	/* memory is on P2 */
#define IEVME_OBMEM  0x0000	/* memory is on board */

#define IEVME_PGMASK 0x0fff	/* gives the physical page frame number */

struct ievme {
	u_int16_t	pgmap[IEVME_MAPSZ];
	u_int16_t	xxx[32];	/* prom */
	u_int16_t	status;		/* see below for bits */
	u_int16_t	xxx2;		/* filler */
	u_int16_t	pectrl;		/* parity control (see below) */
	u_int16_t	peaddr;		/* low 16 bits of address */
};

/*
 * status bits
 */
#define IEVME_RESET 0x8000	/* reset board */
#define IEVME_ONAIR 0x4000	/* go out of loopback 'on-air' */
#define IEVME_ATTEN 0x2000	/* attention */
#define IEVME_IENAB 0x1000	/* interrupt enable */
#define IEVME_PEINT 0x0800	/* parity error interrupt enable */
#define IEVME_PERR  0x0200	/* parity error flag */
#define IEVME_INT   0x0100	/* interrupt flag */
#define IEVME_P2EN  0x0020	/* enable p2 bus */
#define IEVME_256K  0x0010	/* 256kb rams */
#define IEVME_HADDR 0x000f	/* mask for bits 17-20 of address */

/*
 * parity control
 */
#define IEVME_PARACK 0x0100	/* parity error ack */
#define IEVME_PARSRC 0x0080	/* parity error source */
#define IEVME_PAREND 0x0040	/* which end of the data got the error */
#define IEVME_PARADR 0x000f	/* mask to get bits 17-20 of parity address */


/*
 * the 3E board not supported (yet?)
 */


static void ie_vmereset __P((struct ie_softc *));
static void ie_vmeattend __P((struct ie_softc *));
static void ie_vmerun __P((struct ie_softc *));
static int  ie_vmeintr __P((struct ie_softc *));
static caddr_t ie_align __P((caddr_t));

int ie_vme_match __P((struct device *, struct cfdata *, void *));
void ie_vme_attach __P((struct device *, struct device *, void *));

struct cfattach ie_vme_ca = {
	sizeof(struct ie_softc), ie_vme_match, ie_vme_attach
};

/*
 * MULTIBUS/VME support routines
 */
void
ie_vmereset(sc)
	struct ie_softc *sc;
{
	volatile struct ievme *iev = (struct ievme *) sc->sc_reg;
	iev->status = IEVME_RESET;
	delay(100);		/* XXX could be shorter? */
	iev->status = 0;
}

void
ie_vmeattend(sc)
	struct ie_softc *sc;
{
	volatile struct ievme *iev = (struct ievme *) sc->sc_reg;

	iev->status |= IEVME_ATTEN;	/* flag! */
	iev->status &= ~IEVME_ATTEN;	/* down. */
}

void
ie_vmerun(sc)
	struct ie_softc *sc;
{
	volatile struct ievme *iev = (struct ievme *) sc->sc_reg;

	iev->status |= (IEVME_ONAIR | IEVME_IENAB | IEVME_PEINT);
}

int
ie_vmeintr(sc)
	struct ie_softc *sc;
{
	volatile struct ievme *iev = (volatile struct ievme *)sc->sc_reg;

        /*
         * check for parity error
         */
	if (iev->status & IEVME_PERR) {
		printf("%s: parity error (ctrl 0x%x @ 0x%02x%04x)\n",
			sc->sc_dev.dv_xname, iev->pectrl,
			iev->pectrl & IEVME_HADDR, iev->peaddr);
		iev->pectrl = iev->pectrl | IEVME_PARACK;
	}
	return (0);
}

caddr_t
ie_align(ptr)
        caddr_t ptr;
{
        u_long  l = (u_long)ptr;

        l = (l + 3) & ~3L;
        return (caddr_t)l;
}

int
ie_vme_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args *va = aux;
	vme_chipset_tag_t ct = va->vma_chipset_tag;
	bus_space_tag_t bt = va->vma_bustag;
	int mod;

	mod = VMEMOD_A24 | VMEMOD_S | VMEMOD_D;
	if (vme_bus_probe(ct, bt, va->vma_reg[0], 2, mod))
		return (1);

	return (0);
}

void
ie_vme_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void   *aux;
{
	u_int8_t myaddr[ETHER_ADDR_LEN];
	extern void myetheraddr(u_char *);	/* should be elsewhere */
	struct ie_softc *sc = (void *) self;
	struct vme_attach_args *va = aux;
	vme_chipset_tag_t ct = va->vma_chipset_tag;
	bus_space_tag_t bt = va->vma_bustag;
	bus_space_handle_t bh;
	vme_intr_handle_t ih;
	volatile struct ievme *iev;
	u_long  rampaddr;
	int     lcv;
	vme_size_t sz;

	int mod;

	/*
	 * *note*: we don't detect the difference between a VME3E and
	 * a multibus/vme card.   if you want to use a 3E you'll have
	 * to fix this.
	 */
	mod = VMEMOD_A24 | VMEMOD_S | VMEMOD_D;

#if 0
	sc->dmat = va->vma_dmatag;
#endif
	sc->bt = bt;

	sc->hwreset = ie_vmereset;
	sc->hwinit = ie_vmerun;
	sc->chan_attn = ie_vmeattend;
	sc->align = ie_align;
	sc->intrhook = ie_vmeintr;
	sc->memcopy = wcopy;
	sc->memzero = wzero;
	sc->sc_msize = 4*65536;	/* XXX */

	sz = sizeof(struct ievme);
	if (vme_bus_map(ct, va->vma_reg[0], sz, mod, bt, &bh) != 0)
		panic("if_ie: vme_map");
	sc->sc_reg = (caddr_t)bh;

	iev = (volatile struct ievme *) sc->sc_reg;
	/* top 12 bits */
	rampaddr = (u_long)va->vma_reg[0] & 0xfff00000;

	/* 4 more */
	rampaddr = rampaddr | ((iev->status & IEVME_HADDR) << 16);
	sz = sc->sc_msize;
	if (vme_bus_map(ct, rampaddr, sz, mod, bt, &bh) != 0)
		panic("if_ie: vme_map");

	sc->sc_maddr = (caddr_t)bh;
	sc->sc_iobase = sc->sc_maddr;

	iev->pectrl = iev->pectrl | IEVME_PARACK; /* clear to start */

	/*
	 * Set up mappings, direct map except for last page
	 * which is mapped at zero and at high address (for scp)
	 */
	for (lcv = 0; lcv < IEVME_MAPSZ - 1; lcv++)
		iev->pgmap[lcv] = IEVME_SBORDR | IEVME_OBMEM | lcv;
	iev->pgmap[IEVME_MAPSZ - 1] = IEVME_SBORDR | IEVME_OBMEM | 0;

	/* Clear all ram */
	(sc->memzero)(sc->sc_maddr, sc->sc_msize);

	/*
	 * set up pointers to data structures and buffer area.
	 * scp is in double mapped page... get offset into page
	 * and add to sc_maddr.
	 */
	sc->scp = (volatile struct ie_sys_conf_ptr *)
		(sc->sc_maddr + (IE_SCP_ADDR & (IEVME_PAGESIZE - 1)));

	/* iscp at location zero */
	sc->iscp = (volatile struct ie_int_sys_conf_ptr *)sc->sc_maddr;

	/* scb follows iscp */
	sc->scb = (volatile struct ie_sys_ctl_block *)
		sc->sc_maddr + sizeof(struct ie_int_sys_conf_ptr);

	/*
	 * Rest of first page is unused; rest of ram for buffers.
	 */
	sc->buf_area = sc->sc_maddr + IEVME_PAGESIZE;
	sc->buf_area_sz = sc->sc_msize - IEVME_PAGESIZE;

	myetheraddr(myaddr);
	ie_attach(sc, "multibus/vme", myaddr);

	vme_intr_map(ct, va->vma_vec, va->vma_pri, &ih);
	vme_intr_establish(ct, ih, ieintr, sc);
}
