/*	$NetBSD: if_ie_vme.c,v 1.6 1999/03/23 12:01:45 pk Exp $	*/

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
 * 	of the chip address.   the multibus interface has its own
 * 	MMU like page map (without protections or valid bits, etc).
 * 	there are 256 pages of physical memory on the board (each page
 * 	is 1024 bytes).   There are 1024 slots in the page map.  so,
 * 	a 1024 byte page takes up 10 bits of address for the offset,
 * 	and if there are 1024 slots in the page that is another 10 bits
 * 	of the address.   That makes a 20 bit address, and as stated
 * 	earlier the board ignores the top 4 bits, so that accounts
 * 	for all 24 bits of address.
 *
 * 	Note that the last entry of the page map maps the top of the
 * 	24 bit address space and that the SCP is supposed to be at
 * 	0xfffff4 (taking into account allignment).   so,
 *	for multibus, that entry in the page map has to be used for the SCP.
 *
 * 	The page map effects BOTH how the ie chip sees the
 * 	memory, and how the host sees it.
 *
 * 	The page map is part of the "register" area of the board
 *
 *	The page map to control where ram appears in the address space.
 *	We choose to have RAM start at 0 in the 24 bit address space.
 * 
 *	to get the phyiscal address of the board's RAM you must take the
 *	top 12 bits of the physical address of the register address and
 *	or in the 4 bits from the status word as bits 17-20 (remember that
 *	the board ignores the chip's top 4 address lines). For example:
 *	if the register is @ 0xffe88000, then the top 12 bits are 0xffe00000.
 *	to get the 4 bits from the the status word just do status & IEVME_HADDR.
 *	suppose the value is "4".   Then just shift it left 16 bits to get
 *	it into bits 17-20 (e.g. 0x40000).    Then or it to get the
 *	address of RAM (in our example: 0xffe40000).   see the attach routine!
 *
 *
 *   on-board interface:
 *
 *	on the onboard ie interface the 24 bit address space is hardwired
 *	to be 0xff000000 -> 0xffffffff of KVA.   this means that sc_iobase
 *	will be 0xff000000.   sc_maddr will be where ever we allocate RAM
 *	in KVA.    note that since the SCP is at a fixed address it means
 *	that we have to allocate a fixed KVA for the SCP.
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
#include <net/if_media.h>
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

/* Supported media */
static int media[] = {
	IFM_ETHER | IFM_10_2,
};      
#define NMEDIA	(sizeof(media) / sizeof(media[0]))

/*
 * the 3E board not supported (yet?)
 */


static void ie_vmereset __P((struct ie_softc *, int));
static void ie_vmeattend __P((struct ie_softc *));
static void ie_vmerun __P((struct ie_softc *));
static int  ie_vmeintr __P((struct ie_softc *, int));

int ie_vme_match __P((struct device *, struct cfdata *, void *));
void ie_vme_attach __P((struct device *, struct device *, void *));

struct cfattach ie_vme_ca = {
	sizeof(struct ie_softc), ie_vme_match, ie_vme_attach
};


/*
 * MULTIBUS/VME support routines
 */
void
ie_vmereset(sc, what)
	struct ie_softc *sc;
	int what;
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
ie_vmeintr(sc, where)
	struct ie_softc *sc;
	int where;
{
	volatile struct ievme *iev = (volatile struct ievme *)sc->sc_reg;

	if (where != INTR_ENTER)
		return (0);

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

void ie_memcopyin __P((struct ie_softc *, void *, int, size_t));
void ie_memcopyout __P((struct ie_softc *, const void *, int, size_t));

/*
 * Copy board memory to kernel.
 */
void
ie_memcopyin(sc, p, offset, size)
	struct ie_softc	*sc;
	void *p;
	int offset;
	size_t size;
{
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/
	wcopy(addr, p, size);
}

/*
 * Copy from kernel space to naord memory.
 */
void
ie_memcopyout(sc, p, offset, size)
	struct ie_softc	*sc;
	const void *p;
	int offset;
	size_t size;
{
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/
	wcopy(p, addr, size);
}

/* read a 16-bit value at BH offset */
u_int16_t ie_vme_read16 __P((struct ie_softc *, int offset));
/* write a 16-bit value at BH offset */
void ie_vme_write16 __P((struct ie_softc *, int offset, u_int16_t value));
void ie_vme_write24 __P((struct ie_softc *, int offset, int addr));

u_int16_t
ie_vme_read16(sc, offset)
	struct ie_softc *sc;
	int offset;
{
	u_int16_t v;

	bus_space_barrier(sc->bt, sc->bh, offset, 2, BUS_SPACE_BARRIER_READ);
	v = bus_space_read_2(sc->bt, sc->bh, offset);
	return (((v&0xff)<<8) | ((v>>8)&0xff));
}

void
ie_vme_write16(sc, offset, v)
	struct ie_softc *sc;	
	int offset;
	u_int16_t v;
{
	int v0 = ((((v)&0xff)<<8) | (((v)>>8)&0xff));
	bus_space_write_2(sc->bt, sc->bh, offset, v0);
	bus_space_barrier(sc->bt, sc->bh, offset, 2, BUS_SPACE_BARRIER_WRITE);
}

void
ie_vme_write24(sc, offset, addr)
	struct ie_softc *sc;	
	int offset;
	int addr;
{
	u_char *f = (u_char *)&addr;
	u_int16_t v0, v1;
	u_char *t;

	t = (u_char *)&v0;
	t[0] = f[3]; t[1] = f[2];
	bus_space_write_2(sc->bt, sc->bh, offset, v0);

	t = (u_char *)&v1;
	t[0] = f[1]; t[1] = 0;
	bus_space_write_2(sc->bt, sc->bh, offset+2, v1);

	bus_space_barrier(sc->bt, sc->bh, offset, 4, BUS_SPACE_BARRIER_WRITE);
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
	return (vme_bus_probe(ct, bt, va->vma_reg[0], 0, 2, mod, 0, 0));
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

	sc->bt = bt;

	sc->hwreset = ie_vmereset;
	sc->hwinit = ie_vmerun;
	sc->chan_attn = ie_vmeattend;
	sc->intrhook = ie_vmeintr;
	sc->memcopyout = ie_memcopyout;
	sc->memcopyin = ie_memcopyin;
	sc->ie_bus_read16 = ie_vme_read16;
	sc->ie_bus_write16 = ie_vme_write16;
	sc->ie_bus_write24 = ie_vme_write24;
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

	sc->bh = bh;

	iev->pectrl = iev->pectrl | IEVME_PARACK; /* clear to start */

	/*
	 * Set up mappings, direct map except for last page
	 * which is mapped at zero and at high address (for scp)
	 */
	for (lcv = 0; lcv < IEVME_MAPSZ - 1; lcv++)
		iev->pgmap[lcv] = IEVME_SBORDR | IEVME_OBMEM | lcv;
	iev->pgmap[IEVME_MAPSZ - 1] = IEVME_SBORDR | IEVME_OBMEM | 0;

	/* Clear all ram */
	bus_space_set_region_2(sc->bt, sc->bh, 0, 0, sc->sc_msize/2);

	/*
	 * We use the first page to set up SCP, ICSP and SCB data
	 * structures. The remaining pages become the buffer area
	 * (managed in i82586.c).
	 * SCP is in double-mapped page, so the 586 can see it at
	 * the mandatory magic address (IE_SCP_ADDR).
	 */
	sc->scp = (IE_SCP_ADDR & (IEVME_PAGESIZE - 1));

	/* iscp at location zero */
	sc->iscp = 0;

	/* scb follows iscp */
	sc->scb = IE_ISCP_SZ;

	ie_vme_write16(sc, IE_ISCP_SCB((long)sc->iscp), sc->scb);
	ie_vme_write16(sc, IE_ISCP_BASE((u_long)sc->iscp), 0);
	ie_vme_write24(sc, IE_SCP_ISCP((u_long)sc->scp), 0);

	if (i82586_proberam(sc) == 0) {
		printf(": memory probe failed\n");
		return;
	}

	/*
	 * Rest of first page is unused; rest of ram for buffers.
	 */
	sc->buf_area = IEVME_PAGESIZE;
	sc->buf_area_sz = sc->sc_msize - IEVME_PAGESIZE;

	sc->do_xmitnopchain = 0;

	myetheraddr(myaddr);
	i82586_attach(sc, "multibus/vme", myaddr, media, NMEDIA, media[0]);

	vme_intr_map(ct, va->vma_vec, va->vma_pri, &ih);
	vme_intr_establish(ct, ih, i82586_intr, sc);

	vme_bus_establish(ct, &sc->sc_dev);
}
