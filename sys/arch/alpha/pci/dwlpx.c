/* $NetBSD: dwlpx.c,v 1.37.2.1 2012/04/17 00:05:56 yamt Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dwlpx.c,v 1.37.2.1 2012/04/17 00:05:56 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/kftxxvar.h>
#include <alpha/tlsb/kftxxreg.h>
#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>
#include <alpha/pci/pci_kn8ae.h>

#define	KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))
#define	DWLPX_SYSBASE(sc)	\
	    ((((unsigned long)((sc)->dwlpx_node - 4))	<< 36) |	\
	     (((unsigned long) (sc)->dwlpx_hosenum)	<< 34) |	\
	     (1LL					<< 39))
#define	DWLPX_SYSBASE1(node, hosenum)	\
	    ((((unsigned long)(node - 4))	<< 36) |	\
	     (((unsigned long) hosenum)	        << 34) |	\
	     (1LL					<< 39))


static int	dwlpxmatch(device_t, cfdata_t, void *);
static void	dwlpxattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(dwlpx, sizeof(struct dwlpx_softc),
    dwlpxmatch, dwlpxattach, NULL, NULL);

extern struct cfdriver dwlpx_cd;

void	dwlpx_errintr(void *, u_long vec);

static int
dwlpxmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct kft_dev_attach_args *ka = aux;
	unsigned long ls;
	uint32_t ctl;

	if (strcmp(ka->ka_name, dwlpx_cd.cd_name) != 0)
		return (0);

	ls = DWLPX_SYSBASE1(ka->ka_node, ka->ka_hosenum);
	
	/*
	 * Probe the first HPC to make sure this really is a dwlpx and
	 * nothing else.
	 */
	if (badaddr(KV(PCIA_CTL(1) + ls), sizeof (ctl)) != 0) {
		/*
		 * If we are here something went wrong. One reason
		 * could be that this is a dwlma and not a dwlpx.
		 *
		 * We can not clear potential illegal CSR errors here
		 * since it is unknown hardware.
		 */
		return (0);
	}

	return (1);
}

static void
dwlpxattach(device_t parent, device_t self, void *aux)
{
	static int once = 0;
	struct dwlpx_softc *sc = device_private(self);
	struct dwlpx_config *ccp = &sc->dwlpx_cc;
	struct kft_dev_attach_args *ka = aux;
	struct pcibus_attach_args pba;
	uint32_t pcia_present;

	sc->dwlpx_dev = self;
	sc->dwlpx_node = ka->ka_node;
	sc->dwlpx_dtype = ka->ka_dtype;
	sc->dwlpx_hosenum = ka->ka_hosenum;

	dwlpx_init(sc);
	dwlpx_dma_init(ccp);

	pcia_present = REGVAL(PCIA_PRESENT + ccp->cc_sysbase);
	aprint_normal(": PCIA rev. %d, STD I/O %spresent, %dK S/G entries\n",
	    (pcia_present >> PCIA_PRESENT_REVSHIFT) & PCIA_PRESENT_REVMASK,
	    (pcia_present & PCIA_PRESENT_STDIO) == 0 ? "not " : "",
	    sc->dwlpx_sgmapsz == DWLPX_SG128K ? 128 : 32);

#if 0
	{
		int hpc, slot, slotval;
		const char *str;
		for (hpc = 0; hpc < sc->dwlpx_nhpc; hpc++) {
			for (slot = 0; slot < 4; slot++) {
				slotval = (pcia_present >>
				    PCIA_PRESENT_SLOTSHIFT(hpc, slot)) &
				    PCIA_PRESENT_SLOT_MASK;
				if (slotval == PCIA_PRESENT_SLOT_NONE)
					continue;
				switch (slotval) {
				case PCIA_PRESENT_SLOT_25W:
					str = "25";
					break;
				case PCIA_PRESENT_SLOT_15W:
					str = "15";
					break;
				case PCIA_PRESENT_SLOW_7W:
				default:		/* XXX gcc */
					str = "7.5";
					break;
				}
				aprint_normal_dev(sc->dwlpx_dev,
				    "hpc %d slot %d: %s watt module\n",
				    hpc, slot, str);
			}
		}
	}
#endif

	if (once == 0) {
		/*
		 * Set up interrupts
		 */
		pci_kn8ae_pickintr(&sc->dwlpx_cc, 1);
		once++;
	} else {
		pci_kn8ae_pickintr(&sc->dwlpx_cc, 0);
	}

	/*
	 * Attach PCI bus
	 */
	pba.pba_iot = &sc->dwlpx_cc.cc_iot;
	pba.pba_memt = &sc->dwlpx_cc.cc_memt;
	pba.pba_dmat =	/* start with direct, may change... */
	    alphabus_dma_get_tag(&sc->dwlpx_cc.cc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &sc->dwlpx_cc.cc_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

void
dwlpx_init(struct dwlpx_softc *sc)
{
	uint32_t ctl;
	struct dwlpx_config *ccp = &sc->dwlpx_cc;
	unsigned long vec, ls = DWLPX_SYSBASE(sc);
	int i;

	if (ccp->cc_initted == 0) {
		/*
		 * On reads, you get a fault if you read a nonexisted HPC.
		 * We know the internal KFTIA hose (hose 0) has only 2 HPCs,
		 * but we can also actually probe for HPCs.
		 * Assume at least one.
		 */
		for (sc->dwlpx_nhpc = 1; sc->dwlpx_nhpc < NHPC;
		    sc->dwlpx_nhpc++) {
			if (badaddr(KV(PCIA_CTL(sc->dwlpx_nhpc) + ls),
			    sizeof (ctl)) != 0) {
				break;
			}
		}
		if (sc->dwlpx_nhpc != NHPC) {
			/* clear (potential) Illegal CSR Address Error */
			REGVAL(PCIA_ERR(0) + DWLPX_SYSBASE(sc)) =
				PCIA_ERR_ALLERR;
		}

		dwlpx_bus_io_init(&ccp->cc_iot, ccp);
		dwlpx_bus_mem_init(&ccp->cc_memt, ccp);
	}
	dwlpx_pci_init(&ccp->cc_pc, ccp);
	ccp->cc_sc = sc;

	/*
	 * Establish a precalculated base for convenience's sake.
	 */
	ccp->cc_sysbase = ls;

	/*
	 * If there are only 2 HPCs, then the 'present' register is not
	 * implemented, so there will only ever be 32K SG entries. Otherwise
	 * any revision greater than zero will have 128K entries.
	 */
	ctl = REGVAL(PCIA_PRESENT + ccp->cc_sysbase);
	if (sc->dwlpx_nhpc == 2) {
		sc->dwlpx_sgmapsz = DWLPX_SG32K;
#if 0
	/*
	 * As of 2/25/98- When I enable SG128K, and then have to flip
	 * TBIT below, I get bad SGRAM errors. We'll fix this later
	 * if this gets important.
	 */
	} else if ((ctl >> PCIA_PRESENT_REVSHIFT) & PCIA_PRESENT_REVMASK) {
		sc->dwlpx_sgmapsz = DWLPX_SG128K;
#endif
	} else {
		sc->dwlpx_sgmapsz = DWLPX_SG32K;
	}

	/*
	 * Set up interrupt stuff for this DWLPX.
	 *
	 * Note that all PCI interrupt pins are disabled at this time.
	 *
	 * Do this even for all HPCs- even for the nonexistent
	 * one on hose zero of a KFTIA.
	 */
	vec = scb_alloc(dwlpx_errintr, sc);
	if (vec == SCB_ALLOC_FAILED)
		panic("%s: unable to allocate error vector",
		    device_xname(sc->dwlpx_dev));
	aprint_normal_dev(sc->dwlpx_dev, "error interrupt at vector 0x%lx\n",
	    vec);
	for (i = 0; i < NHPC; i++) {
		REGVAL(PCIA_IMASK(i) + ccp->cc_sysbase) = DWLPX_IMASK_DFLT;
		REGVAL(PCIA_ERRVEC(i) + ccp->cc_sysbase) = vec;
	}

	/*
	 * Establish HAE values, as well as make sure of sanity elsewhere.
	 */
	for (i = 0; i < sc->dwlpx_nhpc; i++) {
		ctl = REGVAL(PCIA_CTL(i) + ccp->cc_sysbase);
		ctl &= 0x0fffffff;
		ctl &= ~(PCIA_CTL_MHAE(0x1f) | PCIA_CTL_IHAE(0x1f));
		/*
		 * I originally also had it or'ing in 3, which makes no sense.
		 */

		ctl |= PCIA_CTL_RMMENA | PCIA_CTL_RMMARB;

		/*
		 * Only valid if we're attached to a KFTIA or a KTHA.
		 */
		ctl |= PCIA_CTL_3UP;

		ctl |= PCIA_CTL_CUTENA;

		/*
		 * Fit in appropriate S/G Map Ram size.
		 */
		if (sc->dwlpx_sgmapsz == DWLPX_SG32K)
			ctl |= PCIA_CTL_SG32K;
		else if (sc->dwlpx_sgmapsz == DWLPX_SG128K)
			ctl |= PCIA_CTL_SG128K;
		else
			ctl |= PCIA_CTL_SG32K;

		REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) = ctl;
	}
	/*
	 * Enable TBIT if required
	 */
	if (sc->dwlpx_sgmapsz == DWLPX_SG128K)
		REGVAL(PCIA_TBIT + ccp->cc_sysbase) = 1;
	alpha_mb();
	ccp->cc_initted = 1;
}

void
dwlpx_errintr(void *arg, unsigned long vec)
{
	struct dwlpx_softc *sc = arg;
	struct dwlpx_config *ccp = &sc->dwlpx_cc;
	int i;
	struct {
		uint32_t err;
		uint32_t addr;
	} hpcs[NHPC];

	for (i = 0; i < sc->dwlpx_nhpc; i++) {
		hpcs[i].err = REGVAL(PCIA_ERR(i) + ccp->cc_sysbase);
		hpcs[i].addr = REGVAL(PCIA_FADR(i) + ccp->cc_sysbase);
	}
	aprint_error_dev(sc->dwlpx_dev, "node %d hose %d error interrupt\n",
	    sc->dwlpx_node, sc->dwlpx_hosenum);
	
	for (i = 0; i < sc->dwlpx_nhpc; i++) {
		if ((hpcs[i].err & PCIA_ERR_ERROR) == 0)
			continue;
		aprint_error("\tHPC %d: ERR=0x%08x; DMA %s Memory, "
			"Failing Address 0x%x\n",
			i, hpcs[i].err, hpcs[i].addr & 0x1? "write to" :
			"read from", hpcs[i].addr & ~3);
		if (hpcs[i].err & PCIA_ERR_SERR_L)
			aprint_error("\t       PCI device asserted SERR_L\n");
		if (hpcs[i].err & PCIA_ERR_ILAT)
			aprint_error("\t       Incremental Latency Exceeded\n");
		if (hpcs[i].err & PCIA_ERR_SGPRTY)
			aprint_error("\t       CPU access of SG RAM Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_ILLCSR)
			aprint_error("\t       Illegal CSR Address Error\n");
		if (hpcs[i].err & PCIA_ERR_PCINXM)
			aprint_error("\t       Nonexistent PCI Address Error\n");
		if (hpcs[i].err & PCIA_ERR_DSCERR)
			aprint_error("\t       PCI Target Disconnect Error\n");
		if (hpcs[i].err & PCIA_ERR_ABRT)
			aprint_error("\t       PCI Target Abort Error\n");
		if (hpcs[i].err & PCIA_ERR_WPRTY)
			aprint_error("\t       PCI Write Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_DPERR)
			aprint_error("\t       PCI Data Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_APERR)
			aprint_error("\t       PCI Address Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_DFLT)
			aprint_error("\t       SG Map RAM Invalid Entry Error\n");
		if (hpcs[i].err & PCIA_ERR_DPRTY)
			aprint_error("\t       DMA access of SG RAM Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_DRPERR)
			aprint_error("\t       DMA Read Return Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_MABRT)
			aprint_error("\t       PCI Master Abort Error\n");
		if (hpcs[i].err & PCIA_ERR_CPRTY)
			aprint_error("\t       CSR Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_COVR)
			aprint_error("\t       CSR Overrun Error\n");
		if (hpcs[i].err & PCIA_ERR_MBPERR)
			aprint_error("\t       Mailbox Parity Error\n");
		if (hpcs[i].err & PCIA_ERR_MBILI)
			aprint_error("\t       Mailbox Illegal Length Error\n");
		REGVAL(PCIA_ERR(i) + ccp->cc_sysbase) = hpcs[i].err;
	}
}
