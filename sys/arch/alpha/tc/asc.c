/* $NetBSD: asc.c,v 1.3 1997/04/06 22:31:45 cgd Exp $ */

/*
 * Copyright (c) 1997 Jason R. Thorpe.
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
 *	This product includes software developed for the NetBSD Project
 *	by Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Copyright (c) 1994 Peter Galbavy
 * Copyright (c) 1995 Paul Kranenburg
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

#include <machine/options.h>		/* Pull in config options headers */

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

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <scsi/scsi_message.h>

#include <machine/cpu.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <dev/tc/tcvar.h>
#include <alpha/tc/tcdsvar.h>
#include <alpha/tc/ascvar.h>

void	ascattach	__P((struct device *, struct device *, void *));
int	ascmatch	__P((struct device *, struct cfdata *, void *));

/* Linkup to the rest of the kernel */
struct cfattach asc_ca = {
	sizeof(struct asc_softc), ascmatch, ascattach
};

struct cfdriver asc_cd = {
	NULL, "asc", DV_DULL
};

struct scsi_adapter asc_switch = {
	ncr53c9x_scsi_cmd,
	minphys,		/* no max at this level; handled by DMA code */
	NULL,
	NULL,
};

struct scsi_device asc_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

/*
 * Functions and the switch for the MI code.
 */
u_char	asc_read_reg __P((struct ncr53c9x_softc *, int));
void	asc_write_reg __P((struct ncr53c9x_softc *, int, u_char));
int	asc_dma_isintr __P((struct ncr53c9x_softc *));
void	asc_dma_reset __P((struct ncr53c9x_softc *));
int	asc_dma_intr __P((struct ncr53c9x_softc *));
int	asc_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *));
void	asc_dma_go __P((struct ncr53c9x_softc *));
void	asc_dma_stop __P((struct ncr53c9x_softc *));
int	asc_dma_isactive __P((struct ncr53c9x_softc *));
void	asc_clear_latched_intr __P((struct ncr53c9x_softc *));

struct ncr53c9x_glue asc_glue = {
	asc_read_reg,
	asc_write_reg,
	asc_dma_isintr,
	asc_dma_reset,
	asc_dma_intr,
	asc_dma_setup,
	asc_dma_go,
	asc_dma_stop,
	asc_dma_isactive,
	asc_clear_latched_intr,
};

int
ascmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct tcdsdev_attach_args *tcdsdev = aux;

	if (strncmp(tcdsdev->tcdsda_modname, "PMAZ-AA ", TC_ROM_LLEN))
		return (0);
	return (!tc_badaddr(tcdsdev->tcdsda_addr));
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
ascattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct tcdsdev_attach_args *tcdsdev = aux;
	struct asc_softc *asc = (void *)self;
	struct ncr53c9x_softc *sc = &asc->sc_ncr53c9x;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &asc_glue;

	asc->sc_reg = (volatile u_int32_t *)tcdsdev->tcdsda_addr;
	asc->sc_cookie = tcdsdev->tcdsda_cookie;
	asc->sc_dma = tcdsdev->tcdsda_sc;

	tcds_intr_establish(parent, asc->sc_cookie, TC_IPL_BIO,
	    (int (*)(void *))ncr53c9x_intr, sc);

	if (parent->dv_cfdata->cf_driver == &tcds_cd) {
		sc->sc_id = tcdsdev->tcdsda_id;
		sc->sc_freq = tcdsdev->tcdsda_freq;
	} else {
		/* XXX */
		sc->sc_id = 7;
		sc->sc_freq = 24000000;
	}

	/* gimme Mhz */
	sc->sc_freq /= 1000000;

	asc->sc_dma->sc_asc = asc;			/* XXX */

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
	sc->sc_cfg3 = 0x4;		/* Save residual byte. XXX??? */
	sc->sc_rev = NCR_VARIANT_NCR53C94;

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
	sc->sc_minsync = 1000 / sc->sc_freq;

	sc->sc_maxxfer = 64 * 1024;

	/* Do the common parts of attachment. */
	ncr53c9x_attach(sc, &asc_switch, &asc_dev);
}

/*
 * Glue functions.
 */

u_char
asc_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	u_char v;

	v = asc->sc_reg[reg * 2] & 0xff;
#if 1
	alpha_mb();
#endif
	return (v);
}

void
asc_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	asc->sc_reg[reg * 2] = val;
	alpha_mb();
}

int
asc_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return (tcds_dma_isintr(asc->sc_dma));
}

void
asc_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	tcds_dma_reset(asc->sc_dma);
}

int
asc_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return (tcds_dma_intr(asc->sc_dma));
}

int
asc_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return (tcds_dma_setup(asc->sc_dma, addr, len, datain, dmasize));
}

void
asc_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	tcds_dma_go(asc->sc_dma);
}

void
asc_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
#if 0
	struct asc_softc *asc = (struct asc_softc *)sc;
#endif

	/*
	 * XXX STOP DMA HERE!
	 */
}

int
asc_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return (tcds_dma_isactive(asc->sc_dma));
}

void
asc_clear_latched_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	/* Clear the TCDS interrupt bit. */
	(void)tcds_scsi_isintr(asc->sc_dma, 1);
}
