/*	$NetBSD: dz_vsbus.c,v 1.7 1999/02/02 18:37:21 ragge Exp $ */
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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

#include <machine/../vax/gencons.h>

#include <vax/uba/dzreg.h>
#include <vax/uba/dzvar.h>

#include "ioconf.h"
#include "lkc.h"

static  int     dz_vsbus_match __P((struct device *, struct cfdata *, void *));
static  void    dz_vsbus_attach __P((struct device *, struct device *, void *));
static	int	dz_print __P((void *, const char *));
static  void    txon __P((void));
static  void    rxon __P((void));

static	vaddr_t dz_regs; /* Used for console */

struct  cfattach dz_vsbus_ca = {
        sizeof(struct dz_softc), dz_vsbus_match, dz_vsbus_attach
};

static void
rxon()
{
        vsbus_intr_enable(inr_sr);
}

static void
txon()
{
        vsbus_intr_enable(inr_st);
}

int
dz_print(aux, name)
	void *aux;
	const char *name;
{
	if (name)
		printf ("lkc at %s", name);
	return (UNCONF);
}

static int
dz_vsbus_match(parent, cf, aux)
        struct device *parent;
        struct cfdata *cf;
        void *aux;
{
        struct vsbus_attach_args *va = aux;
        if (va->va_type == inr_sr)
                return 1;
        return 0;
}

static void
dz_vsbus_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
        struct  dz_softc *sc = (void *)self;

        sc->sc_dr.dr_csr = (void *)(dz_regs + 0);
        sc->sc_dr.dr_rbuf = (void *)(dz_regs + 4);
        sc->sc_dr.dr_dtr = (void *)(dz_regs + 9);
        sc->sc_dr.dr_break = (void *)(dz_regs + 13);
        sc->sc_dr.dr_tbuf = (void *)(dz_regs + 12);
        sc->sc_dr.dr_tcr = (void *)(dz_regs + 8);
        sc->sc_dr.dr_dcd = (void *)(dz_regs + 13);
        sc->sc_dr.dr_ring = (void *)(dz_regs + 13);

        sc->sc_type = DZ_DZV;

        sc->sc_txon = txon;
        sc->sc_rxon = rxon;
	sc->sc_dsr = 0x0f; /* XXX check if VS has modem ctrl bits */
        vsbus_intr_attach(inr_sr, dzrint, 0);
        vsbus_intr_attach(inr_st, dzxint, 0);
        printf(": DC367");

        dzattach(sc);

	if ((vax_confdata & 0x80) == 0) /* workstation, have lkc */
		config_found(self, 0, dz_print);
}

/*----------------------------------------------------------------------*/

#define REG(name)     short name; short X##name##X;
static volatile struct {/* base address of DZ-controller: 0x200A0000 */
  REG(csr);           /* 00 Csr: control/status register */
  REG(rbuf);          /* 04 Rbuf/Lpr: receive buffer/line param reg. */
  REG(tcr);           /* 08 Tcr: transmit console register */
  REG(tdr);           /* 0C Msr/Tdr: modem status reg/transmit data reg */
  REG(lpr0);          /* 10 Lpr0: */
  REG(lpr1);          /* 14 Lpr0: */
  REG(lpr2);          /* 18 Lpr0: */
  REG(lpr3);          /* 1C Lpr0: */
} *dz  = (void*)0x200A0000;
#undef REG

cons_decl(dz);

int
dzcngetc(dev) 
	dev_t dev;
{
	int c = 0;
	int mino = minor(dev);
	u_short rbuf;
#if 0
	u_char mask;

	mask = vs_cpu->vc_intmsk;	/* save old state */
	vs_cpu->vc_intmsk = 0;		/* disable all interrupts */
#endif

	do {
		while ((dz->csr & 0x80) == 0)
			; /* Wait for char */
		rbuf = dz->rbuf;
		if (((rbuf >> 8) & 3) != mino)
			continue;
		c = rbuf & 0x7f;
	} while (c == 17 || c == 19);		/* ignore XON/XOFF */

	if (c == 13)
		c = 10;

#if 0
	vs_cpu->vc_intclr = 0x80;	/* clear te interrupt request */
	vs_cpu->vc_intmsk = mask;	/* restore interrupt mask */
#endif

	return (c);
}

#define	DZMAJOR	1

void
dzcnprobe(cndev)
	struct	consdev *cndev;
{
	extern	vaddr_t iospace;

	switch (vax_boardtype) {
	case VAX_BTYP_410:
	case VAX_BTYP_420:
	case VAX_BTYP_43:
	case VAX_BTYP_46:
		cndev->cn_dev = makedev(DZMAJOR, 3);
		dz_regs = iospace;
		ioaccess(iospace, 0x200A0000, 1);
		cndev->cn_pri = CN_NORMAL;
		break;

	default:
		cndev->cn_pri = CN_DEAD;
		break;
	}

	return;
}

void
dzcninit(cndev)
	struct	consdev *cndev;
{
	dz = (void*)dz_regs;

	dz->csr = 0;    /* Disable scanning until initting is done */
	dz->tcr = 8;    /* Turn off all but line 3's xmitter */
	dz->csr = 0x20; /* Turn scanning back on */
}


void
dzcnputc(dev,ch)
	dev_t	dev;
	int	ch;
{
	int timeout = 1<<15;            /* don't hang the machine! */
	int mino = minor(dev);
	u_short tcr;
#if 0
	u_char mask;
#endif

	if (mfpr(PR_MAPEN) == 0)
		return;

#if 0
	mask = vs_cpu->vc_intmsk;	/* save old state */
	vs_cpu->vc_intmsk = 0;		/* disable all interrupts */
#endif
	tcr = dz->tcr;	/* remember which lines to scan */
	dz->tcr = (1 << mino);

	while ((dz->csr & 0x8000) == 0) /* Wait until ready */
		if (--timeout < 0)
			break;
	dz->tdr = ch;                    /* Put the  character */

	dz->tcr = tcr;
#if 0
	vs_cpu->vc_intmsk = mask;	/* restore interrupt mask */
#endif
}

void 
dzcnpollc(dev, pollflag)
	dev_t dev;
	int pollflag;
{
}

#if NLKC
cons_decl(lkc);

void
lkccninit(cndev)
	struct	consdev *cndev;
{
	dz = (void*)dz_regs;

	dz->csr = 0;    /* Disable scanning until initting is done */
	dz->tcr = 1;    /* Turn off all but line 0's xmitter */
	dz->rbuf = 0x1c18; /* XXX */
	dz->csr = 0x20; /* Turn scanning back on */
}

int
lkccngetc(dev) 
	dev_t dev;
{
	int lkc_decode(int);
	int c;
#if 0
	u_char mask;

	mask = vs_cpu->vc_intmsk;	/* save old state */
	vs_cpu->vc_intmsk = 0;		/* disable all interrupts */
#endif

loop:
	while ((dz->csr & 0x80) == 0)
		; /* Wait for char */

	c = lkc_decode(dz->rbuf & 255);
	if (c < 1)
		goto loop;

#if 0
	vs_cpu->vc_intclr = 0x80;	/* clear te interrupt request */
	vs_cpu->vc_intmsk = mask;	/* restore interrupt mask */
#endif

	return (c);
}
#endif
