/*	$NetBSD: if_ie_obio.c,v 1.3 1998/02/28 01:16:43 pk Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * Sparc OBIO front-end for the Intel 82586 Ethernet driver
 *
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <vm/vm.h>
#ifdef UVM
#include <uvm/uvm.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bus.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>

/*
 * the on-board interface
 */
struct ieob {
	u_char  obctrl;
};
#define IEOB_NORSET 0x80	/* don't reset the board */
#define IEOB_ONAIR  0x40	/* put us on the air */
#define IEOB_ATTEN  0x20	/* attention! */
#define IEOB_IENAB  0x10	/* interrupt enable */
#define IEOB_XXXXX  0x08	/* free bit */
#define IEOB_XCVRL2 0x04	/* level 2 transceiver? */
#define IEOB_BUSERR 0x02	/* bus error */
#define IEOB_INT    0x01	/* interrupt */

#define IEOB_ADBASE 0xff000000  /* KVA base addr of 24 bit address space */


static void ie_obreset __P((struct ie_softc *, int));
static void ie_obattend __P((struct ie_softc *));
static void ie_obrun __P((struct ie_softc *));

vm_map_t ie_map; /* XXX - needs change */

int ie_obio_match __P((struct device *, struct cfdata *, void *));
void ie_obio_attach __P((struct device *, struct device *, void *));

struct cfattach ie_obio_ca = {
	sizeof(struct ie_softc), ie_obio_match, ie_obio_attach
};

/* Supported media */
static int media[] = {
	IFM_ETHER | IFM_10_2,
};      
#define NMEDIA	(sizeof(media) / sizeof(media[0]))


/*
 * OBIO ie support routines
 */
void
ie_obreset(sc, what)
	struct ie_softc *sc;
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;
	ieo->obctrl = 0;
	delay(100);			/* XXX could be shorter? */
	ieo->obctrl = IEOB_NORSET;
}
void
ie_obattend(sc)
	struct ie_softc *sc;
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl |= IEOB_ATTEN;	/* flag! */
	ieo->obctrl &= ~IEOB_ATTEN;	/* down. */
}

void
ie_obrun(sc)
	struct ie_softc *sc;
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl |= (IEOB_ONAIR|IEOB_IENAB|IEOB_NORSET);
}

void ie_obio_memcopyin __P((struct ie_softc *, void *, int, size_t));
void ie_obio_memcopyout __P((struct ie_softc *, const void *, int, size_t));

/*
 * Copy board memory to kernel.
 */
void
ie_obio_memcopyin(sc, p, offset, size)
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
ie_obio_memcopyout(sc, p, offset, size)
	struct ie_softc	*sc;
	const void *p;
	int offset;
	size_t size;
{
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/
	wcopy(p, addr, size);
}

/* read a 16-bit value at BH offset */
u_int16_t ie_obio_read16 __P((struct ie_softc *, int offset));
/* write a 16-bit value at BH offset */
void ie_obio_write16 __P((struct ie_softc *, int offset, u_int16_t value));
void ie_obio_write24 __P((struct ie_softc *, int offset, int addr));

u_int16_t
ie_obio_read16(sc, offset)
	struct ie_softc *sc;
	int offset;
{
	u_int16_t v = bus_space_read_2(sc->bt, sc->bh, offset);
	return (((v&0xff)<<8) | ((v>>8)&0xff));
}

void
ie_obio_write16(sc, offset, v)
	struct ie_softc *sc;	
	int offset;
	u_int16_t v;
{
	v = (((v&0xff)<<8) | ((v>>8)&0xff));
	bus_space_write_2(sc->bt, sc->bh, offset, v);
}

void
ie_obio_write24(sc, offset, addr)
	struct ie_softc *sc;	
	int offset;
	int addr;
{
	u_int32_t v;
	u_char *t = (u_char *)&v, *f = (u_char *)&addr;

	t[0] = f[3]; t[1] = f[2]; t[2] = f[1]; t[3] = 0;
	bus_space_write_4(sc->bt, sc->bh, offset, v);
}

int
ie_obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	struct romaux *ra = &ca->ca_ra;

	if (strcmp(cf->cf_driver->cd_name, ra->ra_name)) /* correct name? */
		return (0);

	switch (ca->ca_bustype) {
	default:
		return (0);
	case BUS_OBIO:
		if (probeget(ra->ra_vaddr, 1) != -1)
			return (1);
		break;
	}
	return (0);
}

void
ie_obio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void   *aux;
{
	u_int8_t myaddr[ETHER_ADDR_LEN];
	extern void myetheraddr(u_char *);	/* should be elsewhere */
	struct ie_softc *sc = (void *) self;
	struct confargs *ca = aux;
	struct intrhand *ih;
	register struct bootpath *bp;
	volatile struct ieob *ieo;
	vm_offset_t pa;

	sc->bt = 0;

	sc->hwreset = ie_obreset;
	sc->chan_attn = ie_obattend;
	sc->hwinit = ie_obrun;
	sc->memcopyout = ie_obio_memcopyout;
	sc->memcopyin = ie_obio_memcopyin;
	sc->ie_bus_read16 = ie_obio_read16;
	sc->ie_bus_write16 = ie_obio_write16;
	sc->ie_bus_write24 = ie_obio_write24;
	sc->sc_msize = 65536; /* XXX */

	sc->sc_reg = mapiodev(ca->ca_ra.ra_reg, 0, sizeof(struct ieob));
	ieo = (volatile struct ieob *) sc->sc_reg;

	/*
	 * the rest of the IE_OBIO case needs to be cleaned up
	 * XXX-should provide bus support for 24-bit devices..
	 */

#ifdef UVM
	ie_map = uvm_map_create(pmap_kernel(), (vm_offset_t)IEOB_ADBASE,
		(vm_offset_t)IEOB_ADBASE + sc->sc_msize, TRUE);
#else
	ie_map = vm_map_create(pmap_kernel(), (vm_offset_t)IEOB_ADBASE,
		(vm_offset_t)IEOB_ADBASE + sc->sc_msize, 1);
#endif
	if (ie_map == NULL)
		panic("ie_map");

#ifdef UVM
	sc->sc_maddr = (caddr_t) uvm_km_alloc(ie_map, sc->sc_msize);
#else
	sc->sc_maddr = (caddr_t) kmem_alloc(ie_map, sc->sc_msize);
#endif
	if (sc->sc_maddr == NULL)
		panic("ie kmem_alloc");

	sc->bh = (bus_space_handle_t)(sc->sc_maddr);

	kvm_uncache(sc->sc_maddr, sc->sc_msize >> PGSHIFT);
	if (((u_long)sc->sc_maddr & ~(NBPG-1)) != (u_long)sc->sc_maddr)
		panic("unaligned dvmamalloc breaks");

	sc->sc_iobase = (caddr_t)IEOB_ADBASE; /* 24 bit base addr */
	wzero(sc->sc_maddr, sc->sc_msize);

	/* Map iscp at location zero */
	sc->iscp = (int)sc->bh;

	/* scb follows iscp */
	sc->scb = IE_ISCP_SZ;

	ie_obio_write16(sc, IE_ISCP_SCB((u_long)sc->iscp), sc->scb);
	ie_obio_write24(sc, IE_ISCP_BASE((u_long)sc->iscp),
			    (u_long)IEOB_ADBASE - (u_long)sc->bh);
	ie_obio_write24(sc, IE_SCP_ISCP((u_long)sc->scp),
			    (u_long)IEOB_ADBASE - (u_long)sc->iscp);

	/*
	 * SCP: the scp must appear at KVA IEOB_ADBASE.  The
	 * ROM seems to have page up there, but I'm not sure all
	 * ROMs will have it there.  Also, I'm not sure if that
	 * page is on some free list somewhere or not.   Let's
	 * map the first page of the buffer we just allocated
	 * to IEOB_ADBASE to be safe.
	 */

	pa = pmap_extract(pmap_kernel(), (vm_offset_t)sc->sc_maddr);
	if (pa == 0)
		panic("ie pmap_extract");

	pmap_enter(pmap_kernel(), trunc_page(IEOB_ADBASE+IE_SCP_ADDR),
	    (vm_offset_t)pa | PMAP_NC /*| PMAP_IOC*/,
	    VM_PROT_READ | VM_PROT_WRITE, 1);

	sc->scp = IEOB_ADBASE + IE_SCP_ADDR;

	/*
	 * Rest of first page is unused (wasted!); rest of ram
	 * for buffers.
	 */
	sc->buf_area = NBPG;
	sc->buf_area_sz = sc->sc_msize - NBPG;

	myetheraddr(myaddr);
	i82586_attach(sc, "onboard", myaddr, media, NMEDIA, media[0]);

	ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		panic("ie_obio: can't malloc interrupt handler");

	ih->ih_fun = i82586_intr;
	ih->ih_arg = sc;
	intr_establish(ca->ca_ra.ra_intr[0].int_pri, ih);

	bp = ca->ca_ra.ra_bp;
	if (bp != NULL && strcmp(bp->name, "ie") == 0 &&
	    sc->sc_dev.dv_unit == bp->val[1])
		bp->dev = &sc->sc_dev;
}
