/* $NetBSD: dc.c,v 1.1.2.9 1999/11/19 09:39:37 nisimura Exp $ */

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

#include "opt_ddb.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dc.c,v 1.1.2.9 1999/11/19 09:39:37 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <pmax/ibus/dc7085reg.h>
#include <pmax/ibus/dc7085var.h>

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
	{ 19200,DC_B19200 },
#ifdef notyet
	{ 38400, DC_B38400},	/* Overloaded with 19200, per chip. */
#endif
	{ -1,	-1 }
};

int dcintr __P((void *));			/* EXPORT */
caddr_t dc_cons_addr;				/* EXPORT */
extern int dc_major;				/* IMPORT */
extern void dcstart __P((struct tty *));	/* IMPORT */

int
dcintr(v)
	void *v;
{
	struct dc_softc *sc = v;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int cc, line;
	struct tty *tp;
	unsigned csr;

	csr = bus_space_read_2(bst, bsh, DCCSR);
	if ((csr & (DC_RDONE | DC_TRDY)) == 0)
		return 0;

	do {
		line = (csr >> 8) & 3;
		tp = sc->sc_tty[line];
		if (csr & DC_RDONE) {
			cc = bus_space_read_2(bst, bsh, DCRBUF);
			if (cc & 0x8000) {
				if (cc & DC_DO)
					goto fetchnext; /* overran */
#ifdef DDB
				if ((cc & DC_FE) && tp != NULL
				    && tp->t_dev == cn_tab->cn_dev) {
					console_debugger();
					goto fetchnext;
				}
#endif
				if (tp != NULL) {
					if (cc & DC_FE)
						cc |= TTY_FE;
					if (cc & DC_PE)
						cc |= TTY_PE;
					(*linesw[tp->t_line].l_rint)(cc, tp);
				}
				else if (sc->sc_wscons[line] != NULL)
					(*sc->sc_wscons[line])(cc);
			}
		}
		if (csr & DC_TRDY) {
			if (sc->sc_xmit[line].p < sc->sc_xmit[line].e) {
				cc = *sc->sc_xmit[line].p++;	
				bus_space_write_2(bst, bsh, DCTBUF, cc);
				DELAY(10);
				goto fetchnext;
			}
			if (tp == NULL)
				goto fetchnext;
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
				cc = bus_space_read_2(bst, bsh, DCTCR);
				cc &= ~(1 << line);
				bus_space_write_2(bst, bsh, DCTCR, cc);
				DELAY(10);
			}
		}
   fetchnext:
		csr = bus_space_read_2(bst, bsh, DCCSR);
	} while (csr & (DC_RDONE | DC_TRDY));

	return 1;
}

void dc_cnattach __P((paddr_t, int));
void dc_cninit __P((paddr_t, int, int));
int  dc_cngetc __P((dev_t));
void dc_cnputc __P((dev_t, int));
void dc_cnpollc __P((dev_t, int));

struct consdev dc_cons = {
	NULL, NULL, dc_cngetc, dc_cnputc, dc_cnpollc, NODEV, CN_NORMAL,
};

caddr_t dc_cons_addr;

void
dc_cnattach(addr, line)
	paddr_t addr;
	int line;
{
	dc_cninit(addr, line, 9600);

	cn_tab = &dc_cons;
	cn_tab->cn_pri = CN_REMOTE;
	cn_tab->cn_dev = makedev(dc_major, line);
}

int
dc_cngetc(dev)
	dev_t dev;
{
	struct dc7085reg *v = (void *)dc_cons_addr;
	int s, line;
	u_int16_t c;

	line = DCLINE(dev);
	do {
		s = splhigh();
		while ((v->dc_csr & DC_RDONE) == 0) {
			splx(s);
			DELAY(10);
			s = splhigh();
		}
		splx(s);
		c = v->dc_rbuf;
	} while ((c & (DC_PE | DC_FE | DC_DO)) || line != ((c >> 8) & 3));
	return c & 0xff;
}

void
dc_cnputc(dev, c)
	dev_t dev;
	int c;
{
	struct dc7085reg *v = (void *)dc_cons_addr;
	int s, line, timeout;

	timeout = 1 << 15;
	line = DCLINE(dev);
	s = splhigh();
	while ((v->dc_csr & DC_TRDY) == 0 && --timeout > 0) {
		splx(s);
		DELAY(100);
		s = splhigh();
	}		
	if (line == ((v->dc_csr >> 8) & 3))
		v->dc_tbuf = c & 0xff;
	else {	
		u_int16_t tcr;
	
		tcr = v->dc_tcr;
		v->dc_tcr = (tcr & 0xff00) | (1 << line);
		wbflush();
		v->dc_tbuf = c & 0xff;
		wbflush();
		v->dc_tcr = tcr;
	}
	wbflush();
	splx(s);
}
	
void	
dc_cnpollc(dev, onoff)
	dev_t dev;
	int onoff;
{	
}

void
dc_cninit(addr, line, rate)
	paddr_t addr;
	int line, rate;
{
	struct dc7085reg *v;
	int speed;

	v = (void *)MIPS_PHYS_TO_KSEG1(addr);
	speed = ttspeedtab(rate, dcspeedtab);

	v->dc_csr = DC_CLR;
	wbflush();
	DELAY(10);
	while (v->dc_csr & DC_CLR)
		;
	v->dc_csr = DC_MSE;
	wbflush();
	DELAY(10);

	v->dc_tcr = 1 << line;
	v->dc_lpr = DC_RE | speed | DC_BITS8 | line;
	wbflush();
	DELAY(10);

	dc_cons_addr = (caddr_t)v;
}
