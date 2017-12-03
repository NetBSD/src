/*	$NetBSD: ppi.c,v 1.43.12.2 2017/12/03 11:36:13 jdolecek Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)ppi.c	8.1 (Berkeley) 6/16/93
 */

/*
 * Printer/Plotter HPIB interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ppi.c,v 1.43.12.2 2017/12/03 11:36:13 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>

#include <hp300/dev/hpibvar.h>

#include <hp300/dev/ppiioctl.h>

#include "ioconf.h"

struct	ppi_softc {
	device_t sc_dev;
	int	sc_flags;
	struct	hpibqueue sc_hq;	/* HP-IB job queue entry */
	struct	ppiparam sc_param;
#define sc_burst sc_param.burst
#define sc_timo  sc_param.timo
#define sc_delay sc_param.delay
	int	sc_sec;
	int	sc_slave;		/* HP-IB slave address */
	struct	callout sc_timo_ch;
	struct	callout sc_start_ch;
};

/* sc_flags values */
#define PPIF_ALIVE	0x01
#define PPIF_OPEN	0x02
#define PPIF_UIO	0x04
#define PPIF_TIMO	0x08
#define PPIF_DELAY	0x10

static int	ppimatch(device_t, cfdata_t, void *);
static void	ppiattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ppi, sizeof(struct ppi_softc),
    ppimatch, ppiattach, NULL, NULL);

static dev_type_open(ppiopen);
static dev_type_close(ppiclose);
static dev_type_read(ppiread);
static dev_type_write(ppiwrite);
static dev_type_ioctl(ppiioctl);

const struct cdevsw ppi_cdevsw = {
	.d_open = ppiopen,
	.d_close = ppiclose,
	.d_read = ppiread,
	.d_write = ppiwrite,
	.d_ioctl = ppiioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static void	ppistart(void *);
static void	ppinoop(void *);

static void	ppitimo(void *);
static int	ppirw(dev_t, struct uio *);
static int	ppihztoms(int);
static int	ppimstohz(int);

#define UNIT(x)		minor(x)

#ifdef DEBUG
int	ppidebug = 0x80;
#define PDB_FOLLOW	0x01
#define PDB_IO		0x02
#define PDB_NOCHECK	0x80
#endif

static int
ppimatch(device_t parent, cfdata_t cf, void *aux)
{
	struct hpibbus_attach_args *ha = aux;

	/*
	 * The printer/plotter doesn't return an ID tag.
	 * The check below prevents us from matching a CS80
	 * device by mistake.
	 */
	if (ha->ha_id & 0x200)
		return 0;

	/*
	 * To prevent matching all unused slots on the bus, we
	 * don't allow wildcarded locators.
	 */
	if (cf->hpibbuscf_slave == HPIBBUSCF_SLAVE_DEFAULT ||
	    cf->hpibbuscf_punit == HPIBBUSCF_PUNIT_DEFAULT)
		return 0;

	return 1;
}

static void
ppiattach(device_t parent, device_t self, void *aux)
{
	struct ppi_softc *sc = device_private(self);
	struct hpibbus_attach_args *ha = aux;

	sc->sc_dev = self;
	aprint_normal("\n");

	sc->sc_slave = ha->ha_slave;

	callout_init(&sc->sc_timo_ch, 0);
	callout_init(&sc->sc_start_ch, 0);

	/* Initialize the hpib queue entry. */
	sc->sc_hq.hq_softc = sc;
	sc->sc_hq.hq_slave = sc->sc_slave;
	sc->sc_hq.hq_start = ppistart;
	sc->sc_hq.hq_go = ppinoop;
	sc->sc_hq.hq_intr = ppinoop;

	sc->sc_flags = PPIF_ALIVE;
}

static void
ppinoop(void *arg)
{
	/* Noop! */
}

int
ppiopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct ppi_softc *sc;

	sc = device_lookup_private(&ppi_cd,UNIT(dev));
	if (sc == NULL)
		return ENXIO;

	if ((sc->sc_flags & PPIF_ALIVE) == 0)
		return ENXIO;

#ifdef DEBUG
	if (ppidebug & PDB_FOLLOW)
		printf("ppiopen(%"PRIx64", %x): flags %x\n",
		       dev, flags, sc->sc_flags);
#endif
	if (sc->sc_flags & PPIF_OPEN)
		return EBUSY;
	sc->sc_flags |= PPIF_OPEN;
	sc->sc_burst = PPI_BURST;
	sc->sc_timo = ppimstohz(PPI_TIMO);
	sc->sc_delay = ppimstohz(PPI_DELAY);
	sc->sc_sec = -1;
	return 0;
}

static int
ppiclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct ppi_softc *sc = device_lookup_private(&ppi_cd, UNIT(dev));

#ifdef DEBUG
	if (ppidebug & PDB_FOLLOW)
		printf("ppiclose(%"PRIx64", %x): flags %x\n",
		       dev, flags, sc->sc_flags);
#endif
	sc->sc_flags &= ~PPIF_OPEN;
	return 0;
}

static void
ppistart(void *arg)
{
	struct ppi_softc *sc = arg;

#ifdef DEBUG
	if (ppidebug & PDB_FOLLOW)
		printf("ppistart(%x)\n", device_unit(sc->sc_dev));
#endif
	sc->sc_flags &= ~PPIF_DELAY;
	wakeup(sc);
}

static void
ppitimo(void *arg)
{
	struct ppi_softc *sc = arg;

#ifdef DEBUG
	if (ppidebug & PDB_FOLLOW)
		printf("ppitimo(%x)\n", device_unit(sc->sc_dev));
#endif
	sc->sc_flags &= ~(PPIF_UIO|PPIF_TIMO);
	wakeup(sc);
}

static int
ppiread(dev_t dev, struct uio *uio, int flags)
{

#ifdef DEBUG
	if (ppidebug & PDB_FOLLOW)
		printf("ppiread(%"PRIx64", %p)\n", dev, uio);
#endif
	return ppirw(dev, uio);
}

static int
ppiwrite(dev_t dev, struct uio *uio, int flags)
{

#ifdef DEBUG
	if (ppidebug & PDB_FOLLOW)
		printf("ppiwrite(%"PRIx64", %p)\n", dev, uio);
#endif
	return ppirw(dev, uio);
}

static int
ppirw(dev_t dev, struct uio *uio)
{
	struct ppi_softc *sc = device_lookup_private(&ppi_cd, UNIT(dev));
	int s, s2, len, cnt;
	char *cp;
	int error = 0, gotdata = 0;
	int buflen, ctlr, slave;
	char *buf;

	if (uio->uio_resid == 0)
		return 0;

	ctlr = device_unit(device_parent(sc->sc_dev));
	slave = sc->sc_slave;

#ifdef DEBUG
	if (ppidebug & (PDB_FOLLOW|PDB_IO))
		printf("ppirw(%"PRIx64", %p, %c): burst %d, timo %d, resid %x\n",
		       dev, uio, uio->uio_rw == UIO_READ ? 'R' : 'W',
		       sc->sc_burst, sc->sc_timo, uio->uio_resid);
#endif
	buflen = min(sc->sc_burst, uio->uio_resid);
	buf = (char *)malloc(buflen, M_DEVBUF, M_WAITOK);
	sc->sc_flags |= PPIF_UIO;
	if (sc->sc_timo > 0) {
		sc->sc_flags |= PPIF_TIMO;
		callout_reset(&sc->sc_timo_ch, sc->sc_timo, ppitimo, sc);
	}
	len = cnt = 0;
	while (uio->uio_resid > 0) {
		len = min(buflen, uio->uio_resid);
		cp = buf;
		if (uio->uio_rw == UIO_WRITE) {
			error = uiomove(cp, len, uio);
			if (error)
				break;
		}
again:
		s = splsoftclock();
		s2 = splbio();
		if ((sc->sc_flags & PPIF_UIO) &&
		    hpibreq(device_parent(sc->sc_dev), &sc->sc_hq) == 0)
			(void) tsleep(sc, PRIBIO + 1, "ppirw", 0);
		/*
		 * Check if we timed out during sleep or uiomove
		 */
		splx(s2);
		if ((sc->sc_flags & PPIF_UIO) == 0) {
#ifdef DEBUG
			if (ppidebug & PDB_IO)
				printf("ppirw: uiomove/sleep timo, flags %x\n",
				       sc->sc_flags);
#endif
			if (sc->sc_flags & PPIF_TIMO) {
				callout_stop(&sc->sc_timo_ch);
				sc->sc_flags &= ~PPIF_TIMO;
			}
			splx(s);
			break;
		}
		splx(s);
		/*
		 * Perform the operation
		 */
		if (uio->uio_rw == UIO_WRITE)
			cnt = hpibsend(ctlr, slave, sc->sc_sec, cp, len);
		else
			cnt = hpibrecv(ctlr, slave, sc->sc_sec, cp, len);
		s = splbio();
		hpibfree(device_parent(sc->sc_dev), &sc->sc_hq);
#ifdef DEBUG
		if (ppidebug & PDB_IO)
			printf("ppirw: %s(%d, %d, %x, %p, %d) -> %d\n",
			       uio->uio_rw == UIO_READ ? "recv" : "send",
			       ctlr, slave, sc->sc_sec, cp, len, cnt);
#endif
		splx(s);
		if (uio->uio_rw == UIO_READ) {
			if (cnt) {
				error = uiomove(cp, cnt, uio);
				if (error)
					break;
				gotdata++;
			}
			/*
			 * Didn't get anything this time, but did in the past.
			 * Consider us done.
			 */
			else if (gotdata)
				break;
		}
		s = splsoftclock();
		/*
		 * Operation timeout (or non-blocking), quit now.
		 */
		if ((sc->sc_flags & PPIF_UIO) == 0) {
#ifdef DEBUG
			if (ppidebug & PDB_IO)
				printf("ppirw: timeout/done\n");
#endif
			splx(s);
			break;
		}
		/*
		 * Implement inter-read delay
		 */
		if (sc->sc_delay > 0) {
			sc->sc_flags |= PPIF_DELAY;
			callout_reset(&sc->sc_start_ch, sc->sc_delay,
			    ppistart, sc);
			error = tsleep(sc, (PCATCH|PZERO) + 1, "hpib", 0);
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
	if (sc->sc_flags & PPIF_TIMO) {
		callout_stop(&sc->sc_timo_ch);
		sc->sc_flags &= ~PPIF_TIMO;
	}
	if (sc->sc_flags & PPIF_DELAY) {
		callout_stop(&sc->sc_start_ch);
		sc->sc_flags &= ~PPIF_DELAY;
	}
	splx(s);
	/*
	 * Adjust for those chars that we uiomove'ed but never wrote
	 */
	if (uio->uio_rw == UIO_WRITE && cnt != len) {
		uio->uio_resid += (len - cnt);
#ifdef DEBUG
		if (ppidebug & PDB_IO)
			printf("ppirw: short write, adjust by %d\n",
			       len-cnt);
#endif
	}
	free(buf, M_DEVBUF);
#ifdef DEBUG
	if (ppidebug & (PDB_FOLLOW|PDB_IO))
		printf("ppirw: return %d, resid %d\n", error, uio->uio_resid);
#endif
	return error;
}

static int
ppiioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ppi_softc *sc = device_lookup_private(&ppi_cd,UNIT(dev));
	struct ppiparam *pp, *upp;
	int error = 0;

	switch (cmd) {
	case PPIIOCGPARAM:
		pp = &sc->sc_param;
		upp = (struct ppiparam *)data;
		upp->burst = pp->burst;
		upp->timo = ppihztoms(pp->timo);
		upp->delay = ppihztoms(pp->delay);
		break;
	case PPIIOCSPARAM:
		pp = &sc->sc_param;
		upp = (struct ppiparam *)data;
		if (upp->burst < PPI_BURST_MIN || upp->burst > PPI_BURST_MAX ||
		    upp->delay < PPI_DELAY_MIN || upp->delay > PPI_DELAY_MAX)
			return EINVAL;
		pp->burst = upp->burst;
		pp->timo = ppimstohz(upp->timo);
		pp->delay = ppimstohz(upp->delay);
		break;
	case PPIIOCSSEC:
		sc->sc_sec = *(int *)data;
		break;
	default:
		return EINVAL;
	}
	return error;
}

static int
ppihztoms(int h)
{
	int m = h;

	if (m > 0)
		m = m * 1000 / hz;
	return m;
}

static int
ppimstohz(int m)
{
	int h = m;

	if (h > 0) {
		h = h * hz / 1000;
		if (h == 0)
			h = 1000 / hz;
	}
	return h;
}
