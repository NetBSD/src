/*	$NetBSD: par.c,v 1.24.6.1 2006/04/22 11:38:08 simonb Exp $	*/

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ppi.c	7.3 (Berkeley) 12/16/90
 */

/*
 * parallel port interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: par.c,v 1.24.6.1 2006/04/22 11:38:08 simonb Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/parioctl.h>

#include <arch/x68k/dev/intiovar.h>

struct	par_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_flags;
	struct parparam		sc_param;
#define sc_burst 	sc_param.burst
#define sc_timo  	sc_param.timo
#define sc_delay 	sc_param.delay
	struct callout		sc_timo_ch;
	struct callout		sc_start_ch;
} ;

/* par registers */
#define PAR_DATA	1
#define PAR_STROBE	3

/* sc_flags values */
#define	PARF_ALIVE	0x01	
#define	PARF_OPEN	0x02	
#define PARF_UIO	0x04
#define PARF_TIMO	0x08
#define PARF_DELAY	0x10
#define PARF_OREAD	0x40	/* no support */
#define PARF_OWRITE	0x80


void partimo(void *);
void parstart(void *);
void parintr(void *);
int parrw(dev_t, struct uio *);
int parhztoms(int);
int parmstohz(int);
int parsendch(struct par_softc *, u_char);
int parsend(struct par_softc *, u_char *, int);

static struct callout intr_callout = CALLOUT_INITIALIZER;

#define UNIT(x)		minor(x)

#ifdef DEBUG
#define PDB_FOLLOW	0x01
#define PDB_IO		0x02
#define PDB_INTERRUPT   0x04
#define PDB_NOCHECK	0x80
#ifdef PARDEBUG
int	pardebug = PDB_FOLLOW | PDB_IO | PDB_INTERRUPT;
#else
int	pardebug = 0;
#endif
#endif

int parmatch(struct device *, struct cfdata *, void *);
void parattach(struct device *, struct device *, void *);

CFATTACH_DECL(par, sizeof(struct par_softc),
    parmatch, parattach, NULL, NULL);

extern struct cfdriver par_cd;

static int par_attached;

dev_type_open(paropen);
dev_type_close(parclose);
dev_type_write(parwrite);
dev_type_ioctl(parioctl);

const struct cdevsw par_cdevsw = {
	paropen, parclose, noread, parwrite, parioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

int 
parmatch(struct device *pdp, struct cfdata *cfp, void *aux)
{
	struct intio_attach_args *ia = aux;

	/* X680x0 has only one parallel port */
	if (strcmp(ia->ia_name, "par") || par_attached)
		return 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = 0xe8c000;
	ia->ia_size = 0x2000;
	if (intio_map_allocate_region (pdp, ia, INTIO_MAP_TESTONLY))
		return 0;
	if (ia->ia_intr == INTIOCF_INTR_DEFAULT)
		ia->ia_intr = 99;
#if DIAGNOSTIC
	if (ia->ia_intr != 99)
		return 0;
#endif

	return 1;
}

void 
parattach(struct device *pdp, struct device *dp, void *aux)
{
	struct par_softc *sc = (struct par_softc *)dp;
	struct intio_attach_args *ia = aux;
	int r;
	
	par_attached = 1;

	sc->sc_flags = PARF_ALIVE;
	printf(": parallel port (write only, interrupt)\n");
	ia->ia_size = 0x2000;
	r = intio_map_allocate_region (pdp, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic ("IO map for PAR corruption??");
#endif
	sc->sc_bst = ia->ia_bst;
	r = bus_space_map (sc->sc_bst,
			   ia->ia_addr, ia->ia_size,
			   BUS_SPACE_MAP_SHIFTED,
			   &sc->sc_bsh);
#ifdef DIAGNOSTIC
	if (r)
		panic ("Cannot map IO space for PAR.");
#endif

	intio_set_sicilian_intr(intio_get_sicilian_intr() &
				~SICILIAN_INTR_PAR);

	intio_intr_establish(ia->ia_intr, "par",
			     (intio_intr_handler_t) parintr, (void*) 1);

	callout_init(&sc->sc_timo_ch);
	callout_init(&sc->sc_start_ch);
}

int 
paropen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = UNIT(dev);
	struct par_softc *sc;
	
	sc = device_lookup(&par_cd, unit);
	if (sc == NULL || !(sc->sc_flags & PARF_ALIVE))
		return(ENXIO);
	if (sc->sc_flags & PARF_OPEN)
		return(EBUSY);
	/* X680x0 can't read */
	if ((flags & FREAD) == FREAD)
		return (EINVAL);
	
	sc->sc_flags |= PARF_OPEN;
	
	sc->sc_flags |= PARF_OWRITE;
	
	sc->sc_burst = PAR_BURST;
	sc->sc_timo = parmstohz(PAR_TIMO);
	sc->sc_delay = parmstohz(PAR_DELAY);

	return(0);
}

int 
parclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = UNIT(dev);
	int s;
	struct par_softc *sc = par_cd.cd_devs[unit];
	
	sc->sc_flags &= ~(PARF_OPEN|PARF_OWRITE);

	/* don't allow interrupts any longer */
	s = spl1();
	intio_set_sicilian_intr(intio_get_sicilian_intr() &
				~SICILIAN_INTR_PAR);
	splx(s);

	return (0);
}

void 
parstart(void *arg)
{
	struct par_softc *sc = arg;
#ifdef DEBUG
	if (pardebug & PDB_FOLLOW)
		printf("parstart(%x)\n", device_unit(&sc->sc_dev));
#endif
	sc->sc_flags &= ~PARF_DELAY;
	wakeup(sc);
}

void 
partimo(void *arg)
{
	struct par_softc *sc = arg;
#ifdef DEBUG
	if (pardebug & PDB_FOLLOW)
		printf("partimo(%x)\n", device_unit(&sc->sc_dev));
#endif
	sc->sc_flags &= ~(PARF_UIO|PARF_TIMO);
	wakeup(sc);
}

int 
parwrite(dev_t dev, struct uio *uio, int flag)
{
	
#ifdef DEBUG
	if (pardebug & PDB_FOLLOW)
		printf("parwrite(%x, %p)\n", dev, uio);
#endif
	return (parrw(dev, uio));
}

int 
parrw(dev_t dev, struct uio *uio)
{
	int unit = UNIT(dev);
	struct par_softc *sc = par_cd.cd_devs[unit];
	int len=0xdeadbeef;	/* XXX: shutup gcc */
	int s, cnt=0;
	char *cp;
	int error = 0;
	int buflen;
	char *buf;
	
	if (!!(sc->sc_flags & PARF_OREAD) ^ (uio->uio_rw == UIO_READ))
		return EINVAL;
	
	if (uio->uio_resid == 0)
		return(0);
	
	buflen = min(sc->sc_burst, uio->uio_resid);
	buf = (char *)malloc(buflen, M_DEVBUF, M_WAITOK);
	sc->sc_flags |= PARF_UIO;
	if (sc->sc_timo > 0) {
		sc->sc_flags |= PARF_TIMO;
		callout_reset(&sc->sc_timo_ch, sc->sc_timo, partimo, sc);
	}
	while (uio->uio_resid > 0) {
		len = min(buflen, uio->uio_resid);
		cp = buf;
		if (uio->uio_rw == UIO_WRITE) {
			error = uiomove(cp, len, uio);
			if (error)
				break;
		}
	      again:
		s = spl1();
		/*
		 * Check if we timed out during sleep or uiomove
		 */
		(void) spllowersoftclock();
		if ((sc->sc_flags & PARF_UIO) == 0) {
#ifdef DEBUG
			if (pardebug & PDB_IO)
				printf("parrw: uiomove/sleep timo, flags %x\n",
				       sc->sc_flags);
#endif
			if (sc->sc_flags & PARF_TIMO) {
				callout_stop(&sc->sc_timo_ch);
				sc->sc_flags &= ~PARF_TIMO;
			}
			splx(s);
			break;
		}
		splx(s);
		/*
		 * Perform the operation
		 */
		cnt = parsend(sc, cp, len);
		if (cnt < 0) {
			error = -cnt;
			break;
		}
		
		s = splsoftclock();
		/*
		 * Operation timeout (or non-blocking), quit now.
		 */
		if ((sc->sc_flags & PARF_UIO) == 0) {
#ifdef DEBUG
			if (pardebug & PDB_IO)
				printf("parrw: timeout/done\n");
#endif
			splx(s);
			break;
		}
		/*
		 * Implement inter-read delay
		 */
		if (sc->sc_delay > 0) {
			sc->sc_flags |= PARF_DELAY;
			callout_reset(&sc->sc_start_ch, sc->sc_delay,
			    parstart, sc);
			error = tsleep(sc, PCATCH|(PZERO-1), "par-cdelay", 0);
			if (error) {
				splx(s);
				break;
			}
		}
		splx(s);
		/*
		 * Must not call uiomove again til we've used all data
		 * that we already grabbed.
		 */
		if (uio->uio_rw == UIO_WRITE && cnt != len) {
			cp += cnt;
			len -= cnt;
			cnt = 0;
			goto again;
		}
	}
	s = splsoftclock();
	if (sc->sc_flags & PARF_TIMO) {
		callout_stop(&sc->sc_timo_ch);
		sc->sc_flags &= ~PARF_TIMO;
	}
	if (sc->sc_flags & PARF_DELAY)	{
		callout_stop(&sc->sc_start_ch);
		sc->sc_flags &= ~PARF_DELAY;
	}
	splx(s);
	/*
	 * Adjust for those chars that we uiomove'ed but never wrote
	 */
	/*
	 * XXXjdolecek: this len usage is wrong, this will be incorrect
	 * if the transfer size is longer than sc_burst
	 */
	if (uio->uio_rw == UIO_WRITE && cnt != len) {
		uio->uio_resid += (len - cnt);
#ifdef DEBUG
			if (pardebug & PDB_IO)
				printf("parrw: short write, adjust by %d\n",
				       len-cnt);
#endif
	}
	free(buf, M_DEVBUF);
#ifdef DEBUG
	if (pardebug & (PDB_FOLLOW|PDB_IO))
		printf("parrw: return %d, resid %d\n", error, uio->uio_resid);
#endif
	return (error);
}

int 
parioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct par_softc *sc = par_cd.cd_devs[UNIT(dev)];
	struct parparam *pp, *upp;
	int error = 0;
	
	switch (cmd) {
	      case PARIOCGPARAM:
		pp = &sc->sc_param;
		upp = (struct parparam *)data;
		upp->burst = pp->burst;
		upp->timo = parhztoms(pp->timo);
		upp->delay = parhztoms(pp->delay);
		break;
		
	      case PARIOCSPARAM:
		pp = &sc->sc_param;
		upp = (struct parparam *)data;
		if (upp->burst < PAR_BURST_MIN || upp->burst > PAR_BURST_MAX ||
		    upp->delay < PAR_DELAY_MIN || upp->delay > PAR_DELAY_MAX)
			return(EINVAL);
		pp->burst = upp->burst;
		pp->timo = parmstohz(upp->timo);
		pp->delay = parmstohz(upp->delay);
		break;
		
	      default:
		return(EINVAL);
	}
	return (error);
}

int 
parhztoms(int h)
{
	extern int hz;
	int m = h;
	
	if (m > 0)
		m = m * 1000 / hz;
	return(m);
}

int 
parmstohz(int m)
{
	extern int hz;
	int h = m;
	
	if (h > 0) {
		h = h * hz / 1000;
		if (h == 0)
			h = 1000 / hz;
	}
	return(h);
}

/* stuff below here if for interrupt driven output of data thru
   the parallel port. */

int partimeout_pending;
int parsend_pending;

void 
parintr(void *arg)
{
	int s, mask;

	mask = (int)arg;
	s = splclock();

	intio_set_sicilian_intr(intio_get_sicilian_intr() &
				~SICILIAN_INTR_PAR);

#ifdef DEBUG
	if (pardebug & PDB_INTERRUPT)
		printf ("parintr %d(%s)\n", mask, mask ? "FLG" : "tout");
#endif
	/* if invoked from timeout handler, mask will be 0,
	 * if from interrupt, it will contain the cia-icr mask,
	 * which is != 0
	 */
	if (mask) {
		if (partimeout_pending)
			callout_stop(&intr_callout);
		if (parsend_pending)
			parsend_pending = 0;
	}
	
	/* either way, there won't be a timeout pending any longer */
	partimeout_pending = 0;
	
	wakeup(parintr);
	splx (s);
}

int
parsendch(struct par_softc *sc, u_char ch)
{
	int error = 0;
	int s;
	
	/* if either offline, busy or out of paper, wait for that
	   condition to clear */
	s = spl1();
	while (!error 
	       && (parsend_pending 
		   || !(intio_get_sicilian_intr() & SICILIAN_STAT_PAR)))
		{
			extern int hz;
			
			/* wait a second, and try again */
			callout_reset(&intr_callout, hz, parintr, 0);
			partimeout_pending = 1;
			/* this is essentially a flipflop to have us wait for the
			   first character being transmitted when trying to transmit
			   the second, etc. */
			parsend_pending = 0;
			/* it's quite important that a parallel putc can be
			   interrupted, given the possibility to lock a printer
			   in an offline condition.. */
			if ((error = tsleep (parintr, PCATCH|(PZERO-1), "parsendch", 0))) {
#ifdef DEBUG
				if (pardebug & PDB_INTERRUPT)
					printf ("parsendch interrupted, error = %d\n", error);
#endif
				if (partimeout_pending)
					callout_stop(&intr_callout);
				
				partimeout_pending = 0;
			}
		}
	
	if (!error) {
#ifdef DEBUG
		if (pardebug & PDB_INTERRUPT)
			printf ("#%d", ch);
#endif
		bus_space_write_1 (sc->sc_bst, sc->sc_bsh, PAR_DATA, ch);
		DELAY(1);	/* (DELAY(1) == 1us) > 0.5us */
		bus_space_write_1 (sc->sc_bst, sc->sc_bsh, PAR_STROBE, 0);
		intio_set_sicilian_intr (intio_get_sicilian_intr() |
					 SICILIAN_INTR_PAR);
		DELAY(1);
		bus_space_write_1 (sc->sc_bst, sc->sc_bsh, PAR_STROBE, 1);
		parsend_pending = 1;
	}
	
	splx (s);
	
	return error;
}


int
parsend(struct par_softc *sc, u_char *buf, int len)
{
	int err, orig_len = len;
	
	for (; len; len--, buf++)
		if ((err = parsendch (sc, *buf)))
			return err < 0 ? -EINTR : -err;
	
	/* either all or nothing.. */
	return orig_len;
}
