/* $NetBSD: asc_ioasic.c,v 1.1.2.9 1999/04/06 01:54:28 nisimura Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: asc_ioasic.c,v 1.1.2.9 1999/04/06 01:54:28 nisimura Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/bus.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <dev/tc/ioasicreg.h>

struct asc_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* glue to MI code */
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_scsi_bsh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamap;
	void	*sc_cookie;			/* intr. handling cookie */

	int	sc_active;			/* DMA active ? */
	int	sc_ispullup;			/* DMA into main memory? */
	size_t	sc_dmasize;
	caddr_t *sc_dmaaddr;
	size_t	*sc_dmalen;

	/* XXX XXX XXX */
	volatile u_int32_t *sc_scsi_dmaptr;
	volatile u_int32_t *sc_scsi_nextptr;
};

int	asc_ioasic_match __P((struct device *, struct cfdata *, void *));
void	asc_ioasic_attach __P((struct device *, struct device *, void *));

struct cfattach asc_ioasic_ca = {
	sizeof(struct asc_softc), asc_ioasic_match, asc_ioasic_attach
};

struct scsipi_device asc_ioasic_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

void	asc_ioasic_reset __P((struct ncr53c9x_softc *));
int	asc_ioasic_intr __P((struct ncr53c9x_softc *));
int	asc_ioasic_setup __P((struct ncr53c9x_softc *,
				caddr_t *, size_t *, int, size_t *));
void	asc_ioasic_go __P((struct ncr53c9x_softc *));
void	asc_ioasic_stop __P((struct ncr53c9x_softc *));

static u_char	asc_read_reg __P((struct ncr53c9x_softc *, int));
static void	asc_write_reg __P((struct ncr53c9x_softc *, int, u_char));
static int	asc_dma_isintr __P((struct ncr53c9x_softc *sc));
static int	asc_dma_isactive __P((struct ncr53c9x_softc *));
static void	asc_clear_latched_intr __P((struct ncr53c9x_softc *));

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
/* XXX */
	extern int slot_in_progress;	/* TC slot being probed */
	slot_in_progress = 3;		/* IOASIC always resides in TC slot 3 */
/* XXX */

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &asc_ioasic_glue;
	asc->sc_bst = ((struct ioasic_softc *)parent)->sc_bst;
	asc->sc_bsh = ((struct ioasic_softc *)parent)->sc_bsh;
	asc->sc_dmat = ((struct ioasic_softc *)parent)->sc_dmat;
	if (bus_space_subregion(asc->sc_bst,
			asc->sc_bsh,
			IOASIC_SLOT_12_START, 0x100, &asc->sc_scsi_bsh)) {
		printf("%s: unable to map device\n", sc->sc_dev.dv_xname);
		return;
	}
	asc->sc_cookie = d->iada_cookie;

	/* XXX XXX XXX */
	asc->sc_scsi_dmaptr =	(void *)(ioasic_base + IOASIC_SCSI_DMAPTR);
	asc->sc_scsi_nextptr =	(void *)(ioasic_base + IOASIC_SCSI_NEXTPTR);

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
	sc->sc_adapter.scsipi_cmd = ncr53c9x_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = minphys;
	ncr53c9x_attach(sc, &asc_ioasic_dev);
}

void
asc_ioasic_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	u_int32_t ssr;

	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_SCR, 0);
	asc->sc_active = 0;
}

int
asc_ioasic_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	int trans, resid;
	u_int tcl, tcm, ssr;
	
	if (asc->sc_active == 0)
		panic("dmaintr: DMA wasn't active");

	/* DMA has stopped */
	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_SCR, 0);
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
	if (!asc->sc_ispullup &&
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
	asc->sc_ispullup = datain;

	NCR_DMA(("ioasic_setup: start %d@%p %s\n",
		*asc->sc_dmalen, *asc->sc_dmaaddr, datain ? "IN" : "OUT"));

	/* IOASIC DMA can handle upto 64KB transfer at a time */
	/* XXX but, at most 2 pages due to current implementation XXX */
	size = min(*dmasize, 2 * NBPG - ((size_t)*addr & (NBPG - 1)));
	*dmasize = asc->sc_dmasize = size;

	NCR_DMA(("ioasic_setup: dmasize = %d\n", asc->sc_dmasize));

	/* stop DMA engine first */
	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_SCSI_SCR, 0);

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
	if (asc->sc_ispullup)
		ssr |=  IOASIC_CSR_SCSI_DIR;
	else
		ssr &= ~IOASIC_CSR_SCSI_DIR;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
	return 0;
}

void
asc_ioasic_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	u_int32_t ssr;

	ssr = bus_space_read_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR);
	ssr |= IOASIC_CSR_DMAEN_SCSI;
	bus_space_write_4(asc->sc_bst, asc->sc_bsh, IOASIC_CSR, ssr);
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
	tc_wmb();

	if (asc->sc_ispullup != 0 && (nb = *asc->sc_scsi_scr) != 0) {
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

/*
 * Glue functions.
 */
static u_char
asc_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	u_char v;

	v = bus_space_read_4(asc->sc_bst, asc->sc_scsi_bsh,
	    reg * sizeof(u_int32_t)) & 0xff;

	return (v);
}

static void
asc_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	bus_space_write_4(asc->sc_bst, asc->sc_scsi_bsh,
	    reg * sizeof(u_int32_t), val);
}

static int
asc_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	return !!(NCR_READ_REG(sc, NCR_STAT) & NCRSTAT_INT);
}

static int
asc_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return (asc->sc_active);
}

static void
asc_clear_latched_intr(sc)
	struct ncr53c9x_softc *sc;
{
}
