/* $NetBSD: asc.c,v 1.17 2000/06/05 07:59:50 nisimura Exp $ */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center;  Paul Kranenburg.
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
 * Copyright (c) 1994 Peter Galbavy
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
 *	This product includes software developed by Peter Galbavy
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Based on aic6360 by Jarle Greipsland
 *
 * Acknowledgements: Many of the algorithms used in this driver are
 * inspired by the work of Julian Elischer (julian@tfs.com) and
 * Charles Hannum (mycroft@duality.gnu.ai.mit.edu).  Thanks a million!
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: asc.c,v 1.17 2000/06/05 07:59:50 nisimura Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <dev/tc/tcvar.h>
#include <alpha/tc/tcdsvar.h>
#include <alpha/tc/ascvar.h>

int	asc_tcds_match	__P((struct device *, struct cfdata *, void *));
void	asc_tcds_attach	__P((struct device *, struct device *, void *));

/* Linkup to the rest of the kernel */
struct cfattach asc_tcds_ca = {
	sizeof(struct asc_tcds_softc), asc_tcds_match, asc_tcds_attach
};

/*
 * Functions and the switch for the MI code.
 */
u_char	asc_tcds_read_reg __P((struct ncr53c9x_softc *, int));
void	asc_tcds_write_reg __P((struct ncr53c9x_softc *, int, u_char));
int	asc_tcds_dma_isintr __P((struct ncr53c9x_softc *));
void	asc_tcds_dma_reset __P((struct ncr53c9x_softc *));
int	asc_tcds_dma_intr __P((struct ncr53c9x_softc *));
int	asc_tcds_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *));
void	asc_tcds_dma_go __P((struct ncr53c9x_softc *));
void	asc_tcds_dma_stop __P((struct ncr53c9x_softc *));
int	asc_tcds_dma_isactive __P((struct ncr53c9x_softc *));
void	asc_tcds_clear_latched_intr __P((struct ncr53c9x_softc *));

struct ncr53c9x_glue asc_tcds_glue = {
	asc_tcds_read_reg,
	asc_tcds_write_reg,
	asc_tcds_dma_isintr,
	asc_tcds_dma_reset,
	asc_tcds_dma_intr,
	asc_tcds_dma_setup,
	asc_tcds_dma_go,
	asc_tcds_dma_stop,
	asc_tcds_dma_isactive,
	asc_tcds_clear_latched_intr,
};

int
asc_tcds_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* We always exist. */
	return (1);
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
asc_tcds_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct tcdsdev_attach_args *tcdsdev = aux;
	struct asc_tcds_softc *asc = (void *)self;
	struct ncr53c9x_softc *sc = &asc->sc_ncr53c9x;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &asc_tcds_glue;

	asc->sc_bst = tcdsdev->tcdsda_bst;
	asc->sc_bsh = tcdsdev->tcdsda_bsh;
	asc->sc_dma = tcdsdev->tcdsda_sc;

	sc->sc_id = tcdsdev->tcdsda_id;
	sc->sc_freq = tcdsdev->tcdsda_freq;

	/* gimme Mhz */
	sc->sc_freq /= 1000000;

	asc->sc_dma->sc_asc = asc;			/* XXX */

	tcds_intr_establish(parent, tcdsdev->tcdsda_chip, ncr53c9x_intr, sc);

	/*
	 * XXX More of this should be in ncr53c9x_attach(), but
	 * XXX should we really poke around the chip that much in
	 * XXX the MI code?  Think about this more...
	 */

	/*
	 * Set up static configuration info.
	 */
	sc->sc_cfg1 = sc->sc_id | NCRCFG1_PARENB;
	sc->sc_cfg2 = NCRCFG2_SCSI2;
	sc->sc_cfg3 = NCRCFG3_CDB;
	if (sc->sc_freq > 25)
		sc->sc_cfg3 |= NCRF9XCFG3_FCLK;
	sc->sc_rev = tcdsdev->tcdsda_variant;
	if (tcdsdev->tcdsda_fast) {
		sc->sc_features |= NCR_F_FASTSCSI;
		sc->sc_cfg3_fscsi = NCRF9XCFG3_FSCSI;
	}

	/*
	 * XXX minsync and maxxfer _should_ be set up in MI code,
	 * XXX but it appears to have some dependency on what sort
	 * XXX of DMA we're hooked up to, etc.
	 */

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	sc->sc_minsync = (1000 / sc->sc_freq) * tcdsdev->tcdsda_period / 4;

	sc->sc_maxxfer = 64 * 1024;

	/* Do the common parts of attachment. */
	ncr53c9x_attach(sc, NULL, NULL);
}

/*
 * Glue functions.
 */

u_char
asc_tcds_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;
	u_char v;

	v = bus_space_read_4(asc->sc_bst, asc->sc_bsh,
	    reg * sizeof(u_int32_t)) & 0xff;

	return (v);
}

void
asc_tcds_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	bus_space_write_4(asc->sc_bst, asc->sc_bsh,
	    reg * sizeof(u_int32_t), val);
}

int
asc_tcds_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	return (tcds_dma_isintr(asc->sc_dma));
}

void
asc_tcds_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	tcds_dma_reset(asc->sc_dma);
}

int
asc_tcds_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	return (tcds_dma_intr(asc->sc_dma));
}

int
asc_tcds_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	return (tcds_dma_setup(asc->sc_dma, addr, len, datain, dmasize));
}

void
asc_tcds_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	tcds_dma_go(asc->sc_dma);
}

void
asc_tcds_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
#if 0
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;
#endif

	/*
	 * XXX STOP DMA HERE!
	 */
}

int
asc_tcds_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	return (tcds_dma_isactive(asc->sc_dma));
}

void
asc_tcds_clear_latched_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_tcds_softc *asc = (struct asc_tcds_softc *)sc;

	/* Clear the TCDS interrupt bit. */
	(void)tcds_scsi_isintr(asc->sc_dma, 1);
}
