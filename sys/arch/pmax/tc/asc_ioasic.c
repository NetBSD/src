/* $NetBSD: asc_ioasic.c,v 1.1.2.3 1999/01/18 20:18:26 drochner Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: asc_ioasic.c,v 1.1.2.3 1999/01/18 20:18:26 drochner Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/locore.h>	/* XXX pmax XXX */

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <pmax/tc/ascvar.h>	/* XXX pmax XXX */
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <dev/tc/ioasicreg.h>

int	asc_ioasic_match __P((struct device *, struct cfdata *, void *));
void	asc_ioasic_attach __P((struct device *, struct device *, void *));

struct cfattach asc_ioasic_ca = {
	sizeof(struct asc_softc), asc_ioasic_match, asc_ioasic_attach
};

void	asc_ioasic_reset __P((struct ncr53c9x_softc *));
int	asc_ioasic_intr __P((struct ncr53c9x_softc *));
int	asc_ioasic_setup __P((struct ncr53c9x_softc *,
				caddr_t *, size_t *, int, size_t *));
void	asc_ioasic_go __P((struct ncr53c9x_softc *));
void	asc_ioasic_stop __P((struct ncr53c9x_softc *));

struct ncr53c9x_glue asc_ioasic_glue = {
	asc_read_reg,
	asc_write_reg,
	asc_dma_isintr,
	asc_ioasic_reset,
	asc_ioasic_intr,
	asc_ioasic_setup,
	asc_ioasic_go,
	asc_ioasic_stop,
	asc_dma_isactive,
	asc_clear_latched_intr,
};

int
asc_ioasic_match(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	
	if (strncmp("asc", d->iada_modname, TC_ROM_LLEN))
		return (0);
	if (tc_badaddr(d->iada_addr))
		return (0);

	return (1);
}

void
asc_ioasic_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	struct asc_softc *asc = (struct asc_softc *)self;	
	struct ncr53c9x_softc *sc = &asc->sc_ncr53c9x;
#ifdef __pmax__
	extern int slot_in_progress;	/* XXX TC slot being probed */

	slot_in_progress = 3;	/* IOASIC always resides in TC slot 3 */
#endif

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &asc_ioasic_glue;
#if 0
	asc->sc_bst = /* bus space tag */ 0;
	asc->sc_bsh = /* bus space handle */ 0;
	asc->sc_dma = /* bus dma */ 0;
#endif
	asc->sc_base = (void *)MIPS_PHYS_TO_KSEG1(d->iada_addr);/* useless */
	asc->sc_base = (void *)ioasic_base; /* XXX */ /* TC slot 3 */	
	asc->sc_cookie = d->iada_cookie;

	asc->sc_reg = (char *)asc->sc_base + IOASIC_SLOT_12_START;
	asc->sc_ssr = (void *)((char *)asc->sc_base + IOASIC_CSR);
	asc->sc_scsi_dmaptr = (void *)((char *)asc->sc_base
				       + IOASIC_SCSI_DMAPTR);
	asc->sc_scsi_nextptr = (void *)((char *)asc->sc_base
					+ IOASIC_SCSI_NEXTPTR);
	asc->sc_scsi_scr = (void *)((char *)asc->sc_base + IOASIC_SCSI_SCR);
	asc->sc_scsi_sdr0 = (void *)((char *)asc->sc_base + IOASIC_SCSI_SDR0);
	asc->sc_scsi_sdr1 = (void *)((char *)asc->sc_base + IOASIC_SCSI_SDR1);

	sc->sc_id = 7;
	sc->sc_freq = 25000000;

	/* gimme Mhz */
	sc->sc_freq /= 1000000;

	ioasic_intr_establish(parent, asc->sc_cookie, TC_IPL_BIO,
		(int (*)(void *))ncr53c9x_intr, sc);

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
	sc->sc_cfg3 = 0;
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
	sc->sc_minsync = (1000 / sc->sc_freq) * 5 / 4 ;
	sc->sc_maxxfer = 64 * 1024;

	/* Do the common parts of attachment. */
	sc->sc_adapter.scsipi_cmd = asc_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = minphys;
	ncr53c9x_attach(sc, &asc_dev);
}

void
asc_ioasic_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	*asc->sc_ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	*asc->sc_scsi_scr = 0;

	asc->sc_active = 0;
}

int
asc_ioasic_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	int trans, resid;
	u_int tcl, tcm;
	
	if (asc->sc_active == 0)
		panic("dmaintr: DMA wasn't active");

	/* DMA has stopped */
	*asc->sc_ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	*asc->sc_scsi_scr = 0;
	asc->sc_active = 0;	

	if (asc->sc_dmasize == 0) {
		/* A "Transfer Pad" operation completed */
		tcl = NCR_READ_REG(sc, NCR_TCL); 
		tcm = NCR_READ_REG(sc, NCR_TCM);
		NCR_DMA(("ioasic_intr: discarded %d bytes (tcl=%d, tcm=%d)\n",
		    tcl | (tcm << 8), tcl, tcm));
		return 0;
	}

	resid = 0;
	if (!asc->sc_iswrite &&
	    (resid = (NCR_READ_REG(sc, NCR_FFLAG) & NCRFIFO_FF)) != 0) {
		NCR_DMA(("ioasic_intr: empty FIFO of %d ", resid));
		DELAY(1);
	}

	resid += (tcl = NCR_READ_REG(sc, NCR_TCL));
	resid += (tcm = NCR_READ_REG(sc, NCR_TCM)) << 8;

	trans = asc->sc_dmasize - resid;
	if (trans < 0) {			/* transferred < 0 ? */
		printf("ioasic_intr: xfer (%d) > req (%d)\n",
		    trans, asc->sc_dmasize);
		trans = asc->sc_dmasize;
	}

	NCR_DMA(("ioasic_intr: tcl=%d, tcm=%d; trans=%d, resid=%d\n",
	    tcl, tcm, trans, resid));

	*asc->sc_dmalen -= trans;
	*asc->sc_dmaaddr += trans;
	
	return 0;
}

/* XXX mips XXX */
int
asc_ioasic_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	unsigned phys, nphys;
	u_int32_t ssr;
	size_t size;
	extern paddr_t kvtophys __P((vaddr_t));
#define	MIPS_KVA_TO_PHYS(x) (unsigned)kvtophys((vaddr_t)(x))

	asc->sc_dmaaddr = addr;
	asc->sc_dmalen = len;
	asc->sc_iswrite = datain;

	NCR_DMA(("ioasic_setup: start %d@%p %s\n",
		*asc->sc_dmalen, *asc->sc_dmaaddr, datain ? "IN" : "OUT"));

	/* IOASIC DMA can handle upto 64KB transfer at a time */
	/* XXX but, at most 2 pages due to current implementation XXX */
	size = min(*dmasize, 2 * NBPG - ((size_t)*addr & (NBPG - 1)));
	*dmasize = asc->sc_dmasize = size;

	NCR_DMA(("ioasic_setup: dmasize = %d\n", asc->sc_dmasize));

	/* stop DMA engine first */
	*asc->sc_ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	*asc->sc_scsi_scr = 0;

	/* If R4K, writeback and invalidate the buffer */
	if (CPUISMIPS3)
		mips3_HitFlushDCache((vaddr_t)*addr, size);

	/* XXX only the next page can be handled with single operation XXX */
	phys = MIPS_KVA_TO_PHYS(*addr);
	if ((unsigned)*addr + size <= mips_trunc_page(*addr) + NBPG)
		nphys = ~0;
	else
		nphys = MIPS_KVA_TO_PHYS(mips_trunc_page(*addr) + NBPG);

	/* If not R4K, need to invalidate cache lines for physical segments */
	if (!CPUISMIPS3 && datain) {
		if (nphys == ~0)
			MachFlushDCache(MIPS_PHYS_TO_KSEG0(phys), size);
		else {
			int size0 = NBPG - (phys & (NBPG - 1));
			int size1 = size - size0;
			MachFlushDCache(MIPS_PHYS_TO_KSEG0(phys), size0);
			MachFlushDCache(MIPS_PHYS_TO_KSEG0(nphys), size1);
		}
	}

	*asc->sc_scsi_dmaptr = IOASIC_DMA_ADDR(phys);
	*asc->sc_scsi_nextptr = IOASIC_DMA_ADDR(nphys);
	ssr = *asc->sc_ssr;
	if (asc->sc_iswrite)
		ssr = (ssr | IOASIC_CSR_SCSI_DIR) | IOASIC_CSR_DMAEN_SCSI;
	else
		ssr = (ssr &~ IOASIC_CSR_SCSI_DIR) | IOASIC_CSR_DMAEN_SCSI;
	*asc->sc_ssr = ssr;
	wbflush();
	asc->sc_active = 1;
	return 0;
}

void
asc_ioasic_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
#if 0
	u_int32_t ssr = *asc->sc_ssr;

	if (asc->sc_iswrite)
		ssr = (ssr | IOASIC_CSR_SCSI_DIR) | IOASIC_CSR_DMAEN_SCSI;
	else
		ssr = (ssr &~ IOASIC_CSR_SCSI_DIR) | IOASIC_CSR_DMAEN_SCSI;
	*asc->sc_ssr = ssr;
	wbflush();
#endif
	asc->sc_active = 1;
}

/* NEVER CALLED BY MI 53C9x ENGINE INDEED */
void
asc_ioasic_stop(sc)
	struct ncr53c9x_softc *sc;
{
#if 0
	struct asc_softc *asc = (struct asc_softc *)sc;
	u_short *to;
	int w;
	int nb;

	*asc->sc_ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	to = (u_short *)MIPS_PHYS_TO_KSEG1(*asc->sc_scsi_dmaptr >> 3);
	*asc->sc_scsi_dmaptr = ~0;
	*asc->sc_scsi_nextptr = ~0;
	wbflush();

	if (asc->sc_iswrite != 0 && (nb = *asc->sc_scsi_scr) != 0) {
		/* pick up last upto 6 bytes, sigh. */

		/* Last byte really xferred is.. */
		w = *asc->sc_scsi_sdr0;
		*to++ = w;
		if (--nb > 0) {
			w >>= 16;
			*to++ = w;
		}
		if (--nb > 0) {
			w = *asc->sc_scsi_sdr1;
			*to++ = w;
		}
	}
	asc->sc_active = 0;
#endif
}
