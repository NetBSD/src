/*	$NetBSD: asc_vsbus.c,v 1.3 2000/03/04 18:20:53 matt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: asc_vsbus.c,v 1.3 2000/03/04 18:20:53 matt Exp $");

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
#include <sys/reboot.h>
#include <sys/queue.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/bus.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <machine/cpu.h>
#include <machine/sid.h>
#include <machine/rpb.h>
#include <machine/scb.h>
#include <machine/vsbus.h>

struct asc_vsbus_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* Must be first */
	bus_space_tag_t sc_bst;			/* bus space tag */
	bus_space_handle_t sc_bsh;		/* bus space handle */
	bus_dma_tag_t sc_dmat;			/* bus dma tag */
	bus_dmamap_t sc_dmamap;
	caddr_t *sc_dmaaddr;
	size_t *sc_dmalen;
	size_t sc_dmasize;
	unsigned int sc_flags;
#define	ASC_DMAACTIVE		0x0001
#define	ASC_ISWRITE		0x0002
#define	ASC_INTR		0x0004
};

#define	ASC_REG_ADR		0x0000
#define	ASC_REG_DIR		0x000C
#define	ASC_REG_NCR		0x0080
#define	ASC_REG_END		0x00B0

#define	ASC_DIR_TO_MEMORY	0x0000
#define	ASC_DIR_FROM_MEMORY	0x0001

#define	ASC_MAXXFERSIZE		65536
#define	ASC_FREQUENCY		20000000

static int asc_vsbus_match __P((struct device *, struct cfdata *, void *));
static void asc_vsbus_attach __P((struct device *, struct device *, void *));
static void ascintr __P((void *));

struct cfattach asc_vsbus_ca = {
	sizeof(struct asc_vsbus_softc), asc_vsbus_match, asc_vsbus_attach
};

static struct scsipi_device asc_vsbus_dev = {
	NULL,			/* Use the default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* use the default done handler */
};

/*
 * Functions and the switch for the MI code
 */
static u_char	asc_vsbus_read_reg __P((struct ncr53c9x_softc *, int));
static void	asc_vsbus_write_reg __P((struct ncr53c9x_softc *, int, u_char));
static int	asc_vsbus_dma_isintr __P((struct ncr53c9x_softc *));
static void	asc_vsbus_dma_reset __P((struct ncr53c9x_softc *));
static int	asc_vsbus_dma_intr __P((struct ncr53c9x_softc *));
static int	asc_vsbus_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
		    size_t *, int, size_t *));
static void	asc_vsbus_dma_go __P((struct ncr53c9x_softc *));
static void	asc_vsbus_dma_stop __P((struct ncr53c9x_softc *));
static int	asc_vsbus_dma_isactive __P((struct ncr53c9x_softc *));
static void	asc_vsbus_clear_latched_intr __P((struct ncr53c9x_softc *));

static struct ncr53c9x_glue asc_vsbus_glue = {
	asc_vsbus_read_reg,
	asc_vsbus_write_reg,
	asc_vsbus_dma_isintr,
	asc_vsbus_dma_reset,
	asc_vsbus_dma_intr,
	asc_vsbus_dma_setup,
	asc_vsbus_dma_go,
	asc_vsbus_dma_stop,
	asc_vsbus_dma_isactive,
	asc_vsbus_clear_latched_intr,
};

static int
asc_vsbus_match( struct device *parent, struct cfdata *cf, void *aux)
{
	struct vsbus_attach_args *va = aux;
	int dummy;
	volatile u_int8_t *ncr_regs;

	if (vax_boardtype != VAX_BTYP_46
	   && vax_boardtype != VAX_BTYP_48
	   && vax_boardtype != VAX_BTYP_49)
		return 0;

	ncr_regs = (volatile u_int8_t *) va->va_addr;

	/*  *** need to generate an interrupt here
	 * From trial and error, I've determined that an INT is generated
	 * only when the following sequence of events occurs:
	 *   1) The interrupt status register (0x05) must be read.
	 *   2) SCSI bus reset interrupt must be enabled
	 *   3) SCSI bus reset command must be sent
	 *   4) NOP command must be sent
	 */

	dummy = ncr_regs[NCR_INTR << 2] & 0xFF;
        ncr_regs[NCR_CFG1 << 2] = 0x07; /* we're ID 7, turn on INT for SCSI reset */
        ncr_regs[NCR_CMD << 2] = NCRCMD_RSTSCSI; /* send the reset */
        ncr_regs[NCR_CMD << 2] = NCRCMD_NOP; /* send a NOP */
	DELAY(10000);

	dummy = ncr_regs[NCR_INTR << 2] & 0xFF;
	return (dummy & NCRINTR_SBR) != 0;
}


/*
 * Attach this instance, and then all the sub-devices
 */
static void
asc_vsbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct asc_vsbus_softc *asc = (void *)self;
	struct ncr53c9x_softc *sc = &asc->sc_ncr53c9x;
	int error;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &asc_vsbus_glue;

	asc->sc_bst = va->va_iot;
	asc->sc_dmat = va->va_dmat;

	error = bus_space_map(asc->sc_bst, va->va_paddr - ASC_REG_NCR,
	    ASC_REG_END, 0, &asc->sc_bsh);
	if (error) {
		printf(": failed to map registers: error=%d\n", error);
		return;
	}

	sc->sc_id = 7;	/* XXX need to get this from VMB */
	sc->sc_freq = ASC_FREQUENCY;

	/* gimme Mhz */
	sc->sc_freq /= 1000000;

	scb_vecalloc(va->va_cvec, ascintr, asc, SCB_ISTACK);

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
	sc->sc_minsync = (1000 / sc->sc_freq);
	sc->sc_maxxfer = 64 * 1024;

	printf("\n%s", self->dv_xname);	/* Pretty print */

	/* Do the common parts of attachment. */
	sc->sc_adapter.scsipi_cmd = ncr53c9x_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = minphys;
	ncr53c9x_attach(sc, &asc_vsbus_dev);

	/*
	 * Register this device as boot device if we booted from it.
	 * This will fail if there are more than one le in a machine,
	 * fortunately there may be only one.
	 */
	if (B_TYPE(bootdev) == BDEV_SD)
		booted_from = self;
}

static void
ascintr(void *arg)
{
	struct asc_vsbus_softc *asc = arg;
	asc->sc_flags |= ASC_INTR;
	ncr53c9x_intr(&asc->sc_ncr53c9x);
	asc->sc_flags &= ~ASC_INTR;
}

/*
 * Glue functions.
 */

static u_char
asc_vsbus_read_reg(struct ncr53c9x_softc *sc, int reg)
{
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
	u_char v;

	v = bus_space_read_1(asc->sc_bst, asc->sc_bsh,
	    ASC_REG_NCR + reg * sizeof(u_int32_t));

	return (v);
}

static void
asc_vsbus_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;

	bus_space_write_1(asc->sc_bst, asc->sc_bsh,
	    ASC_REG_NCR + reg * sizeof(u_int32_t), val);
}

static int
asc_vsbus_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
	int isintr = (asc->sc_flags & ASC_INTR);
	asc->sc_flags &= ~ASC_INTR;
	return isintr;		/* this is the only reason */
}

static void
asc_vsbus_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
#if 0
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
#endif

	/* nothing to do */
}

static int
asc_vsbus_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
#if 0
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
#endif

	return 0;
}

static int
asc_vsbus_dma_setup(struct ncr53c9x_softc *sc, caddr_t *addr, size_t *len,
		    int datain, size_t *dmasize)
{
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;

	asc->sc_dmaaddr = addr;
	asc->sc_dmalen = len;
	if (datain) {
		asc->sc_flags |= ASC_ISWRITE;
	} else {
		asc->sc_flags &= ~ASC_ISWRITE;
	}

        NCR_DMA(("%s: start %d@%p,%d\n", sc->sc_dev.dv_xname,
                (int)*asc->sc_dmalen, *asc->sc_dmaaddr, (asc->sc_flags & ASC_ISWRITE)));
	*dmasize = asc->sc_dmasize = min(*dmasize, ASC_MAXXFERSIZE);

	if (asc->sc_dmasize) {
		if (bus_dmamap_load(asc->sc_dmat, asc->sc_dmamap,
				*asc->sc_dmaaddr, asc->sc_dmasize,
				NULL /* kernel address */,   
				BUS_DMA_NOWAIT))
			panic("%s: cannot load dma map", sc->sc_dev.dv_xname);
		bus_dmamap_sync(asc->sc_dmat, asc->sc_dmamap,
				0, asc->sc_dmasize, datain
					? BUS_DMASYNC_PREREAD
					: BUS_DMASYNC_PREWRITE);
		bus_space_write_4(asc->sc_bst, asc->sc_bsh, ASC_REG_ADR,
				  asc->sc_dmamap->dm_segs[0].ds_addr);
		bus_space_write_4(asc->sc_bst, asc->sc_bsh, ASC_REG_DIR,
				  datain ? 0 : 1);
	}


	return 0;
}

static void
asc_vsbus_dma_go(struct ncr53c9x_softc *sc)
{
#if 0
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
#endif

	/* Nothing to do */
}

static void
asc_vsbus_dma_stop(struct ncr53c9x_softc *sc)
{
#if 0
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
#endif

	/*
	 * XXX STOP DMA HERE!
	 */
}

static int
asc_vsbus_dma_isactive(struct ncr53c9x_softc *sc)
{
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;

	return (asc->sc_flags & ASC_DMAACTIVE) != 0;
}

static void
asc_vsbus_clear_latched_intr(struct ncr53c9x_softc *sc)
{
#if 0
	struct asc_vsbus_softc *asc = (struct asc_vsbus_softc *)sc;
#endif
}
