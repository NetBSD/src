/*	$NetBSD: dc.c,v 1.1.2.8 1999/04/17 13:45:53 nisimura Exp $ */

/*
 * Copyright (c) 1998, 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *	for the NetBSD Project.
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

/*
 * DC7085 (DZ-11 look alike) quad asynchronous serial interface
 */

#include "opt_ddb.h"			/* XXX TBD XXX */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dc.c,v 1.1.2.8 1999/04/17 13:45:53 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <pmax/ibus/dc7085reg.h> /* XXX dev/ic/dc7085reg.h XXX */
#include <pmax/ibus/dc7085var.h> /* XXX machine/dc7085var.h XXX */

struct speedtab dcspeedtab[] = {
	{ 0,	0,	  },
	{ 50,	DC_B50	  },
	{ 75,	DC_B75	  },
	{ 110,	DC_B110	  },
	{ 134,	DC_B134_5 },
	{ 150,	DC_B150	  },
	{ 300,	DC_B300	  },
	{ 600,	DC_B600	  },
	{ 1200,	DC_B1200  },
	{ 1800,	DC_B1800  },
	{ 2400,	DC_B2400  },
	{ 4800,	DC_B4800  },
	{ 9600,	DC_B9600  },
	{ 19200, DC_B19200},
#ifdef notyet
	{ 38400, DC_B38400},	/* Overloaded with 19200, per chip. */
#endif
	{ -1,	-1 }
};

int dcintr __P((void *));			/* EXPORT */
int dcsoftintr __P((struct dc_softc *));	/* EXPORT */
caddr_t dc_cons_addr;				/* EXPORT */
extern int dc_major;				/* IMPORT */
extern int softisr;				/* IMPORT */
extern void dcstart __P((struct tty *));	/* IMPORT */

int
dcintr(v)
	void *v;
{
	struct dc_softc *sc = v;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int dc_softreq, c, line, put, put_next;
	struct tty *tp;
	unsigned csr;

	csr = bus_space_read_2(bst, bsh, DCCSR);
	if ((csr & (DC_RDONE | DC_TRDY)) == 0)
		return 0;

	dc_softreq = 0;
	do {
		if (csr & DC_RDONE) {
			c = bus_space_read_2(bst, bsh, DCRBUF);
			while (c & 0x8000) {
				sc->dc_rbuf[put] = c & 0x3ff;
				put_next = (put + 1) & DC_RX_RING_MASK;
				if (put_next == sc->dc_rbget)
					continue; /* overflow */
				sc->dc_rbput = put = put_next;
				c = bus_space_read_2(bst, bsh, DCRBUF);
			}
			dc_softreq = 1;
		}
		if (csr & DC_TRDY) {
			line = (csr >> 8) & 3;
			if (sc->sc_xmit[line].p < sc->sc_xmit[line].e) {
				c = *sc->sc_xmit[line].p++;	
				bus_space_write_2(bst, bsh, DCTBUF, c);
				DELAY(10);
				continue;
			}
			tp = sc->sc_tty[line];
			if (tp == NULL)
				continue;
			tp->t_state &= ~TS_BUSY;
			if (tp->t_state & TS_FLUSH)
				tp->t_state &= ~TS_FLUSH;
			else {
				ndflush(&tp->t_outq,
				    sc->sc_xmit[line].e - tp->t_outq.c_cf);
			}
			if (tp->t_line)
				(*linesw[tp->t_line].l_start)(tp);
			else
				dcstart(tp);
			if (tp->t_outq.c_cc == 0 || !(tp->t_state & TS_BUSY)) {
				c = bus_space_read_2(bst, bsh, DCTCR);
				c &= ~(1 << line);
				bus_space_write_2(bst, bsh, DCTCR, c);
				DELAY(10);
			}
		}
		csr = bus_space_read_2(bst, bsh, DCCSR);
	} while (csr & (DC_RDONE | DC_TRDY));

	if (dc_softreq == 1) {
		softisr |= 1 << sc->sc_unit;		/* !!! MD !!! */
		_setsoftintr(MIPS_SOFT_INT_MASK_1);	/* !!! MD !!! */
	}
	return 1;
}

int
dcsoftintr(sc)
	struct dc_softc *sc;
{
	int c, cc, line, get, s;
	void (*input) __P((void *, int));

	/* Atomically get and clear flags. */
	s = splhigh();
#if 0
	intr_flags = sc->dc_intr_flags;
	sc->dc_intr_flags = 0;
#endif
	/* Now lower to spltty for the rest. */
	(void)spltty();
	get = sc->dc_rbget;
	while (get != sc->dc_rbput) {
		c = sc->dc_rbuf[get];
		get = (get + 1) & DC_RX_RING_MASK;
		line = (c >> 8) & 3;
		input = sc->sc_line[line].f;
		if (input == NULL)
			continue;
		cc = c & 0xff;
		if (c & DC_FE)
			cc |= TTY_FE;
		if (c & DC_PE)
			cc |= TTY_PE;
		(*input)(sc->sc_tty[line], cc);
	}
	splx(s);
	return 1;
}

int	dcgetc __P((struct dc7085reg *, int));		/* EXPORT */
void	dcputc __P((struct dc7085reg *, int, int));	/* EXPORT */

int
dcgetc(reg, line)
	struct dc7085reg *reg;
	int line;
{
	int s;
	u_int16_t c;

	s = splhigh();
again:
	while ((reg->dccsr & DC_RDONE) == 0)
		DELAY(10);
	c = reg->dcrbuf;
	if ((c & (DC_PE | DC_FE | DC_DO)) || line != ((c >> 8) & 3))
		goto again;
	splx(s);
	
	return c & 0xff;
}

void
dcputc(reg, line, c)
	struct dc7085reg *reg;
	int line, c;
{
	int s, timeout = 1 << 15;

	s = splhigh();
	while ((reg->dccsr & DC_TRDY) == 0 && --timeout > 0)
		DELAY(100);
	if (line == ((reg->dccsr >> 8) & 3))
		reg->dctbuf = c & 0xff;
	else {
		u_int16_t tcr = reg->dctcr;
		reg->dctcr = (tcr & 0xff00) | (1 << line);
		wbflush();
		reg->dctbuf = c & 0xff;
		wbflush();
		reg->dctcr = tcr;
	}
	wbflush();
	splx(s);
}

static int  dc_cngetc __P((dev_t));
static void dc_cnputc __P((dev_t, int));
static void dc_cnpollc __P((dev_t, int));
int dc_cnattach __P((paddr_t, int, int, int));		/* EXPORT */

struct consdev dc_cons = {
	NULL, NULL, dc_cngetc, dc_cnputc, dc_cnpollc, NODEV, CN_NORMAL,
};

static int
dc_cngetc(dev)
	dev_t dev;
{
	return dcgetc((struct dc7085reg *)dc_cons_addr, DCLINE(dev));
}

static void
dc_cnputc(dev, c)
	dev_t dev;
	int c;
{
	if (DCUNIT(dev) != 0)
		return;
	dcputc((struct dc7085reg *)dc_cons_addr, DCLINE(dev), c);
}

static void
dc_cnpollc(dev, onoff)
	dev_t dev;
	int onoff;
{
	struct dc7085reg *reg;
	int csr;

	reg = (void *)dc_cons_addr;
	csr = DC_MSE;
	if (onoff)
		csr |= DC_RIE | DC_TIE;
	else
		csr &= ~(DC_RIE | DC_TIE);
	reg->dccsr = csr;
	wbflush();
}

/*ARGSUSED*/
int
dc_cnattach(addr, line, rate, cflag)
	paddr_t addr;
	int line, rate, cflag;
{
	struct dc7085reg *reg;
	int speed;

	dc_cons_addr = (caddr_t)MIPS_PHYS_TO_KSEG1(addr); /* XXX */

	if (badaddr((caddr_t)dc_cons_addr, 2))
		return 1;

	speed = ttspeedtab(rate, dcspeedtab);

	reg = (void *)dc_cons_addr;
	reg->dccsr = DC_CLR;
	wbflush();
	reg->dctcr = line << 3;
	reg->dclpr = DC_RE | speed | DC_BITS8 | (line << 3);
	reg->dccsr = DC_MSE;
	wbflush();

	cn_tab = &dc_cons;
	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(dc_major, line);

	return 0;
}
