/*	$NetBSD: asc_ioasic.c,v 1.20 2000/03/04 05:43:51 mhitch Exp $	*/

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */
#define USE_CACHED_BUFFER 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <dev/tc/ioasicreg.h>

#include <pmax/dev/device.h>		/* XXX */
#include <pmax/dev/scsi.h>		/* XXX */

#include <pmax/dev/ascreg.h>
#include <dev/tc/ascvar.h>

extern paddr_t kvtophys __P((vaddr_t));

/*
 * Autoconfiguration data for config.
 */
int asc_ioasic_match __P((struct device *, struct cfdata *, void *));
void asc_ioasic_attach __P((struct device *, struct device *, void *));

struct cfattach asc_ioasic_ca = {
	sizeof(struct asc_softc), asc_ioasic_match, asc_ioasic_attach
};

/*
 * DMA callback declarations
 */

static int
asic_dma_start __P((asc_softc_t asc, State *state, caddr_t cp, int flag,
    int len, int off));

static void
asic_dma_end __P((asc_softc_t asc, State *state, int flag));

int
asc_ioasic_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;

	if (strncmp(d->iada_modname, "asc", TC_ROM_LLEN) != 0)
		return (0);

	return (1);
}

void
asc_ioasic_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	asc_softc_t asc = (asc_softc_t)self;
	struct ioasicdev_attach_args *d = aux;
/* XXX Hook for dk_establish() to determine boot device */
	extern int booted_slot;
	extern struct device *booted_controller;

	/* Is this the controller we booted from? */
	if (booted_slot == 3)
		booted_controller = self;
/* XXX */

	asc->sc_bst = ((struct ioasic_softc *)parent)->sc_bst;
	asc->sc_bsh = ((struct ioasic_softc *)parent)->sc_bsh;
	asc->sc_dmat = ((struct ioasic_softc *)parent)->sc_dmat;
	if (bus_space_subregion(asc->sc_bst,
			asc->sc_bsh,
			IOASIC_SLOT_12_START, 0x100, &asc->sc_scsi_bsh)) {
		printf("%s: unable to map device\n", asc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Initialize hw descriptor, cache some pointers
	 */
	asc->regs = (asc_regmap_t *)(MIPS_PHYS_TO_KSEG1(d->iada_addr) + ASC_OFFSET_53C94);

	/*
	 * Set up machine dependencies.
	 * (1) how to do dma
	 * (2) timing based on turbochannel frequency
	 */
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_DMAPTR, -1);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_NEXTPTR, -1);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_SCR, 0);

	asc->dma_start = asic_dma_start;
	asc->dma_end = asic_dma_end;

	/* digital meters show IOASIC 53c94s are clocked at approx 25MHz */
	ascattach(asc, ASC_SPEED_25_MHZ);

	/* tie pseudo-slot to device */

	ioasic_intr_establish(parent, d->iada_cookie, IPL_BIO, asc_intr, asc);
}


/*
 * DMA handling routines. For a turbochannel device, just set the dmar.
 * For the I/O ASIC, handle the actual DMA interface.
 */
static int
asic_dma_start(asc, state, cp, flag, len, off)
	asc_softc_t asc;
	State *state;
	caddr_t cp;
	int flag;
	int len;
	int off;
{
	u_int32_t ssr, phys, nphys;

	/* stop DMA engine first */
	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_SCR, 0);

	/* restrict len to the maximum the IOASIC can transfer */
	if (len > ((caddr_t)mips_trunc_page(cp + NBPG * 2) - cp))
		len = (caddr_t)mips_trunc_page(cp + NBPG * 2) - cp;

	if ((vaddr_t)cp & 7) {
		u_int32_t *p;
		u_int32_t scrval;

		p = (u_int32_t *)((vaddr_t)cp & ~7);
		bus_space_write_4(asc->sc_bst, asc->sc_bsh,
						IOASIC_SCSI_SDR0, p[0]);
		bus_space_write_4(asc->sc_bst, asc->sc_bsh,
						IOASIC_SCSI_SDR1, p[1]);
		scrval = ((vaddr_t)cp >> 1) & 3;
		cp = (caddr_t)((vaddr_t)cp & ~7);
		if (flag != ASCDMA_READ) {
			scrval |= 4;
			cp += 8;
		}
		bus_space_write_4(asc->sc_bst, asc->sc_bsh,
						IOASIC_SCSI_SCR, scrval);
	}

	/* If R4K, writeback and invalidate  the buffer */
	if (CPUISMIPS3)
		mips3_HitFlushDCache((vaddr_t)cp, len);

	/* Get physical address of buffer start, no next phys addr */
	phys = (u_int)kvtophys((vaddr_t)cp);
	nphys = -1;

	/* Compute 2nd DMA pointer only if next page is part of this I/O */
	if ((NBPG - (phys & (NBPG - 1))) < len) {
		cp = (caddr_t)mips_trunc_page(cp + NBPG);
		nphys = (u_int)kvtophys((vaddr_t)cp);
	}

	/* If not R4K, need to invalidate cache lines for both physical segments */
	if (!CPUISMIPS3 && flag == ASCDMA_READ) {
		MachFlushDCache(MIPS_PHYS_TO_KSEG0(phys),
		    nphys == 0xffffffff ?  len : NBPG - (phys & (NBPG - 1)));
		if (nphys != 0xffffffff)
			MachFlushDCache(MIPS_PHYS_TO_KSEG0(nphys),
			    NBPG);	/* XXX */
	}

#ifdef notyet
	asc->dma_next = cp;
	asc->dma_xfer = state->dmalen - (nphys - phys);
#endif

	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh,
				IOASIC_SCSI_DMAPTR, IOASIC_DMA_ADDR(phys));
	bus_space_write_4(asc->sc_bst, asc->sc_bsh,
				IOASIC_SCSI_NEXTPTR, IOASIC_DMA_ADDR(nphys));
	if (flag == ASCDMA_READ)
		ssr |= (IOASIC_CSR_SCSI_DIR | IOASIC_CSR_DMAEN_SCSI);
	else
		ssr = (ssr & ~IOASIC_CSR_SCSI_DIR) | IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	return (len);
}

static void
asic_dma_end(asc, state, flag)
	asc_softc_t asc;
	State *state;
	int flag;
{
	u_int32_t ssr, ptr, halfwords;

	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	ptr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_DMAPTR);
#if USE_CACHED_BUFFER	/* XXX - Should uncached address always be used? */
	ptr = MIPS_PHYS_TO_KSEG0(ptr >> 3);
#else
	ptr = MIPS_PHYS_TO_KSEG1(ptr >> 3);
#endif
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_DMAPTR, -1);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_NEXTPTR, -1);

	if (flag == ASCDMA_READ) {
#if !defined(ASC_IOASIC_BOUNCE) && USE_CACHED_BUFFER
		/* Invalidate cache for the buffer */
#ifdef MIPS3
		if (CPUISMIPS3)
			MachFlushDCache(MIPS_KSEG1_TO_PHYS(state->dmaBufAddr),
			   state->dmalen);
		else
#endif /* MIPS3 */
			MachFlushDCache(MIPS_PHYS_TO_KSEG0(
			    MIPS_KSEG1_TO_PHYS(state->dmaBufAddr)),
			    state->dmalen);
#endif	/* USE_CACHED_BUFFER */
		halfwords = bus_space_read_4(asc->sc_bst, asc->sc_bsh,
					IOASIC_SCSI_SCR);
		if (halfwords != 0) {
			int sdr[2];
			/* pick up last upto 6 bytes, sigh. */

			/* Copy untransferred data from IOASIC */
			sdr[0] = bus_space_read_4(asc->sc_bst, asc->sc_bsh,
					IOASIC_SCSI_SDR0);
			sdr[1] = bus_space_read_4(asc->sc_bst, asc->sc_bsh,
					IOASIC_SCSI_SDR1);
			memcpy((caddr_t)ptr, (caddr_t)sdr, halfwords * 2);
		}
	}
}

#ifdef notdef
/*
 * XXX Below is just informational for how IOASIC DMA is handled. XXX
 */

extern struct cfdriver asc_cd;

/*
 * Called by asic_intr() for scsi dma pointer update interrupts.
 */
void
asc_dma_intr()
{
	asc_softc_t asc =  &asc_cd.cd_devs[0]; /*XXX*/
	u_int next_phys;

	asc->dma_xfer -= NBPG;
	if (asc->dma_xfer <= -NBPG) {
		volatile u_int *ssr = (volatile u_int *)
			IOASIC_REG_CSR(ioasic_base);
		*ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	} else {
		asc->dma_next += NBPG;
		next_phys = MIPS_KSEG0_TO_PHYS(asc->dma_next);
	}
	*(volatile int *)IOASIC_REG_SCSI_DMANPTR(ioasic_base) =
		IOASIC_DMA_ADDR(next_phys);
	wbflush();
}
#endif /*notdef*/
