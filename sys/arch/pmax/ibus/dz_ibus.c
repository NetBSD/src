/*	$NetBSD: dz_ibus.c,v 1.1.2.1 2002/03/15 14:22:42 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include "dzkbd.h"
#include "dzms.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>

#include <dev/cons.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>

#include <pmax/ibus/ibusvar.h>

#define	DZ_LINE_KBD	0
#define	DZ_LINE_MOUSE	1
#define	DZ_LINE_CONSOLE	2
#define	DZ_LINE_AUX	3

cdev_decl(dz);

int	dz_ibus_match(struct device *, struct cfdata *, void *);
void	dz_ibus_attach(struct device *, struct device *, void *);
int	dz_ibus_intr(void *);
void	dz_ibus_cnsetup(paddr_t);
void	dz_ibus_cnattach(void);
int	dz_ibus_cngetc(dev_t);
void	dz_ibus_cnputc(dev_t, int);
void	dz_ibus_cnpollc(dev_t, int);
int	dz_ibus_getmajor(void);
int	dz_ibus_print(void *, const char *);

int	dzgetc(struct dz_linestate *);
void	dzputc(struct dz_linestate *, int);

struct cfattach dz_ibus_ca = {
	sizeof(struct dz_softc), dz_ibus_match, dz_ibus_attach
};

struct consdev dz_ibus_consdev = {
	NULL, NULL, dz_ibus_cngetc, dz_ibus_cnputc,
	dz_ibus_cnpollc, NULL, NODEV, CN_NORMAL,
};

struct dzregs {
	u_int16_t	csr;	/* 00 Csr: control/status */
	u_int16_t	p0[3];

	u_int16_t	rbuf;	/* 08 Rbuf/Lpr: receive buffer/line param  */
	u_int16_t	p1[3];

	u_int16_t	tcr;	/* 10 Tcr: transmit console */
	u_int16_t	p2[3];

	u_int16_t	tdr;	/* 18 Msr/Tdr: modem status reg/xmit data */
	u_int16_t	p3[3];
} volatile *dzcn;

int	dz_ibus_iscn;

int
dz_ibus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ibus_attach_args *iba;

	iba = aux;

	if (strcmp(iba->ia_name, "dc") != 0 &&
	    strcmp(iba->ia_name, "mdc") != 0 &&
	    strcmp(iba->ia_name, "dc7085") != 0)
		return (0);

	if (badaddr((caddr_t)iba->ia_addr, 2))
		return (0);

	return (1);
}

void
dz_ibus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ibus_attach_args *iba;
	struct dz_softc *sc;
	volatile struct dzregs *dz;
	struct dzkm_attach_args daa;
	int i;

	iba = aux;
	sc = (struct dz_softc *)self;

	DELAY(100000);

	/* 
	 * XXX - This is evil and ugly, but... due to the nature of how
	 * bus_space_* works on pmax it will do for the time being.
	 */
	sc->sc_ioh = (bus_space_handle_t)MIPS_PHYS_TO_KSEG1(iba->ia_addr);

	sc->sc_dr.dr_csr = 0;
	sc->sc_dr.dr_rbuf = 8;
	sc->sc_dr.dr_dtr = 17;
	sc->sc_dr.dr_break = 25;
	sc->sc_dr.dr_tbuf = 24;
	sc->sc_dr.dr_tcr = 16;
	sc->sc_dr.dr_dcd = 25;
	sc->sc_dr.dr_ring = 24;

	sc->sc_type = DZ_DZV;

	dz = (volatile struct dzregs *)sc->sc_ioh;
	i = dz->tcr;
	dz->csr = DZ_CSR_MSE | DZ_CSR_TXIE;
	dz->tcr = 0;
	DELAY(1000);
	dz->tcr = 1;
	DELAY(100000);
	dz->tcr = i;

	sc->sc_dsr = 0x0f; /* XXX check if VS has modem ctrl bits */

	printf(": DC-7805\n");
	ibus_intr_establish(parent, (void *)iba->ia_cookie, IPL_TTY,
	    dz_ibus_intr, sc);
	dzattach(sc, NULL);
	DELAY(10000);

#if NDZKBD > 0
	if (!dz_ibus_iscn)
		dz->rbuf = DZ_LPR_RX_ENABLE | (DZ_LPR_B4800 << 8) |
		    DZ_LPR_8_BIT_CHAR;
	daa.daa_line = DZ_LINE_KBD;
	daa.daa_flags = (dz_ibus_iscn ? 0 : DZKBD_CONSOLE);
	config_found(self, &daa, dz_ibus_print);
#endif
#if NDZMS > 0
	dz->rbuf = DZ_LPR_RX_ENABLE | (DZ_LPR_B4800 << 8) | DZ_LPR_7_BIT_CHAR
	    | DZ_LPR_PARENB | DZ_LPR_OPAR | 1 /* line */;
	daa.daa_line = DZ_LINE_MOUSE;
	daa.daa_flags = 0;
	config_found(self, &daa, dz_ibus_print);
#endif
}

int
dz_ibus_getmajor(void)
{
	static int cache = -1;
	int i;

	if (cache != -1)
		return (cache);

	for (i = 0; i < nchrdev; i++)
		if (cdevsw[i].d_open == dzopen)
			return (cache = i);

	panic("dz_ibus_getmajor()");
}

int
dz_ibus_intr(void *cookie)
{
	struct dz_softc *sc;
	volatile struct dzregs *dzr;
	unsigned csr;

	sc = cookie;
	dzr = (volatile struct dzregs *)sc->sc_ioh;

	while (((csr = dzr->csr) & (DZ_CSR_RX_DONE | DZ_CSR_TX_READY)) != 0) {
		if ((csr & DZ_CSR_RX_DONE) != 0)
			dzrint(sc);
		if ((csr & DZ_CSR_TX_READY) != 0)
			dzxint(sc);
	}

	return (0);
}

void
dz_ibus_cnsetup(paddr_t addr)
{

	dzcn = (void *)MIPS_PHYS_TO_KSEG1(addr);
}

void
dz_ibus_cnattach(void)
{

	printf("dz_ibus_cnattach: 0\n");

	dz_ibus_iscn = 1;

	/* Disable scanning until init is done. */
	dzcn->csr = 0;

	/* Turn on transmitter for the console. */
	dzcn->tcr = (1 << DZ_LINE_CONSOLE);

	/* Turn scanning back on. */
	dzcn->csr = 0x20;

	printf("dz_ibus_cnattach: 1\n");

	/*
	 * Point the console at the DZ-11.
	 */
	cn_tab = &dz_ibus_consdev;
	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(dz_ibus_getmajor(), DZ_LINE_CONSOLE);

	printf("dz_ibus_cnattach: 2\n");
}

int
dz_ibus_cngetc(dev_t dev)
{
	int c, line;
	u_int16_t rbuf;

	c = 0;
	line = minor(dev);

	do {
		while ((dzcn->csr & DZ_CSR_RX_DONE) == 0)
			;
		rbuf = dzcn->rbuf;
		if (((rbuf >> 8) & 3) != line)
			continue;
		c = rbuf & 0x7f;
	} while (c == 17 || c == 19);		/* ignore XON/XOFF */

	if (c == 13)
		c = 10;

	return (c);
}

void
dz_ibus_cnputc(dev_t dev, int ch)
{
	int timeout;
	u_int16_t tcr;

	/* Don't hang the machine! */
	timeout = 1 << 15;

	/* Remember which lines to scan */
	tcr = dzcn->tcr;
	dzcn->tcr = (1 << minor(dev));

	/* Wait until ready */
	while ((dzcn->csr & 0x8000) == 0)
		if (--timeout < 0)
			break;

	/* Put the character */
	dzcn->tdr = ch;
	timeout = 1 << 15;

	/* Wait until ready */
	while ((dzcn->csr & 0x8000) == 0)
		if (--timeout < 0)
			break;

	dzcn->tcr = tcr;
}

void    
dz_ibus_cnpollc(dev_t dev, int pollflag)
{

}

#if NDZKBD > 0 || NDZMS > 0
int
dz_ibus_print(void *aux, const char *name)
{

	return (QUIET);
}

int
dzgetc(struct dz_linestate *ls)
{
	volatile struct dzregs *dzr;
	int line;
	u_int16_t rbuf;

	if (ls == NULL) {
		/*
		 * dzkbd is the only thing that should put us here.
		 */
		line = DZ_LINE_KBD;
		dzr = dzcn;
	} else {
		line = ls->dz_line;
		dzr = (volatile struct dzregs *)ls->dz_sc->sc_ioh;
	}

	for (;;) {
		while ((dzr->csr & DZ_CSR_RX_DONE) == 0)
			;
		rbuf = dzr->rbuf;
		if (((rbuf >> 8) & 3) == line)
			return (rbuf & 0xff);
	}
}

void
dzputc(struct dz_linestate *ls, int ch)
{
	volatile struct dzregs *dzr;
	int line;
	u_int16_t tcr;
	int s;

	/*
	 * If the dz has already been attached, the MI driver will do the
	 * transmitting:
	 */
	if (ls != NULL && ls->dz_sc != NULL) {
		line = ls->dz_line;
		dzr = (volatile struct dzregs *)ls->dz_sc->sc_ioh;
		s = spltty();
		putc(ch, &ls->dz_sc->sc_dz[line].dz_tty->t_outq);
		tcr = dzr->tcr;
		if ((tcr & (1 << line)) == 0)
			dzr->tcr = tcr | (1 << line);
		dzxint(ls->dz_sc);
		splx(s);
		return;
	}

	/* Use dzcnputc to do the transmitting. */
	dz_ibus_cnputc(makedev(dz_ibus_getmajor(), DZ_LINE_KBD), ch);
}
#endif /* NDZKBD > 0 || NDZMS > 0 */
