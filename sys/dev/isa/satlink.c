/*	$NetBSD: satlink.c,v 1.29 2006/10/12 01:31:17 christos Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Canada Connect Corp.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Device driver for SatLink interface.
 *
 * This thing is really simple.  We essentially DMA into a ring buffer
 * which the user then reads from, and provide an ioctl interface to
 * reset the card, etc.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: satlink.c,v 1.29 2006/10/12 01:31:17 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/tty.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/satlinkreg.h>
#include <dev/isa/satlinkio.h>

struct satlink_softc {
	struct	device sc_dev;		/* device glue */
	bus_space_tag_t sc_iot;		/* space tag */
	bus_space_handle_t sc_ioh;	/* space handle */
	isa_chipset_tag_t sc_ic;	/* ISA chipset info */
	int	sc_drq;			/* the DRQ we're using */
	bus_size_t sc_bufsize;		/* DMA buffer size */
	caddr_t	sc_buf;			/* ring buffer for incoming data */
	int	sc_uptr;		/* user index into ring buffer */
	int	sc_sptr;		/* satlink index into ring buffer */
	int	sc_flags;		/* misc. flags. */
	int	sc_lastresid;		/* residual */
	struct selinfo sc_selq;		/* our select/poll queue */
	struct	satlink_id sc_id;	/* ID cached at attach time */
	struct callout sc_ch;		/* callout pseudo-interrupt */
};

/* sc_flags */
#define	SATF_ISOPEN		0x01	/* device is open */
#define	SATF_DATA		0x02	/* waiting for data */

/*
 * Our pesudo-interrupt.  Since up to 328 bytes can arrive in 1/100 of
 * a second, this gives us 3280 bytes per timeout.
 */
#define	SATLINK_TIMEOUT		(hz/10)

int	satlinkprobe(struct device *, struct cfdata *, void *);
void	satlinkattach(struct device *, struct device *, void *);
void	satlinktimeout(void *);

CFATTACH_DECL(satlink, sizeof(struct satlink_softc),
    satlinkprobe, satlinkattach, NULL, NULL);

extern struct cfdriver satlink_cd;

dev_type_open(satlinkopen);
dev_type_close(satlinkclose);
dev_type_read(satlinkread);
dev_type_ioctl(satlinkioctl);
dev_type_poll(satlinkpoll);
dev_type_kqfilter(satlinkkqfilter);

const struct cdevsw satlink_cdevsw = {
	satlinkopen, satlinkclose, satlinkread, nowrite, satlinkioctl,
	nostop, notty, satlinkpoll, nommap, satlinkkqfilter, D_OTHER,
};

int
satlinkprobe(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int rv = 0;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_ndrq < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* Don't allow wildcarding of iobase or drq. */
	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return (0);
	if (ia->ia_drq[0].ir_drq == ISA_UNKNOWN_DRQ)
		return (0);

	if (bus_space_map(iot, ia->ia_io[0].ir_addr, SATLINK_IOSIZE, 0, &ioh))
		return (0);

	/*
	 * XXX Should check manufacturer ID here, or something.
	 */

	rv = 1;

	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = SATLINK_IOSIZE;

	ia->ia_ndrq = 1;

	ia->ia_nirq = 0;
	ia->ia_niomem = 0;

	bus_space_unmap(iot, ioh, SATLINK_IOSIZE);
	return (rv);
}

void
satlinkattach(struct device *parent __unused, struct device *self, void *aux)
{
	struct satlink_softc *sc = (struct satlink_softc *)self;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	bus_addr_t ringaddr;

	printf("\n");

	/* Map the card. */
	if (bus_space_map(iot, ia->ia_io[0].ir_addr, SATLINK_IOSIZE, 0, &ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_ic = ia->ia_ic;
	sc->sc_drq = ia->ia_drq[0].ir_drq;

	/* Reset the card. */
	bus_space_write_1(iot, ioh, SATLINK_COMMAND, SATLINK_CMD_RESET);

	/* Read ID from the card. */
	sc->sc_id.sid_mfrid =
	    bus_space_read_1(iot, ioh, SATLINK_MFRID_L) |
	    (bus_space_read_1(iot, ioh, SATLINK_MFRID_H) << 8);
	sc->sc_id.sid_grpid = bus_space_read_1(iot, ioh, SATLINK_GRPID);
	sc->sc_id.sid_userid =
	    bus_space_read_1(iot, ioh, SATLINK_USERID_L) |
	    (bus_space_read_1(iot, ioh, SATLINK_USERID_H) << 8);
	sc->sc_id.sid_serial =
	    bus_space_read_1(iot, ioh, SATLINK_SER_L) |
	    (bus_space_read_1(iot, ioh, SATLINK_SER_M0) << 8) |
	    (bus_space_read_1(iot, ioh, SATLINK_SER_M1) << 16) |
	    (bus_space_read_1(iot, ioh, SATLINK_SER_H) << 24);

	printf("%s: mfrid 0x%x, grpid 0x%x, userid 0x%x, serial %d\n",
	    sc->sc_dev.dv_xname, sc->sc_id.sid_mfrid,
	    sc->sc_id.sid_grpid, sc->sc_id.sid_userid,
	    sc->sc_id.sid_serial);

	callout_init(&sc->sc_ch);

	sc->sc_bufsize = isa_dmamaxsize(sc->sc_ic, sc->sc_drq);

	/* Allocate and map the ring buffer. */
	if (isa_dmamem_alloc(sc->sc_ic, sc->sc_drq, sc->sc_bufsize,
	    &ringaddr, BUS_DMA_NOWAIT)) {
		printf("%s: can't allocate ring buffer\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (isa_dmamem_map(sc->sc_ic, sc->sc_drq, ringaddr, sc->sc_bufsize,
	    &sc->sc_buf, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		printf("%s: can't map ring buffer\n", sc->sc_dev.dv_xname);
		isa_dmamem_free(sc->sc_ic, sc->sc_drq, ringaddr,
		    sc->sc_bufsize);
		return;
	}

	if (isa_drq_alloc(sc->sc_ic, sc->sc_drq) != 0) {
		printf("%s: can't reserve drq %d\n",
		    sc->sc_dev.dv_xname, sc->sc_drq);
		isa_dmamem_unmap(sc->sc_ic, sc->sc_drq, sc->sc_buf,
		    sc->sc_bufsize);
		isa_dmamem_free(sc->sc_ic, sc->sc_drq, ringaddr,
		    sc->sc_bufsize);
		return;
	}

	/* Create the DMA map. */
	if (isa_dmamap_create(sc->sc_ic, sc->sc_drq, sc->sc_bufsize,
	    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW)) {
		printf("%s: can't create DMA map\n", sc->sc_dev.dv_xname);
		isa_dmamem_unmap(sc->sc_ic, sc->sc_drq, sc->sc_buf,
		    sc->sc_bufsize);
		isa_dmamem_free(sc->sc_ic, sc->sc_drq, ringaddr,
		    sc->sc_bufsize);
		return;
	}
}

int
satlinkopen(dev_t dev, int flags __unused, int fmt __unused,
    struct lwp *l __unused)
{
	struct satlink_softc *sc;
	int error;

	sc = device_lookup(&satlink_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);

	if (sc->sc_flags & SATF_ISOPEN)
		return (EBUSY);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SATLINK_COMMAND,
		    SATLINK_CMD_RESET);

	/* Reset the ring buffer, and start the DMA loop. */
	sc->sc_uptr = 0;
	sc->sc_sptr = 0;
	sc->sc_lastresid = sc->sc_bufsize;
	memset(sc->sc_buf, 0, sc->sc_bufsize);
	error = isa_dmastart(sc->sc_ic, sc->sc_drq, sc->sc_buf,
	    sc->sc_bufsize, NULL, DMAMODE_READ|DMAMODE_LOOP, BUS_DMA_WAITOK);
	if (error)
		return (error);

	sc->sc_flags |= SATF_ISOPEN;

	callout_reset(&sc->sc_ch, SATLINK_TIMEOUT, satlinktimeout, sc);

	return (0);
}

int
satlinkclose(dev_t dev, int flags __unused, int fmt __unused,
    struct lwp *l __unused)
{
	struct satlink_softc *sc = device_lookup(&satlink_cd, minor(dev));
	int s;

	s = splsoftclock();
	sc->sc_flags &= ~SATF_ISOPEN;
	splx(s);

	isa_dmaabort(sc->sc_ic, sc->sc_drq);
	callout_stop(&sc->sc_ch);

	return (0);
}

int
satlinkread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct satlink_softc *sc = device_lookup(&satlink_cd, minor(dev));
	int error, s, count, sptr;
	int wrapcnt, oresid;

	s = splsoftclock();

	/* Wait for data to be available. */
	while (sc->sc_sptr == sc->sc_uptr) {
		if (flags & O_NONBLOCK) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sc->sc_flags |= SATF_DATA;
		if ((error = tsleep(sc, TTIPRI | PCATCH, "satio", 0)) != 0) {
			splx(s);
			return (error);
		}
	}

	sptr = sc->sc_sptr;
	splx(s);

	/* Compute number of readable bytes. */
	if (sptr > sc->sc_uptr)
		count = sptr - sc->sc_uptr;
	else
		count = sc->sc_bufsize - sc->sc_uptr + sptr;

	if (count > uio->uio_resid)
		count = uio->uio_resid;

	/* Send data out to user. */
	if (sptr > sc->sc_uptr) {
		/*
		 * Easy case - no wrap-around.
		 */
		error = uiomove(&sc->sc_buf[sc->sc_uptr], count, uio);
		if (error == 0) {
			sc->sc_uptr += count;
			if (sc->sc_uptr == sc->sc_bufsize)
				sc->sc_uptr = 0;
		}
		return (error);
	}

	/*
	 * We wrap around.  Copy to the end of the ring...
	 */
	wrapcnt = sc->sc_bufsize - sc->sc_uptr;
	oresid = uio->uio_resid;
	if (wrapcnt > uio->uio_resid)
		wrapcnt = uio->uio_resid;
	error = uiomove(&sc->sc_buf[sc->sc_uptr], wrapcnt, uio);
	sc->sc_uptr = 0;
	if (error != 0 || wrapcnt == oresid)
		return (error);

	/* ...and the rest. */
	count -= wrapcnt;
	error = uiomove(sc->sc_buf, count, uio);
	sc->sc_uptr += count;
	if (sc->sc_uptr == sc->sc_bufsize)
		sc->sc_uptr = 0;

	return (error);
}

int
satlinkioctl(dev_t dev, u_long cmd, caddr_t data, int flags __unused,
    struct lwp *l __unused)
{
	struct satlink_softc *sc = device_lookup(&satlink_cd, minor(dev));

	switch (cmd) {
	case SATIORESET:
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, SATLINK_COMMAND,
		    SATLINK_CMD_RESET);
		sc->sc_uptr = isa_dmacount(sc->sc_ic, sc->sc_drq);
		sc->sc_sptr = sc->sc_uptr;
		break;

	case SATIOGID:
		memcpy(data, &sc->sc_id, sizeof(sc->sc_id));
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

int
satlinkpoll(dev, events, l)
	dev_t dev;
	int events;
	struct lwp *l;
{
	struct satlink_softc *sc = device_lookup(&satlink_cd, minor(dev));
	int s, revents;

	revents = events & (POLLOUT | POLLWRNORM);

	/* Attempt to save some work. */
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	/* We're timeout-driven, so must block the clock. */
	s = splsoftclock();
	if (sc->sc_uptr != sc->sc_sptr)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(l, &sc->sc_selq);
	splx(s);

	return (revents);
}

static void
filt_satlinkrdetach(struct knote *kn)
{
	struct satlink_softc *sc = kn->kn_hook;
	int s;

	s = splsoftclock();
	SLIST_REMOVE(&sc->sc_selq.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_satlinkread(struct knote *kn, long hint __unused)
{
	struct satlink_softc *sc = kn->kn_hook;

	if (sc->sc_uptr == sc->sc_sptr)
		return (0);

	if (sc->sc_sptr > sc->sc_uptr)
		kn->kn_data = sc->sc_sptr - sc->sc_uptr;
	else
		kn->kn_data = (sc->sc_bufsize - sc->sc_uptr) +
		    sc->sc_sptr;
	return (1);
}

static const struct filterops satlinkread_filtops =
	{ 1, NULL, filt_satlinkrdetach, filt_satlinkread };

static const struct filterops satlink_seltrue_filtops =
	{ 1, NULL, filt_satlinkrdetach, filt_seltrue };

int
satlinkkqfilter(dev_t dev, struct knote *kn)
{
	struct satlink_softc *sc = device_lookup(&satlink_cd, minor(dev));
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_selq.sel_klist;
		kn->kn_fop = &satlinkread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sc->sc_selq.sel_klist;
		kn->kn_fop = &satlink_seltrue_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = sc;

	s = splsoftclock();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

void
satlinktimeout(arg)
	void *arg;
{
	struct satlink_softc *sc = arg;
	bus_size_t resid;
	int newidx;

	if ((sc->sc_flags & SATF_ISOPEN) == 0)
		return;

	/*
	 * Get the current residual count from the DMA controller
	 * and compute the satlink's index into the ring buffer.
	 */
	resid = isa_dmacount(sc->sc_ic, sc->sc_drq);
	newidx = sc->sc_bufsize - resid;
	if (newidx == sc->sc_bufsize)
		newidx = 0;

	if (newidx == sc->sc_sptr)
		goto out;

	sc->sc_sptr = newidx;

	/* Wake up anyone blocked in read... */
	if (sc->sc_flags & SATF_DATA) {
		sc->sc_flags &= ~SATF_DATA;
		wakeup(sc);
	}

	/* Wake up anyone blocked in poll... */
	selnotify(&sc->sc_selq, 0);

 out:
	callout_reset(&sc->sc_ch, SATLINK_TIMEOUT, satlinktimeout, sc);
}
