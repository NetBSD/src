/*	$NetBSD: i82072.c,v 1.3.16.1 2002/05/17 15:40:55 gehenna Exp $	*/
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/mainboard.h>
#include <machine/bus.h>

dev_type_open(fdopen);
dev_type_strategy(fdstrategy);

const struct bdevsw fd_bdevsw = {
	fdopen, nullclose, fdstrategy, noioctl, nodump, nosize, D_DISK
};

const struct cdevsw fd_cdevsw = {
	fdopen, nullclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, D_DISK
};

#define	I82072_STATUS	0x000003
#define	I82072_DATA	0x000007
#define	I82072_TC	0x800003

struct	fd_softc {
        struct  device dev; 
        struct  evcnt  fd_intrcnt;
	bus_space_tag_t	fd_bst;
	bus_space_handle_t fd_bsh;
        int     unit;
};

static int	fd_match (struct device *, struct cfdata *, void *);
static void	fd_attach (struct device *, struct device *, void *);
static void     fd_reset (struct fd_softc *);

struct cfattach fd_ca = {
	sizeof(struct fd_softc), fd_match, fd_attach
};

static int	fd_intr (void *);

int
fd_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
fd_attach(struct device *parent, struct device *self, void *aux)
{
        struct fd_softc *sc = (void *)self;
	struct confargs *ca = aux;

	sc->fd_bst = ca->ca_bustag;
	if (bus_space_map(ca->ca_bustag, ca->ca_addr,
			  0x1000000,	
			  BUS_SPACE_MAP_LINEAR,
			  &sc->fd_bsh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	evcnt_attach_dynamic(&sc->fd_intrcnt, EVCNT_TYPE_INTR, NULL,
			     self->dv_xname, "intr");

	bus_intr_establish(sc->fd_bst, SYS_INTR_FDC, 0, 0, fd_intr, sc);

	fd_reset(sc);
	printf(": not fully implemented\n");
}

void
fd_reset(struct fd_softc *sc)
{
	/* This clears any pending interrupts from the i82072 FDC */
	bus_space_write_1(sc->fd_bst, sc->fd_bsh, I82072_STATUS, 0x80);
	DELAY(1000);
	bus_space_write_1(sc->fd_bst, sc->fd_bsh, I82072_TC, 0x01);
	DELAY(1000);
}

int
fdopen(dev_t dev, int flags, int mode, struct proc *p)
{
	return (EBADF);
}

void
fdstrategy(struct buf *bp)
{
	panic("fdstrategy");
}

static int
fd_intr(arg)
	void *arg;
{
	struct fd_softc *sc = arg;

	sc->fd_intrcnt.ev_count++;
	fd_reset(sc);
	return 0;
}
