/*	$NetBSD: vsbus.c,v 1.16 1999/03/13 15:16:48 ragge Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/device.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/stat.h>

#include <machine/pte.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/nexus.h>

#include <machine/uvax.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/ka43.h>

#include <machine/vsbus.h>

#include "ioconf.h"

int	vsbus_match	__P((struct device *, struct cfdata *, void *));
void	vsbus_attach	__P((struct device *, struct device *, void *));
int	vsbus_print	__P((void *, const char *));
int	vsbus_search	__P((struct device *, struct cfdata *, void *));

void	ka410_attach	__P((struct device *, struct device *, void *));
void	ka43_attach	__P((struct device *, struct device *, void *));

struct	vsbus_softc {
	struct	device sc_dev;
	volatile struct vs_cpu *sc_cpu;
	u_char	sc_mask;	/* Interrupts to enable after autoconf */
};

struct	cfattach vsbus_ca = { 
	sizeof(struct vsbus_softc), vsbus_match, vsbus_attach
};

int
vsbus_print(aux, name)
	void *aux;
	const char *name;
{
	struct vsbus_attach_args *va = aux;

	printf(" csr 0x%lx vec %o ipl %x maskbit %x", va->va_paddr,
	    va->va_cvec & 511, va->va_br, va->va_maskno - 1);
	return(UNCONF); 
}

int
vsbus_match(parent, cf, aux)
	struct	device	*parent;
	struct cfdata	*cf;
	void	*aux;
{
	struct bp_conf *bp = aux;
	
	if (strcmp(bp->type, "vsbus"))
		return 0;
	/*
	 * on machines which can have it, the vsbus is always there
	 */
	if ((vax_bustype & VAX_VSBUS) == 0)
		return (0);

	return (1);
}

void
vsbus_attach(parent, self, aux)
	struct	device	*parent, *self;
	void	*aux;
{
	struct	vsbus_softc *sc = (void *)self;

	printf("\n");

	sc->sc_cpu = (void *)vax_map_physmem(VS_REGS, 1);

	/*
	 * First: find which interrupts we won't care about.
	 * There are interrupts that interrupt on a periodic basic
	 * that we don't want to interfere with the rest of the 
	 * interrupt probing.
	 */
	sc->sc_cpu->vc_intmsk = 0;
	sc->sc_cpu->vc_intclr = 0xff;
	DELAY(1000000); /* Wait a second */
	sc->sc_mask = sc->sc_cpu->vc_intreq;
	printf("%s: interrupt mask %x\n", self->dv_xname, sc->sc_mask);
	/*
	 * now check for all possible devices on this "bus"
	 */
	config_search(vsbus_search, self, NULL);

	/* Autoconfig finished, enable interrupts */
	sc->sc_cpu->vc_intmsk = ~sc->sc_mask;
}

int
vsbus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	vsbus_softc *sc = (void *)parent;
	struct	vsbus_attach_args va;
	int i, vec, br;
	u_char c;

	va.va_paddr = cf->cf_loc[0];
	va.va_addr = vax_map_physmem(va.va_paddr, 1);

	sc->sc_cpu->vc_intmsk = 0;
	sc->sc_cpu->vc_intclr = 0xff;
	scb_vecref(0, 0); /* Clear vector ref */

	i = (*cf->cf_attach->ca_match) (parent, cf, &va);
	vax_unmap_physmem(va.va_addr, 1);
	c = sc->sc_cpu->vc_intreq & ~sc->sc_mask;
	if (i > 10)
		c = sc->sc_mask; /* Fooling interrupt */
	if (c == 0 || i == 0)
		goto forgetit;

	sc->sc_cpu->vc_intmsk = c;
	DELAY(100);
	sc->sc_cpu->vc_intmsk = 0;
	va.va_maskno = ffs((u_int)c);
	i = scb_vecref(&vec, &br);
	if (i == 0)
		goto fail;
	if (vec == 0)
		goto fail;

	scb_vecalloc(vec, va.va_ivec, cf->cf_unit, SCB_ISTACK);
	va.va_br = br;
	va.va_cvec = vec;

	config_attach(parent, cf, &va, vsbus_print);
	return 0;

fail:
	printf("%s%d at %s csr %x %s\n",
	    cf->cf_driver->cd_name, cf->cf_unit, parent->dv_xname,
	    cf->cf_loc[0], (i ? "zero vector" : "didn't interrupt"));
forgetit:
	return 0;
}

/*
 * Sets a new interrupt mask. Returns the old one.
 * Works like spl functions.
 */
unsigned char
vsbus_setmask(mask)
	unsigned char mask;
{
	struct vsbus_softc *sc = vsbus_cd.cd_devs[0];
	unsigned char ch;

	ch = sc->sc_cpu->vc_intmsk;
	sc->sc_cpu->vc_intmsk = mask;
	return ch;
}

/*
 * Clears the interrupts in mask.
 */
void
vsbus_clrintr(mask)
	unsigned char mask;
{
	struct vsbus_softc *sc = vsbus_cd.cd_devs[0];

	sc->sc_cpu->vc_intclr = mask;
}

#ifdef notyet
/*
 * Allocate/free DMA pages and other bus resources.
 * VS2000: All DMA and register access must be exclusive.
 * VS3100: DMA area may be accessed by anyone anytime.
 *   MFM/SCSI: Don't touch reg's while DMA is active.
 *   SCSI/SCSI: Legal to touch any register anytime.
 */


#endif
