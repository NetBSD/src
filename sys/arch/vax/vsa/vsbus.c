/*	$NetBSD: vsbus.c,v 1.11 1998/06/04 15:51:12 ragge Exp $ */
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
#include <machine/ka43.h>

#include <machine/vsbus.h>

#include "ioconf.h"

int	vsbus_match	__P((struct device *, struct cfdata *, void *));
void	vsbus_attach	__P((struct device *, struct device *, void *));
int	vsbus_print	__P((void *, const char *));

void	ka410_attach	__P((struct device *, struct device *, void *));
void	ka43_attach	__P((struct device *, struct device *, void *));

#define VSBUS_MAXINTR	8

struct	vsbus_softc {
	struct	device sc_dev;
	struct	ivec_dsp sc_dsp[VSBUS_MAXINTR];
};

struct	cfattach vsbus_ca = { 
	sizeof(struct vsbus_softc), vsbus_match, vsbus_attach
};

void	vsbus_intr_setup __P((struct vsbus_softc *));

#define VSBUS_MAXDEVS	8

int
vsbus_print(aux, name)
	void *aux;
	const char *name;
{
	struct vsbus_attach_args *va = aux;

	if (name) {
		printf ("device %d at %s", va->va_type, name);
		return (UNSUPP);
	}
	return (UNCONF); 
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
	struct	vsbus_attach_args va;

	printf("\n");
	/*
	 * first setup interrupt-table, so that devices can register
	 * their interrupt-routines...
	 */
	vsbus_intr_setup(sc);

	/*
	 * now check for all possible devices on this "bus"
	 */
	/* Always have network */
	va.va_type = INR_NP;
	config_found(self, &va, vsbus_print);

	/* Always have serial line */
	va.va_type = INR_SR;
	config_found(self, &va, vsbus_print);

	/* If sm_addr is set, a monochrome graphics adapter is found */
	if (sm_addr) {
		va.va_type = INR_VF;
		config_found(self, &va, vsbus_print);
	}
}

static	void stray __P((int));

static void
stray(arg)
	int arg;
{
	printf("stray interrupt nr %d.\n", arg);
}

static int inrs[] = {IVEC_DC, IVEC_SC, IVEC_VS, IVEC_VF,
		IVEC_NS, IVEC_NP, IVEC_ST, IVEC_SR};

void
vsbus_intr_setup(sc)
	struct vsbus_softc *sc;
{
	extern struct ivec_dsp idsptch;		/* subr.s */
	void **scbP = (void*)scb;
	int i;

	vs_cpu->vc_intmsk = 0;		/* disable all interrupts */
	vs_cpu->vc_intclr = 0xFF;	/* clear all old interrupts */

	for (i = 0; i < VSBUS_MAXINTR; i++) {
		bcopy(&idsptch, &sc->sc_dsp[i], sizeof(struct ivec_dsp));
		sc->sc_dsp[i].hoppaddr = stray;
		sc->sc_dsp[i].pushlarg = i;
		scbP[inrs[i]/4] = &sc->sc_dsp[i];
	}
}

void
vsbus_intr_attach(nr, func, arg)
	int nr;
	void (*func)(int);
	int arg;
{
	struct vsbus_softc *sc = vsbus_cd.cd_devs[0];

	sc->sc_dsp[nr].hoppaddr = func;
	sc->sc_dsp[nr].pushlarg = arg;
}

void
vsbus_intr_enable(nr)
	int nr;
{
	vs_cpu->vc_intclr = (1<<nr);
	vs_cpu->vc_intmsk |= (1<<nr);
}

void
vsbus_intr_disable(nr)
	int nr;
{
	vs_cpu->vc_intmsk = vs_cpu->vc_intmsk & ~(1<<nr);
}

/*
 *
 *
 */

static volatile struct dma_lock {
	int	dl_locked;
	int	dl_wanted;
	void	*dl_owner;
	int	dl_count;
} dmalock = { 0, 0, NULL, 0 };

int
vsbus_lockDMA(ca)
	struct confargs *ca;
{
	while (dmalock.dl_locked) {
		dmalock.dl_wanted++;
		sleep((caddr_t)&dmalock, PRIBIO);	/* PLOCK or PRIBIO ? */
		dmalock.dl_wanted--;
	}
	dmalock.dl_locked++;
	dmalock.dl_owner = ca;

	/*
	 * no checks yet, no timeouts, nothing...
	 */

#ifdef DEBUG
	if ((++dmalock.dl_count % 1000) == 0)
		printf("%d locks, owner: %s\n", dmalock.dl_count, ca->ca_name);
#endif
	return (0);
}

int 
vsbus_unlockDMA(ca)
	struct confargs *ca;
{
	if (dmalock.dl_locked != 1 || dmalock.dl_owner != ca) {
		printf("locking-problem: %d, %s\n", dmalock.dl_locked,
		       (char *)(dmalock.dl_owner ? dmalock.dl_owner : "null"));
		dmalock.dl_locked = 0;
		return (-1);
	}
	dmalock.dl_owner = NULL;
	dmalock.dl_locked = 0;
	if (dmalock.dl_wanted) {
		wakeup((caddr_t)&dmalock);
	}
	return (0);
}
