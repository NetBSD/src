/* $NetBSD: dwlpx.c,v 1.8 1997/06/08 07:59:20 thorpej Exp $ */

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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dwlpx.c,v 1.8 1997/06/08 07:59:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/autoconf.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/kftxxvar.h>
#include <alpha/tlsb/kftxxreg.h>
#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>
#include <alpha/pci/pci_kn8ae.h>

#include <alpha/include/pmap.old.h>

#define	KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))

static int	dwlpxmatch __P((struct device *, struct cfdata *, void *));
static void	dwlpxattach __P((struct device *, struct device *, void *));
struct cfattach dwlpx_ca = {
	sizeof(struct dwlpx_softc), dwlpxmatch, dwlpxattach
};

struct cfdriver	dwlpx_cd = {
	NULL, "dwlpx", DV_DULL,
};

static int	dwlpxprint __P((void *, const char *));

static int
dwlpxprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pcibus_attach_args *pba = aux;
	/* only PCIs can attach to DWLPX's; easy. */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

static int
dwlpxmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct kft_dev_attach_args *ka = aux;

	if (strcmp(ka->ka_name, dwlpx_cd.cd_name) != 0)
		return (0);
	return (1);
}

static void
dwlpxattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	static int once = 0;
	struct dwlpx_softc *sc = (struct dwlpx_softc *)self;
	struct dwlpx_config *ccp = &sc->dwlpx_cc;
	struct kft_dev_attach_args *ka = aux;
	struct pcibus_attach_args pba;
	u_int32_t pcia_present;

	sc->dwlpx_node = ka->ka_node;
	sc->dwlpx_dtype = ka->ka_dtype;
	sc->dwlpx_hosenum = ka->ka_hosenum;
	/*
	 * On reads, you get a fault if you read a nonexisted HPC.
	 * The internal KFTIA hose (hose 0) has only 2 HPCs.
	 */
	sc->dwlpx_nhpc = NHPC;
	if (sc->dwlpx_hosenum == 0) {
		if (TLDEV_DTYPE(sc->dwlpx_dtype) == TLDEV_DTYPE_KFTIA) {
			sc->dwlpx_nhpc = NHPC - 1;
		}
	}

	dwlpx_init(sc);

	/* XXX Need to detect DWLPA vs. DWLPB here. */
	pcia_present = REGVAL(PCIA_PRESENT + ccp->cc_sysbase);

	printf(": PCIA rev. %d, STD I/O %spresent\n",
	    (pcia_present >> PCIA_PRESENT_REVSHIFT) & PCIA_PRESENT_REVMASK,
	    (pcia_present & PCIA_PRESENT_STDIO) == 0 ? "not " : "");

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
				printf("%s: hpc %d slot %d: %s watt module\n",
				    sc->dwlpx_dev.dv_xname, hpc, slot, str);
			}
		}
	}
#endif

	if (once == 0) {
		/*
		 * Set up interrupts
		 */
		pci_kn8ae_pickintr(&sc->dwlpx_cc, 1);
#ifdef	EVCNT_COUNTERS
		evcnt_attach(self, "intr", kn8ae_intr_evcnt);
#endif
		once++;
	} else {
		pci_kn8ae_pickintr(&sc->dwlpx_cc, 0);
	}

	/*
	 * Attach PCI bus
	 */
	pba.pba_busname = "pci";
	pba.pba_iot = sc->dwlpx_cc.cc_iot;
	pba.pba_memt = sc->dwlpx_cc.cc_memt;
	pba.pba_dmat =	/* start with direct, may change... */
	    alphabus_dma_get_tag(&sc->dwlpx_cc.cc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_pc = &sc->dwlpx_cc.cc_pc;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	config_found(self, &pba, dwlpxprint);
}

void
dwlpx_init(sc)
	struct dwlpx_softc *sc;
{
	int i;
	struct dwlpx_config *ccp = &sc->dwlpx_cc;

	if (ccp->cc_initted == 0) {
		ccp->cc_iot = dwlpx_bus_io_init(ccp);
		ccp->cc_memt = dwlpx_bus_mem_init(ccp);
	}
	dwlpx_pci_init(&ccp->cc_pc, ccp);
	ccp->cc_sc = sc;

	/*
	 * Establish a precalculated base for convenience's sake.
	 */
	ccp->cc_sysbase =
	    (((unsigned long)(sc->dwlpx_node - 4))	<< 36) |
	    (((unsigned long) sc->dwlpx_hosenum)	<< 34) |
	    (1LL					<< 39);

	/*
	 * Set up DMA stuff for this DWLPX.
	 */
	dwlpx_dma_init(ccp);

	/*
	 * Set up interrupt stuff for this DWLPX.
	 *
	 * Note that all PCI interrupt pins are disabled at this time.
	 *
	 * Do this even for all HPCs- even for the nonexistent
	 * one on hose zero of a KFTIA.
	 */
	for (i = 0; i < NHPC; i++) {
		REGVAL(PCIA_IMASK(i) + ccp->cc_sysbase) = DWLPX_IMASK_DFLT;
		REGVAL(PCIA_ERRVEC(i) + ccp->cc_sysbase) =
		    DWLPX_ERRVEC((sc->dwlpx_node - 4), sc->dwlpx_hosenum);
	}
	for (i = 0; i < DWLPX_MAXDEV; i++) {
		u_int16_t vec;
		int ss, hpc;

		vec = DWLPX_MVEC((sc->dwlpx_node - 4), sc->dwlpx_hosenum, i);
		ss = i;
		if (i < 4) {
			hpc = 0;
		} else if (i < 8) {
			ss -= 4;
			hpc = 1;
		} else {
			ss -= 8;
			hpc = 2;
		}
		REGVAL(PCIA_DEVVEC(hpc, ss, 1) + ccp->cc_sysbase) = vec;
		REGVAL(PCIA_DEVVEC(hpc, ss, 2) + ccp->cc_sysbase) = vec;
		REGVAL(PCIA_DEVVEC(hpc, ss, 3) + ccp->cc_sysbase) = vec;
		REGVAL(PCIA_DEVVEC(hpc, ss, 4) + ccp->cc_sysbase) = vec;
	}
	/*
	 * Establish HAE values, as well as make sure of sanity elsewhere.
	 */
	for (i = 0; i < sc->dwlpx_nhpc; i++) {
		u_int32_t ctl = REGVAL(PCIA_CTL(i) + ccp->cc_sysbase);
		ctl &= 0x0fffffff;
		ctl &= ~((0x1f << 14) | (0x1f << 9) | 0x3);
#if	0
		ctl |=  ((1 << 14) | (1 << 9));
#endif
		REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) = ctl;
	}
	ccp->cc_initted = 1;
}
