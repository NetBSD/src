/*	$NetBSD: dz_ibus.c,v 1.23.4.1 2002/03/16 16:00:18 jdolecek Exp $ */
/*
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <dev/cons.h>

#include <machine/mtpr.h>
#include <machine/sid.h>
#include <machine/uvax.h>
#include <machine/vsbus.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/nexus.h>

#include <arch/vax/vax/gencons.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>

#include "ioconf.h"

#define DZADDR	0x25000000
#define DZVEC	0x154

static	int	dz_ibus_match(struct device *, struct cfdata *, void *);
static	void	dz_ibus_attach(struct device *, struct device *, void *);

static	vaddr_t idz_regs; /* Used for console */

struct	cfattach dz_ibus_ca = {
	sizeof(struct dz_softc), dz_ibus_match, dz_ibus_attach
};

#define REG(name)     short name; short X##name##X;
static volatile struct ss_dz {/* base address of DZ-controller: 0x200A0000 */
	REG(csr);	/* 00 Csr: control/status register */
	REG(rbuf);	/* 04 Rbuf/Lpr: receive buffer/line param reg. */
	REG(tcr);	/* 08 Tcr: transmit console register */
	REG(tdr);	/* 0C Msr/Tdr: modem status reg/transmit data reg */
	REG(lpr0);	/* 10 Lpr0: */
	REG(lpr1);	/* 14 Lpr0: */
	REG(lpr2);	/* 18 Lpr0: */
	REG(lpr3);	/* 1C Lpr0: */
} *idz;
#undef REG

cons_decl(idz);

static int
dz_ibus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct bp_conf *bp = aux;

	if (strcmp(bp->type, "dz") == 0)
		return 1;
	return 0;
}

static void
dz_ibus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct	dz_softc *sc = (void *)self;
	struct ss_dz *dzP;
	int i, vec, br;

	/* 
	 * XXX - This is evil and ugly, but...
	 * due to the nature of how bus_space_* works on VAX, this will
	 * be perfectly good until everything is converted.
	 */
	sc->sc_ioh = idz_regs;
	sc->sc_dr.dr_csr = 0;
	sc->sc_dr.dr_rbuf = 4;
	sc->sc_dr.dr_dtr = 9;
	sc->sc_dr.dr_break = 13;
	sc->sc_dr.dr_tbuf = 12;
	sc->sc_dr.dr_tcr = 8;
	sc->sc_dr.dr_dcd = 13;
	sc->sc_dr.dr_ring = 13;

	sc->sc_type = DZ_DZV;

	scb_vecref(0, 0);
	dzP = (struct ss_dz *)idz_regs;
	i = dzP->tcr;
	dzP->csr = DZ_CSR_MSE|DZ_CSR_TXIE;
	dzP->tcr = 0;
	DELAY(1000);
	dzP->tcr = 1;
	DELAY(100000);
	dzP->tcr = i;
	i = scb_vecref(&vec, &br);
	printf(": vec %o ipl %x", vec, br);
	if(i == 0 || vec == 0) printf(" -> NO INT!");

	sc->sc_dsr = 0x0f; /* XXX check if VS has modem ctrl bits */

	scb_vecalloc(DZVEC, dzxint, sc, SCB_ISTACK, &sc->sc_tintrcnt);
	scb_vecalloc(DZVEC - 4, dzrint, sc, SCB_ISTACK, &sc->sc_rintrcnt);

	printf("\n%s: 4 lines", self->dv_xname);

	dzattach(sc, NULL);
}

int
idzcngetc(dev) 
	dev_t dev;
{
	int c = 0;
	int mino = minor(dev);
	u_short rbuf;

	do {
		while ((idz->csr & 0x80) == 0)
			; /* Wait for char */
		rbuf = idz->rbuf;
		if (((rbuf >> 8) & 3) != mino)
			continue;
		c = rbuf & 0x7f;
	} while (c == 17 || c == 19);		/* ignore XON/XOFF */

	if (c == 13)
		c = 10;

	return (c);
}

#define	DZMAJOR 1

void
idzcnprobe(cndev)
	struct	consdev *cndev;
{
	extern	vaddr_t iospace;
	int diagcons;
	paddr_t ioaddr = DZADDR;

	/* not fine... but for now only KA53 is known to have dz@ibus */

	if(vax_boardtype != VAX_BTYP_53) {
		cndev->cn_pri = CN_DEAD;
		return;
	}

	diagcons = 3;
	cndev->cn_pri = CN_NORMAL;
	cndev->cn_dev = makedev(DZMAJOR, diagcons);
	idz_regs = iospace;
	ioaccess(iospace, ioaddr, 1);
}

void
idzcninit(cndev)
	struct	consdev *cndev;
{
	idz = (void*)idz_regs;

	idz->csr = 0;	 /* Disable scanning until initting is done */
	idz->tcr = (1 << minor(cndev->cn_dev));	   /* Turn on xmitter */
	idz->csr = 0x20; /* Turn scanning back on */
}


void
idzcnputc(dev,ch)
	dev_t	dev;
	int	ch;
{
	int timeout = 1<<15;		/* don't hang the machine! */
	int mino = minor(dev);
	u_short tcr;

	if (mfpr(PR_MAPEN) == 0)
		return;

	tcr = idz->tcr; /* remember which lines to scan */
	idz->tcr = (1 << mino);

	while ((idz->csr & 0x8000) == 0) /* Wait until ready */
		if (--timeout < 0)
			break;
	idz->tdr = ch;			  /* Put the  character */
	timeout = 1<<15;
	while ((idz->csr & 0x8000) == 0) /* Wait until ready */
		if (--timeout < 0)
			break;

	idz->tcr = tcr;
}

void    
idzcnpollc(dev_t dev, int pollflag)
{
}
