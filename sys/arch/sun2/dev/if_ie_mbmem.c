/*	$NetBSD: if_ie_mbmem.c,v 1.7 2003/07/15 03:36:11 lukem Exp $	*/

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
 * The i82586 is a very painful chip, found in sun2's, sun3's, sun-4/100's
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
 *	to get the 4 bits from the status word just do status & IEMBMEM_HADDR.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ie_mbmem.c,v 1.7 2003/07/15 03:36:11 lukem Exp $");

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

#include <machine/autoconf.h>
#include <machine/idprom.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/cpu.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>

#include "locators.h"

/*
 * VME/multibus definitions
 */
#define IEMBMEM_PAGESIZE 1024	/* bytes */
#define IEMBMEM_PAGSHIFT 10	/* bits */
#define IEMBMEM_NPAGES   256	/* number of pages on chip */
#define IEMBMEM_MAPSZ    1024	/* number of entries in the map */

/*
 * PTE for the page map
 */
#define IEMBMEM_SBORDR 0x8000	/* sun byte order */
#define IEMBMEM_IBORDR 0x0000	/* intel byte ordr */

#define IEMBMEM_P2MEM  0x2000	/* memory is on P2 */
#define IEMBMEM_OBMEM  0x0000	/* memory is on board */

#define IEMBMEM_PGMASK 0x0fff	/* gives the physical page frame number */

struct iembmem {
	u_int16_t	pgmap[IEMBMEM_MAPSZ];
	u_int16_t	xxx[32];	/* prom */
	u_int16_t	status;		/* see below for bits */
	u_int16_t	xxx2;		/* filler */
	u_int16_t	pectrl;		/* parity control (see below) */
	u_int16_t	peaddr;		/* low 16 bits of address */
};

/*
 * status bits
 */
#define IEMBMEM_RESET 0x8000	/* reset board */
#define IEMBMEM_ONAIR 0x4000	/* go out of loopback 'on-air' */
#define IEMBMEM_ATTEN 0x2000	/* attention */
#define IEMBMEM_IENAB 0x1000	/* interrupt enable */
#define IEMBMEM_PEINT 0x0800	/* parity error interrupt enable */
#define IEMBMEM_PERR  0x0200	/* parity error flag */
#define IEMBMEM_INT   0x0100	/* interrupt flag */
#define IEMBMEM_P2EN  0x0020	/* enable p2 bus */
#define IEMBMEM_256K  0x0010	/* 256kb rams */
#define IEMBMEM_HADDR 0x000f	/* mask for bits 17-20 of address */

/*
 * parity control
 */
#define IEMBMEM_PARACK 0x0100	/* parity error ack */
#define IEMBMEM_PARSRC 0x0080	/* parity error source */
#define IEMBMEM_PAREND 0x0040	/* which end of the data got the error */
#define IEMBMEM_PARADR 0x000f	/* mask to get bits 17-20 of parity address */

/* Supported media */
static int media[] = {
	IFM_ETHER | IFM_10_2,
};      
#define NMEDIA	(sizeof(media) / sizeof(media[0]))

/*
 * the 3E board not supported (yet?)
 */


static void ie_mbmemreset __P((struct ie_softc *, int));
static void ie_mbmemattend __P((struct ie_softc *, int));
static void ie_mbmemrun __P((struct ie_softc *));
static int  ie_mbmemintr __P((struct ie_softc *, int));

int ie_mbmem_match __P((struct device *, struct cfdata *, void *));
void ie_mbmem_attach __P((struct device *, struct device *, void *));

struct ie_mbmem_softc {
	struct ie_softc ie;
	bus_space_tag_t ievt;
	bus_space_handle_t ievh;
};

CFATTACH_DECL(ie_mbmem, sizeof(struct ie_mbmem_softc),
    ie_mbmem_match, ie_mbmem_attach, NULL, NULL);

#define read_iev(sc, reg) \
  bus_space_read_2(sc->ievt, sc->ievh, offsetof(struct iembmem, reg))
#define write_iev(sc, reg, val) \
  bus_space_write_2(sc->ievt, sc->ievh, offsetof(struct iembmem, reg), val)

/*
 * MULTIBUS support routines
 */
void
ie_mbmemreset(sc, what)
	struct ie_softc *sc;
	int what;
{
	struct ie_mbmem_softc *vsc = (struct ie_mbmem_softc *)sc;
	write_iev(vsc, status, IEMBMEM_RESET);
	delay(100);		/* XXX could be shorter? */
	write_iev(vsc, status, 0);
}

void
ie_mbmemattend(sc, why)
	struct ie_softc *sc;
	int why;
{
	struct ie_mbmem_softc *vsc = (struct ie_mbmem_softc *)sc;

	/* flag! */
	write_iev(vsc, status, read_iev(vsc, status) | IEMBMEM_ATTEN);
	/* down. */
	write_iev(vsc, status, read_iev(vsc, status) & ~IEMBMEM_ATTEN);
}

void
ie_mbmemrun(sc)
	struct ie_softc *sc;
{
	struct ie_mbmem_softc *vsc = (struct ie_mbmem_softc *)sc;

	write_iev(vsc, status, read_iev(vsc, status)
		  | IEMBMEM_ONAIR | IEMBMEM_IENAB | IEMBMEM_PEINT);
}

int
ie_mbmemintr(sc, where)
	struct ie_softc *sc;
	int where;
{
	struct ie_mbmem_softc *vsc = (struct ie_mbmem_softc *)sc;

	if (where != INTR_ENTER)
		return (0);

        /*
         * check for parity error
         */
	if (read_iev(vsc, status) & IEMBMEM_PERR) {
		printf("%s: parity error (ctrl 0x%x @ 0x%02x%04x)\n",
		       sc->sc_dev.dv_xname, read_iev(vsc, pectrl),
		       read_iev(vsc, pectrl) & IEMBMEM_HADDR,
		       read_iev(vsc, peaddr));
		write_iev(vsc, pectrl, read_iev(vsc, pectrl) | IEMBMEM_PARACK);
	}
	return (0);
}

void ie_mbmemcopyin __P((struct ie_softc *, void *, int, size_t));
void ie_mbmemcopyout __P((struct ie_softc *, const void *, int, size_t));

/*
 * Copy board memory to kernel.
 */
void
ie_mbmemcopyin(sc, p, offset, size)
	struct ie_softc	*sc;
	void *p;
	int offset;
	size_t size;
{
	bus_space_copyin(sc->bt, sc->bh, offset, p, size);
}

/*
 * Copy from kernel space to board memory.
 */
void
ie_mbmemcopyout(sc, p, offset, size)
	struct ie_softc	*sc;
	const void *p;
	int offset;
	size_t size;
{
	bus_space_copyout(sc->bt, sc->bh, offset, p, size);
}

/* read a 16-bit value at BH offset */
u_int16_t ie_mbmem_read16 __P((struct ie_softc *, int offset));
/* write a 16-bit value at BH offset */
void ie_mbmem_write16 __P((struct ie_softc *, int offset, u_int16_t value));
void ie_mbmem_write24 __P((struct ie_softc *, int offset, int addr));

u_int16_t
ie_mbmem_read16(sc, offset)
	struct ie_softc *sc;
	int offset;
{
	u_int16_t v;

	bus_space_barrier(sc->bt, sc->bh, offset, 2, BUS_SPACE_BARRIER_READ);
	v = bus_space_read_2(sc->bt, sc->bh, offset);
	return (((v&0xff)<<8) | ((v>>8)&0xff));
}

void
ie_mbmem_write16(sc, offset, v)
	struct ie_softc *sc;	
	int offset;
	u_int16_t v;
{
	int v0 = ((((v)&0xff)<<8) | (((v)>>8)&0xff));
	bus_space_write_2(sc->bt, sc->bh, offset, v0);
	bus_space_barrier(sc->bt, sc->bh, offset, 2, BUS_SPACE_BARRIER_WRITE);
}

void
ie_mbmem_write24(sc, offset, addr)
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
ie_mbmem_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mbmem_attach_args *mbma = aux;
	bus_space_handle_t bh;
	int matched;

	/* No default Multibus address. */
	if (mbma->mbma_paddr == -1)
		return(0);

	/* Make sure there is something there... */
	if (bus_space_map(mbma->mbma_bustag, mbma->mbma_paddr, sizeof(struct iembmem), 
			  0, &bh))
		return (0);
	matched = (bus_space_peek_2(mbma->mbma_bustag, bh, 0, NULL) == 0);
	bus_space_unmap(mbma->mbma_bustag, bh, sizeof(struct iembmem));
	if (!matched)
		return (0);

	/* Default interrupt priority. */
	if (mbma->mbma_pri == -1)
		mbma->mbma_pri = 3;

	return (1);
}

void
ie_mbmem_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void   *aux;
{
	u_int8_t myaddr[ETHER_ADDR_LEN];
	struct ie_mbmem_softc *vsc = (void *) self;
	struct mbmem_attach_args *mbma = aux;
	struct ie_softc *sc;
	bus_size_t memsize;
	bus_addr_t rampaddr;
	int lcv;

	sc = &vsc->ie;

	sc->hwreset = ie_mbmemreset;
	sc->hwinit = ie_mbmemrun;
	sc->chan_attn = ie_mbmemattend;
	sc->intrhook = ie_mbmemintr;
	sc->memcopyout = ie_mbmemcopyout;
	sc->memcopyin = ie_mbmemcopyin;

	sc->ie_bus_barrier = NULL;
	sc->ie_bus_read16 = ie_mbmem_read16;
	sc->ie_bus_write16 = ie_mbmem_write16;
	sc->ie_bus_write24 = ie_mbmem_write24;

	/*
	 * There is 64K of memory on the Multibus board.
	 * (determined by hardware - NOT configurable!)
	 */
	memsize = 0x10000; /* MEMSIZE 64K */

	/* Map in the board control regs. */
	vsc->ievt = mbma->mbma_bustag;
	if (bus_space_map(mbma->mbma_bustag, mbma->mbma_paddr, sizeof(struct iembmem), 
			  0, &vsc->ievh))
		panic("ie_mbmem_attach: can't map regs");

	/*
	 * Find and map in the board memory.
	 */
	/* top 12 bits */
	rampaddr = mbma->mbma_paddr & 0xfff00000;
	/* 4 more */
	rampaddr = rampaddr | ((read_iev(vsc, status) & IEMBMEM_HADDR) << 16);
	sc->bt = mbma->mbma_bustag;
	if (bus_space_map(mbma->mbma_bustag, rampaddr, memsize, 0, &sc->bh))
		panic("ie_mbmem_attach: can't map mem");

	write_iev(vsc, pectrl, read_iev(vsc, pectrl) | IEMBMEM_PARACK);

	/*
	 * Set up mappings, direct map except for last page
	 * which is mapped at zero and at high address (for scp)
	 */
	for (lcv = 0; lcv < IEMBMEM_MAPSZ - 1; lcv++)
		write_iev(vsc, pgmap[lcv], IEMBMEM_SBORDR | IEMBMEM_OBMEM | lcv);
	write_iev(vsc, pgmap[IEMBMEM_MAPSZ - 1], IEMBMEM_SBORDR | IEMBMEM_OBMEM | 0);

	/* Clear all ram */
	bus_space_set_region_2(sc->bt, sc->bh, 0, 0, memsize/2);

	/*
	 * We use the first page to set up SCP, ICSP and SCB data
	 * structures. The remaining pages become the buffer area
	 * (managed in i82586.c).
	 * SCP is in double-mapped page, so the 586 can see it at
	 * the mandatory magic address (IE_SCP_ADDR).
	 */
	sc->scp = (IE_SCP_ADDR & (IEMBMEM_PAGESIZE - 1));

	/* iscp at location zero */
	sc->iscp = 0;

	/* scb follows iscp */
	sc->scb = IE_ISCP_SZ;

	ie_mbmem_write16(sc, IE_ISCP_SCB((long)sc->iscp), sc->scb);
	ie_mbmem_write16(sc, IE_ISCP_BASE((u_long)sc->iscp), 0);
	ie_mbmem_write24(sc, IE_SCP_ISCP((u_long)sc->scp), 0);

	if (i82586_proberam(sc) == 0) {
		printf(": memory probe failed\n");
		return;
	}

	/*
	 * Rest of first page is unused; rest of ram for buffers.
	 */
	sc->buf_area = IEMBMEM_PAGESIZE;
	sc->buf_area_sz = memsize - IEMBMEM_PAGESIZE;

	sc->do_xmitnopchain = 0;

	printf("\n%s:", self->dv_xname);

	/* Set the ethernet address. */
	idprom_etheraddr(myaddr);

	i82586_attach(sc, "multibus", myaddr, media, NMEDIA, media[0]);

	bus_intr_establish(mbma->mbma_bustag, mbma->mbma_pri, IPL_NET, 0,
			   i82586_intr, sc);
}
