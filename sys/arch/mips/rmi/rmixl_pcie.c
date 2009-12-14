/*	$NetBSD: rmixl_pcie.c,v 1.2 2009/12/14 00:46:07 matt Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI configuration support for RMI XLS SoC
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_pcie.c,v 1.2 2009/12/14 00:46:07 matt Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_pcievar.h>

#include <mips/rmi/rmixl_obiovar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#ifdef	PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

#include <machine/pci_machdep.h>

#ifdef PCI_DEBUG
int rmixl_pcie_debug = PCI_DEBUG;
# define DPRINTF(x)	do { if (rmixl_pcie_debug) printf x ; } while (0)
#else
# define DPRINTF(x)
#endif

#ifndef DDB
# define STATIC static
#else
# define STATIC
#endif


/*
 * XLS PCIe Extended Configuration Registers
 */
#define RMIXL_PCIE_ECFG_UESR	0x104	/* Uncorrectable Error Status Reg */
#define RMIXL_PCIE_ECFG_UEMR	0x108	/* Uncorrectable Error Mask Reg */
#define RMIXL_PCIE_ECFG_UEVR	0x10c	/* Uncorrectable Error seVerity Reg */
#define  PCIE_ECFG_UEVR_DFLT	\
		(__BITS(18,17) | __BIT(31) | __BITS(5,4) | __BIT(0))
#define  PCIE_ECFG_UExR_RESV	(__BITS(31,21) | __BITS(11,6) | __BITS(3,1))
#define RMIXL_PCIE_ECFG_CESR	0x110	/* Correctable Error Status Reg */
#define RMIXL_PCIE_ECFG_CEMR	0x114	/* Correctable Error Mask Reg */
#define  PCIE_ECFG_CExR_RESV	(__BITS(31,14) | __BITS(11,9) | __BITS(5,1))
#define RMIXL_PCIE_ECFG_ACCR	0x118	/* Adv. Capabilities Control Reg */
#define RMIXL_PCIE_ECFG_HLRn(n)	(0x11c + ((n) * 4))	/* Header Log Regs */
#define RMIXL_PCIE_ECFG_RECR	0x12c	/* Root Error Command Reg */
#define  PCIE_ECFG_RECR_RESV	__BITS(31,3)
#define RMIXL_PCIE_ECFG_RESR	0x130	/* Root Error Status Reg */
#define  PCIE_ECFG_RESR_RESV	__BITS(26,7)
#define RMIXL_PCIE_ECFG_ESI	0x134	/* Error Source Identification Reg */
#define RMIXL_PCIE_ECFG_DSNCR	0x140	/* Dev Serial Number Capability Regs */

static const struct {
	u_int offset;
	u_int32_t rw1c;
} pcie_ecfg_errs_tab[] = {
	{ RMIXL_PCIE_ECFG_UESR,		(__BITS(20,12) | __BIT(4)) },
	{ RMIXL_PCIE_ECFG_CESR,		(__BITS(20,12) | __BIT(4)) },
	{ RMIXL_PCIE_ECFG_HLRn(0),	0 },
	{ RMIXL_PCIE_ECFG_HLRn(1),	0 },
	{ RMIXL_PCIE_ECFG_HLRn(2),	0 },
	{ RMIXL_PCIE_ECFG_HLRn(3),	0 },
	{ RMIXL_PCIE_ECFG_RESR,		__BITS(6,0) },
	{ RMIXL_PCIE_ECFG_ESI,		0 },
};
#define PCIE_ECFG_ERRS_OFFTAB_NENTRIES \
	(sizeof(pcie_ecfg_errs_tab)/sizeof(pcie_ecfg_errs_tab[0]))

static int	rmixl_pcie_match(device_t, cfdata_t, void *);
static void	rmixl_pcie_attach(device_t, device_t, void *);
static void	rmixl_pcie_init(struct rmixl_pcie_softc *);
static void	rmixl_pcie_init_ecfg(struct rmixl_pcie_softc *);
static void	rmixl_pcie_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
static void	rmixl_pcie_lnkcfg_4xx(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg_408Lite(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg_2xx(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg_1xx(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg(struct rmixl_pcie_softc *);
static void	rmixl_pcie_errata(struct rmixl_pcie_softc *);
static void	rmixl_conf_interrupt(void *, int, int, int, int, int *);
static int	rmixl_pcie_bus_maxdevs(void *, int);
static pcitag_t	rmixl_tag_to_ecfg(pcitag_t);
static pcitag_t	rmixl_pcie_make_tag(void *, int, int, int);
static void	rmixl_pcie_decompose_tag(void *, pcitag_t, int *, int *, int *);
void		rmixl_pcie_tag_print(const char *restrict, void *, pcitag_t,				int, vaddr_t, u_long);
static int	rmixl_pcie_conf_setup(struct rmixl_pcie_softc *,
			pcitag_t, int *, bus_space_tag_t *,
			bus_space_handle_t *);
static pcireg_t	rmixl_pcie_conf_read(void *, pcitag_t, int);
static void	rmixl_pcie_conf_write(void *, pcitag_t, int, pcireg_t);

static int	rmixl_pcie_intr_map(struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *
		rmixl_pcie_intr_string(void *, pci_intr_handle_t);
static const struct evcnt *
		rmixl_pcie_intr_evcnt(void *, pci_intr_handle_t);
static void	*rmixl_pcie_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	rmixl_pcie_intr_disestablish(void *, void *);
#if defined(DEBUG) || defined(DDB)
int		rmixl_pcie_error_check(void);
#endif
static int	_rmixl_pcie_error_check(void *);
static int	rmixl_pcie_error_intr(void *);

/*
 * XXX use locks
 */
#define	PCI_CONF_LOCK(s)	(s) = splhigh()
#define	PCI_CONF_UNLOCK(s)	splx((s))


#define RMIXL_PCIE_CONCAT3(a,b,c) a ## b ## c
#define RMIXL_PCIE_BAR_INIT(reg, bar, size, align) {			\
	struct extent *ext = rmixl_configuration.rc_phys_ex;		\
	u_long region_start;						\
	uint64_t ba;							\
	int err;							\
									\
	err = extent_alloc(ext, (size), (align), 0UL, EX_NOWAIT,	\
		&region_start);						\
	if (err != 0)							\
		panic("%s: extent_alloc(%p, %#lx, %#lx, %#lx, %#x, %p)",\
			__func__, ext, size, align, 0UL, EX_NOWAIT,	\
			&region_start);					\
	ba = (uint64_t)region_start;					\
	ba *= (1024 * 1024);						\
	bar = RMIXL_PCIE_CONCAT3(RMIXL_PCIE_,reg,_BAR)(ba, 1);		\
	DPRINTF(("PCIE %s BAR was not enabled by firmware\n"		\
		"enabling %s at phys %#" PRIxBUSADDR ", size %lu MB\n",	\
		__STRING(reg), __STRING(reg), ba, size));		\
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_BRIDGE + 			\
		RMIXL_PCIE_CONCAT3(RMIXL_SBC_PCIE_,reg,_BAR), bar);	\
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE +			\
		RMIXL_PCIE_CONCAT3(RMIXL_SBC_PCIE_,reg,_BAR));		\
	DPRINTF(("%s: %s BAR %#x\n", __func__, __STRING(reg), bar));	\
}


#if defined(DEBUG) || defined(DDB)
static void *rmixl_pcie_v;
#endif

CFATTACH_DECL_NEW(rmixl_pcie, sizeof(struct rmixl_pcie_softc),
    rmixl_pcie_match, rmixl_pcie_attach, NULL, NULL); 

static int rmixl_pcie_found;

/*
 * rmixl_cache_err_dis:
 * - disable Cache, Data ECC, Snoop Tag Parity, Tag Parity errors
 * - clear the cache error log
 * - return previous value from RMIXL_PCR_L1D_CONFIG0
 */
static inline uint64_t
rmixl_cache_err_dis(void)
{
	uint64_t r;

	r = rmixl_mfcr(RMIXL_PCR_L1D_CONFIG0);
	rmixl_mtcr(RMIXL_PCR_L1D_CONFIG0, r & ~0x2e);
	rmixl_mtcr(RMIXL_PCR_L1D_CACHE_ERROR_LOG, 0);
	return r;
}

/*
 * rmixl_cache_err_restore:
 * - clear the cache error log, cache error overflow log,
 *   and cache interrupt registers
 * - restore previous value to RMIXL_PCR_L1D_CONFIG0
 */
static inline void
rmixl_cache_err_restore(uint64_t r)
{
	rmixl_mtcr(RMIXL_PCR_L1D_CACHE_ERROR_LOG, 0);
	rmixl_mtcr(RMIXL_PCR_L1D_CACHE_ERROR_OVF_LO, 0);
	rmixl_mtcr(RMIXL_PCR_L1D_CACHE_INTERRUPT, 0);
	rmixl_mtcr(RMIXL_PCR_L1D_CONFIG0, r);
}

static inline uint64_t
rmixl_cache_err_check(void)
{
	return rmixl_mfcr(RMIXL_PCR_L1D_CACHE_ERROR_LOG);
}

static int  
rmixl_pcie_match(device_t parent, cfdata_t cf, void *aux)
{        
	uint32_t r;

	/* XXX
	 * for now there is only one PCIe Interface on chip
	 * this could change with furture RMI XL family designs
	 */
	if (rmixl_pcie_found)
		return 0;

	/* read GPIO Reset Configuration register */
	/* XXX FIXME define the offset */
	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET_CFG);
	r >>= 26;
	r &= 3;
	if (r != 0)
		return 0;	/* strapped for SRIO */

	return 1;
}    

static void 
rmixl_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_pcie_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct rmixl_config *rcp = &rmixl_configuration;
        struct pcibus_attach_args pba;
	uint32_t bar;

	rmixl_pcie_found = 1;
	sc->sc_dev = self;

	aprint_normal(" RMI XLS PCIe Interface\n");

	rmixl_pcie_lnkcfg(sc);

	rmixl_pcie_errata(sc);

	sc->sc_29bit_dmat = obio->obio_29bit_dmat;
	sc->sc_32bit_dmat = obio->obio_32bit_dmat;
	sc->sc_64bit_dmat = obio->obio_64bit_dmat;

	/*
	 * get PCI config space base addr from SBC PCIe CFG BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXL_SBC_PCIE_CFG_BAR);
	DPRINTF(("%s: PCIE_CFG_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIE_CFG_BAR_ENB) == 0) {
		u_long n = RMIXL_PCIE_CFG_SIZE / (1024 * 1024);
		RMIXL_PCIE_BAR_INIT(CFG, bar, n, n);
	}
	rcp->rc_pcie_cfg_pbase = (bus_addr_t)RMIXL_PCIE_CFG_BAR_TO_BA(bar);
	rcp->rc_pcie_cfg_size  = (bus_size_t)RMIXL_PCIE_CFG_SIZE;

	/*
	 * get PCIE Extended config space base addr from SBC PCIe ECFG BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXL_SBC_PCIE_ECFG_BAR);
	DPRINTF(("%s: PCIE_ECFG_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIE_ECFG_BAR_ENB) == 0) {
		u_long n = RMIXL_PCIE_ECFG_SIZE / (1024 * 1024);
		RMIXL_PCIE_BAR_INIT(ECFG, bar, n, n);
	}
	rcp->rc_pcie_ecfg_pbase = (bus_addr_t)RMIXL_PCIE_ECFG_BAR_TO_BA(bar);
	rcp->rc_pcie_ecfg_size  = (bus_size_t)RMIXL_PCIE_ECFG_SIZE;

	/*
	 * get PCI MEM space base [addr, size] from SBC PCIe MEM BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXL_SBC_PCIE_MEM_BAR);
	DPRINTF(("%s: PCIE_MEM_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIE_MEM_BAR_ENB) == 0) {
		u_long n = 256;				/* 256 MB */
		RMIXL_PCIE_BAR_INIT(MEM, bar, n, n);
	}
	rcp->rc_pci_mem_pbase = (bus_addr_t)RMIXL_PCIE_MEM_BAR_TO_BA(bar);
	rcp->rc_pci_mem_size  = (bus_size_t)RMIXL_PCIE_MEM_BAR_TO_SIZE(bar);

	/*
	 * get PCI IO space base [addr, size] from SBC PCIe IO BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXL_SBC_PCIE_IO_BAR);
	DPRINTF(("%s: PCIE_IO_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIE_IO_BAR_ENB) == 0) {
		u_long n = 32;				/* 32 MB */
		RMIXL_PCIE_BAR_INIT(IO, bar, n, n);
	}
	rcp->rc_pci_io_pbase = (bus_addr_t)RMIXL_PCIE_IO_BAR_TO_BA(bar);
	rcp->rc_pci_io_size  = (bus_size_t)RMIXL_PCIE_IO_BAR_TO_SIZE(bar);

	/*
	 * initialize the PCI CFG, ECFG bus space tags
	 */
	rmixl_pcie_cfg_bus_mem_init(&rcp->rc_pcie_cfg_memt, rcp);
	sc->sc_pcie_cfg_memt = &rcp->rc_pcie_cfg_memt;

	rmixl_pcie_ecfg_bus_mem_init(&rcp->rc_pcie_ecfg_memt, rcp);
	sc->sc_pcie_ecfg_memt = &rcp->rc_pcie_ecfg_memt;

	/*
	 * initialize the PCI MEM and IO bus space tags
	 */
	rmixl_pcie_bus_mem_init(&rcp->rc_pci_memt, rcp);
	rmixl_pcie_bus_io_init(&rcp->rc_pci_iot, rcp);

	/*
	 * initialize the extended configuration regs
	 */
	rmixl_pcie_init_ecfg(sc);

	/*
	 * initialize the PCI chipset tag
	 */
	rmixl_pcie_init(sc);

	/*
	 * attach the PCI bus
	 */
	memset(&pba, 0, sizeof(pba));
	pba.pba_memt = &rcp->rc_pci_memt;
	pba.pba_iot =  &rcp->rc_pci_iot;
	pba.pba_dmat = sc->sc_29bit_dmat;	/* XXX */
#ifdef NOTYET
	pba.pba_dmat64 = NULL;
#endif
	pba.pba_pc = &sc->sc_pci_chipset;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
		PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;

	(void) config_found_ia(self, "pcibus", &pba, pcibusprint);
}

/*
 * rmixl_pcie_lnkcfg_4xx - link configs for XLS4xx and XLS6xx
 *	use IO_AD[11] and IO_AD[10], observable in 
 *	Bits[21:20] of the GPIO Reset Configuration register
 */
static void
rmixl_pcie_lnkcfg_4xx(rmixl_pcie_lnktab_t *ltp, uint32_t grcr)
{
	u_int index;
	static const rmixl_pcie_lnkcfg_t lnktab_4xx[4][4] = {
		{{ LCFG_EP, 4}, {LCFG_NO, 0}, {LCFG_NO, 0}, {LCFG_NO, 0}},
		{{ LCFG_RC, 4}, {LCFG_NO, 0}, {LCFG_NO, 0}, {LCFG_NO, 0}},
		{{ LCFG_EP, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}},
		{{ LCFG_RC, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}},
	};
	static const char *lnkstr_4xx[4] = {
		"EP 1x4",
		"RC 1x4",
		"EP 1x1, RC 4x1",
		"RC 4x1"
	};
	index = (grcr >> 20) & 3;
	ltp->ncfgs = 4;
	ltp->cfg = lnktab_4xx[index];
	ltp->str = lnkstr_4xx[index];
}

/*
 * rmixl_pcie_lnkcfg_408Lite - link configs for XLS408Lite and XLS04A
 *	use IO_AD[11] and IO_AD[10], observable in 
 *	Bits[21:20] of the GPIO Reset Configuration register
 */
static void
rmixl_pcie_lnkcfg_408Lite(rmixl_pcie_lnktab_t *ltp, uint32_t grcr)
{
	u_int index;
	static const rmixl_pcie_lnkcfg_t lnktab_408Lite[4][2] = {
		{{ LCFG_EP, 4}, {LCFG_NO, 0}},
		{{ LCFG_RC, 4}, {LCFG_NO, 0}},
		{{ LCFG_EP, 1}, {LCFG_RC, 1}},
		{{ LCFG_RC, 1}, {LCFG_RC, 1}},
	};
	static const char *lnkstr_408Lite[4] = {
		"EP 1x4",
		"RC 1x4",
		"EP 1x1, RC 1x1",
		"RC 2x1"
	};

	index = (grcr >> 20) & 3;
	ltp->ncfgs = 2;
	ltp->cfg = lnktab_408Lite[index];
	ltp->str = lnkstr_408Lite[index];
}

/*
 * rmixl_pcie_lnkcfg_2xx - link configs for XLS2xx
 *	use IO_AD[10], observable in Bit[20] of the
 *	GPIO Reset Configuration register
 */
static void
rmixl_pcie_lnkcfg_2xx(rmixl_pcie_lnktab_t *ltp, uint32_t grcr)
{
	u_int index;
	static const rmixl_pcie_lnkcfg_t lnktab_2xx[2][4] = {
		{{ LCFG_EP, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}},
		{{ LCFG_RC, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}, {LCFG_RC, 1}}
	};
	static const char *lnkstr_2xx[2] = {
		"EP 1x1, RC 3x1",
		"RC 4x1",
	};

	index = (grcr >> 20) & 1;
	ltp->ncfgs = 4;
	ltp->cfg = lnktab_2xx[index];
	ltp->str = lnkstr_2xx[index];
}

/*
 * rmixl_pcie_lnkcfg_1xx - link configs for XLS1xx
 *	use IO_AD[10], observable in Bit[20] of the
 *	GPIO Reset Configuration register
 */
static void
rmixl_pcie_lnkcfg_1xx(rmixl_pcie_lnktab_t *ltp, uint32_t grcr)
{
	u_int index;
	static const rmixl_pcie_lnkcfg_t lnktab_1xx[2][2] = {
		{{ LCFG_EP, 1}, {LCFG_RC, 1}},
		{{ LCFG_RC, 1}, {LCFG_RC, 1}}
	};
	static const char *lnkstr_1xx[2] = {
		"EP 1x1, RC 1x1",
		"RC 2x1",
	};

	index = (grcr >> 20) & 1;
	ltp->ncfgs = 2;
	ltp->cfg = lnktab_1xx[index];
	ltp->str = lnkstr_1xx[index];
}

/*
 * rmixl_pcie_lnkcfg - determine PCI Express Link Configuration
 */
static void
rmixl_pcie_lnkcfg(struct rmixl_pcie_softc *sc)
{
	uint32_t r;

	/* read GPIO Reset Configuration register */
	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET_CFG);
	DPRINTF(("%s: GPIO RCR %#x\n", __func__, r));

	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
		rmixl_pcie_lnkcfg_1xx(&sc->sc_pcie_lnktab, r);
		break;
	case MIPS_XLS204:
	case MIPS_XLS208:
		rmixl_pcie_lnkcfg_2xx(&sc->sc_pcie_lnktab, r);
		break;
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		rmixl_pcie_lnkcfg_408Lite(&sc->sc_pcie_lnktab, r);
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608: 
	case MIPS_XLS616:
		/* 6xx uses same table as 4xx */
		rmixl_pcie_lnkcfg_4xx(&sc->sc_pcie_lnktab, r);
		break;
	default:
		panic("%s: unknown RMI PRID IMPL", __func__);
	}

	aprint_normal("%s: link config %s\n",
		device_xname(sc->sc_dev), sc->sc_pcie_lnktab.str);
}

static void
rmixl_pcie_errata(struct rmixl_pcie_softc *sc)
{
	u_int rev;
	u_int lanes;
	bool e391 = false;

	/*
	 * 3.9.1 PCIe Link-0 Registers Reset to Incorrect Values
	 * check if it allies to this CPU implementation and revision
	 */
	rev = MIPS_PRID_REV(cpu_id);
	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
		break;
	case MIPS_XLS204:
	case MIPS_XLS208:
		/* stepping A0 is affected */
		if (rev == 0)
			e391 = true;
		break;
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
		/* steppings A0 and A1 are affected */
		if ((rev == 0) || (rev == 1))
			e391 = true;
		break;
	case MIPS_XLS608: 
	case MIPS_XLS616:
		break;
	default:
		panic("unknown RMI PRID IMPL");
        }

	/*
	 * for XLS we only need to check entry #0
	 * this may need to change for later XL family chips
	 */
	lanes = sc->sc_pcie_lnktab.cfg[0].lanes;

	if ((e391 != false) && ((lanes == 2) || (lanes == 4))) {
		/*
		 * attempt work around for errata 3.9.1
		 * "PCIe Link-0 Registers Reset to Incorrect Values"
		 * the registers are write-once: if the firmware already wrote,
		 * then our writes are ignored;  hope they did it right.
		 */
		uint32_t queuectrl;
		uint32_t bufdepth;
#ifdef DIAGNOSTIC
		uint32_t r;
#endif

		aprint_normal("%s: attempt work around for errata 3.9.1",
			device_xname(sc->sc_dev));
		if (lanes == 4) {
			queuectrl = 0x00018074;
			bufdepth  = 0x001901D1;
		} else {
			queuectrl = 0x00018036;
			bufdepth  = 0x001900D9;
		}

		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_BE +
			RMIXL_VC0_POSTED_RX_QUEUE_CTRL, queuectrl);
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_BE +
			RMIXL_VC0_POSTED_BUFFER_DEPTH, bufdepth);

#ifdef DIAGNOSTIC
		r = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_BE +
			RMIXL_VC0_POSTED_RX_QUEUE_CTRL);
		printf("\nVC0_POSTED_RX_QUEUE_CTRL %#x\n", r);

		r = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_BE +
			RMIXL_VC0_POSTED_BUFFER_DEPTH);
		printf("VC0_POSTED_BUFFER_DEPTH %#x\n", r);
#endif
	}
}

static void
rmixl_pcie_init(struct rmixl_pcie_softc *sc)
{
	pci_chipset_tag_t pc = &sc->sc_pci_chipset;
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
#endif

	pc->pc_conf_v = (void *)sc;
	pc->pc_attach_hook = rmixl_pcie_attach_hook;
	pc->pc_bus_maxdevs = rmixl_pcie_bus_maxdevs;
	pc->pc_make_tag = rmixl_pcie_make_tag;
	pc->pc_decompose_tag = rmixl_pcie_decompose_tag;
	pc->pc_conf_read = rmixl_pcie_conf_read;
	pc->pc_conf_write = rmixl_pcie_conf_write;

	pc->pc_intr_v = (void *)sc;
	pc->pc_intr_map = rmixl_pcie_intr_map;
	pc->pc_intr_string = rmixl_pcie_intr_string;
	pc->pc_intr_evcnt = rmixl_pcie_intr_evcnt;
	pc->pc_intr_establish = rmixl_pcie_intr_establish;
	pc->pc_intr_disestablish = rmixl_pcie_intr_disestablish;
	pc->pc_conf_interrupt = rmixl_conf_interrupt;

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	/*
	 * Configure the PCI bus.
	 */
	struct rmixl_config *rcp = &rmixl_configuration;

	aprint_normal("%s: configuring PCI bus\n",
		device_xname(sc->sc_dev));

	ioext  = extent_create("pciio",
		rcp->rc_pci_io_pbase,
		rcp->rc_pci_io_pbase + rcp->rc_pci_io_size - 1,
		M_DEVBUF, NULL, 0, EX_NOWAIT);

	memext = extent_create("pcimem",
		rcp->rc_pci_mem_pbase,
		rcp->rc_pci_mem_pbase + rcp->rc_pci_mem_size - 1,
		M_DEVBUF, NULL, 0, EX_NOWAIT);

	pci_configure_bus(pc, ioext, memext, NULL, 0, mips_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

static void
rmixl_pcie_init_ecfg(struct rmixl_pcie_softc *sc)
{
	void *v;
	pcitag_t tag;
	pcireg_t r;

	v = sc;
	tag = rmixl_pcie_make_tag(v, 0, 0, 0);

#ifdef PCI_DEBUG
	int i, offset;
	static const int offtab[] =
		{ 0, 4, 8, 0xc, 0x10, 0x14, 0x18, 0x1c,
		  0x2c, 0x30, 0x34 };
	for (i=0; i < sizeof(offtab)/sizeof(offtab[0]); i++) {
		offset = 0x100 + offtab[i];
		r = rmixl_pcie_conf_read(v, tag, offset);
		printf("%s: %#x: %#x\n", __func__, offset, r);
	}
#endif
	r = rmixl_pcie_conf_read(v, tag, 0x100);
	if (r == -1)
		return;	/* cannot access */

	/* check pre-existing uncorrectable errs */
	r = rmixl_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_UESR);
	r &= ~PCIE_ECFG_UExR_RESV;
	if (r != 0)
		panic("%s: Uncorrectable Error Status: %#x\n",
			__func__, r);

	/* unmask all uncorrectable errs */
	r = rmixl_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_UEMR);
	r &= ~PCIE_ECFG_UExR_RESV;
	rmixl_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_UEMR, r);

	/* ensure default uncorrectable err severity confniguration */
	r = rmixl_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_UEVR);
	r &= ~PCIE_ECFG_UExR_RESV;
	r |= PCIE_ECFG_UEVR_DFLT;
	rmixl_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_UEVR, r);

	/* check pre-existing correctable errs */
	r = rmixl_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_CESR);
	r &= ~PCIE_ECFG_CExR_RESV;
#ifdef DIAGNOSTIC
	if (r != 0)
		aprint_normal("%s: Correctable Error Status: %#x\n",
			device_xname(sc->sc_dev), r);
#endif

	/* unmask all correctable errs */
	r = rmixl_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_CEMR);
	r &= ~PCIE_ECFG_CExR_RESV;
	rmixl_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_UEMR, r);

	/* check pre-existing Root Error Status */
	r = rmixl_pcie_conf_read(v, tag, RMIXL_PCIE_ECFG_RESR);
	r &= ~PCIE_ECFG_RESR_RESV;
	if (r != 0)
		panic("%s: Root Error Status: %#x\n", __func__, r);
			/* XXX TMP FIXME */

	/* enable all Root errs */
	r = (pcireg_t)(~PCIE_ECFG_RECR_RESV);
	rmixl_pcie_conf_write(v, tag, RMIXL_PCIE_ECFG_RECR, r);


	if (MIPS_PRID_IMPL(cpu_id) == MIPS_XLS408LITE) {
		/*
		 * establish ISR for PCIE Fatal Error interrupt
		 * XXX for XLS408Lite, XLS2xx, XLS1xx only
		 *     tested on XLS408Lite only
		 */
		(void)rmixl_intr_establish(29, IPL_HIGH,
			RMIXL_INTR_LEVEL, RMIXL_INTR_HIGH,
			rmixl_pcie_error_intr, v);
	}
#if defined(DEBUG) || defined(DDB)
	rmixl_pcie_v = v;
#endif
}

void
rmixl_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *iline)
{
	DPRINTF(("%s: %p, %d, %d, %d, %d, %p\n",
		__func__, v, bus, dev, ipin, swiz, iline));
}

void
rmixl_pcie_attach_hook(struct device *parent, struct device *self,
	struct pcibus_attach_args *pba)
{
	DPRINTF(("%s: pba_bus %d, pba_bridgetag %p, pc_conf_v %p\n",
		__func__, pba->pba_bus, pba->pba_bridgetag,
		pba->pba_pc->pc_conf_v));
}

int
rmixl_pcie_bus_maxdevs(void *v, int busno)
{
	return (32);	/* XXX depends on the family of XLS SoC */
}

/*
 * rmixl_tag_to_ecfg - convert cfg address (generic tag) to ecfg address
 *
 *	39:29   (reserved)
 *	28      Swap (0=little, 1=big endian)
 *	27:20   Bus number
 *	19:15   Device number
 *	14:12   Function number
 *	11:8    Extended Register number
 *	7:0     Register number
 */
static pcitag_t
rmixl_tag_to_ecfg(pcitag_t tag)
{
	KASSERT((tag & __BITS(7,0)) == 0);
	return (tag << 4);
}

/*
 * XLS pci tag is a 40 bit address composed thusly:
 *	39:25   (reserved)
 *	24      Swap (0=little, 1=big endian)
 *	23:16   Bus number
 *	15:11   Device number
 *	10:8    Function number
 *	7:0     Register number
 *
 * Note: this is the "native" composition for addressing CFG space, but not for ECFG space.
 */
pcitag_t
rmixl_pcie_make_tag(void *v, int bus, int dev, int fun)
{
	return ((bus << 16) | (dev << 11) | (fun << 8));
}

void
rmixl_pcie_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

void
rmixl_pcie_tag_print(const char *restrict s, void *v, pcitag_t tag, int offset,
	vaddr_t va, u_long r)
{
	int bus, dev, fun;

	rmixl_pcie_decompose_tag(v, tag, &bus, &dev, &fun);
	printf("%s: %d/%d/%d/%d - %#" PRIxVADDR ":%#lx\n",
		s, bus, dev, fun, offset, va, r);
}

static int
rmixl_pcie_conf_setup(struct rmixl_pcie_softc *sc,
	pcitag_t tag, int *offp, bus_space_tag_t *bstp,
	bus_space_handle_t *bshp)
{
	struct rmixl_config *rcp = &rmixl_configuration;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t size;
	pcitag_t mask;
	bus_addr_t ba;
	int err;
	static bus_space_handle_t cfg_bsh;
	static bus_addr_t cfg_oba = -1;
	static bus_space_handle_t ecfg_bsh;
	static bus_addr_t ecfg_oba = -1;

	/*
	 * bus space depends on offset
	 */
	if ((*offp >= 0) && (*offp < 0x100)) {
		mask = __BITS(15,0);
		bst = sc->sc_pcie_cfg_memt;
		ba = rcp->rc_pcie_cfg_pbase;
		ba += (tag & ~mask);
		*offp += (tag & mask);
		if (ba != cfg_oba) {
			size = (bus_size_t)(mask + 1);
			if (cfg_oba != -1)
				bus_space_unmap(bst, cfg_bsh, size);
			err = bus_space_map(bst, ba, size, 0, &cfg_bsh);
			if (err != 0) {
#ifdef DEBUG
				panic("%s: bus_space_map err %d, CFG space",
					__func__, err);	/* XXX */
#endif
				return -1;
			}
			cfg_oba = ba;
		}
		bsh = cfg_bsh;
	} else if ((*offp >= 0x100) && (*offp <= 0x700)) {
		mask = __BITS(14,0);
		tag = rmixl_tag_to_ecfg(tag);	/* convert to ECFG format */
		bst = sc->sc_pcie_ecfg_memt;
		ba = rcp->rc_pcie_ecfg_pbase;
		ba += (tag & ~mask);
		*offp += (tag & mask);
		if (ba != ecfg_oba) {
			size = (bus_size_t)(mask + 1);
			if (ecfg_oba != -1)
				bus_space_unmap(bst, ecfg_bsh, size);
			err = bus_space_map(bst, ba, size, 0, &ecfg_bsh);
			if (err != 0) {
#ifdef DEBUH
				panic("%s: bus_space_map err %d, ECFG space",
					__func__, err);	/* XXX */
#endif
				return -1;
			}
			ecfg_oba = ba;
		}
		bsh = ecfg_bsh;
	} else  {
#ifdef DEBUG
		panic("%s: offset %#x: unknown", __func__, *offp);
#endif
		return -1;
	}

	*bstp = bst;
	*bshp = bsh;

	return 0;
}

pcireg_t
rmixl_pcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct rmixl_pcie_softc *sc = v;
	static bus_space_handle_t bsh;
	bus_space_tag_t bst;
	pcireg_t rv;
	uint64_t cfg0;
	u_int s;

	PCI_CONF_LOCK(s);

	if (rmixl_pcie_conf_setup(sc, tag, &offset, &bst, &bsh) == 0) {
		cfg0 = rmixl_cache_err_dis();
		rv = bus_space_read_4(bst, bsh, (bus_size_t)offset);
		if (rmixl_cache_err_check() != 0) {
#ifdef DIAGNOSTIC
			int bus, dev, fun;

			rmixl_pcie_decompose_tag(v, tag, &bus, &dev, &fun);
			printf("%s: %d/%d/%d, offset %#x: bad address\n",
				__func__, bus, dev, fun, offset);
#endif
			rv = (pcireg_t) -1;
		}
		rmixl_cache_err_restore(cfg0);
	} else {
		rv = -1;
	}

	PCI_CONF_UNLOCK(s);
	return rv;
}

void
rmixl_pcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct rmixl_pcie_softc *sc = v;
	static bus_space_handle_t bsh;
	bus_space_tag_t bst;
	uint64_t cfg0;
	u_int s;

	PCI_CONF_LOCK(s);

	if (rmixl_pcie_conf_setup(sc, tag, &offset, &bst, &bsh) == 0) {
		cfg0 = rmixl_cache_err_dis();
		bus_space_write_4(bst, bsh, (bus_size_t)offset, val);
		if (rmixl_cache_err_check() != 0) {
#ifdef DIAGNOSTIC
			int bus, dev, fun;

			rmixl_pcie_decompose_tag(v, tag, &bus, &dev, &fun);
			printf("%s: %d/%d/%d, offset %#x: bad address\n",
				__func__, bus, dev, fun, offset);
#endif
		}
		rmixl_cache_err_restore(cfg0);
	}

	PCI_CONF_UNLOCK(s);
}

int
rmixl_pcie_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *pih)
{
	u_int irq;

#ifdef DEBUG
	DPRINTF(("%s: ps_bus %d, pa_intrswiz %#x, pa_intrtag %#lx,"
		" pa_intrpin %d,  pa_intrline %d, pa_rawintrpin %d\n",
		__func__, pa->pa_bus, pa->pa_intrswiz, pa->pa_intrtag,
		pa->pa_intrpin, pa->pa_intrline, pa->pa_rawintrpin));
#endif

	/*
	 * XXX cpu implementation specific
	 */
	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_XLS408LITE:
		switch (pa->pa_bus) {
		case 1:
			irq = 26;
			break;
		case 2:
			irq = 27;
			break;
		default:
			panic("%s: bad bus %d\n", __func__, pa->pa_bus);
		}
		break;
	case MIPS_XLS416:
	case MIPS_XLS616:
		switch (pa->pa_bus) {
		case 1:
			irq = 26;
			break;
		case 2:
			irq = 27;
			break;
		case 3:
			irq = 28;
			break;
		case 4:
			irq = 29;
			break;
		default:
			panic("%s: bad bus %d\n", __func__, pa->pa_bus);
		}
		break;
	default:
		panic("%s: cpu IMPL %#x not supported\n",
			__func__, MIPS_PRID_IMPL(cpu_id));
	}

	*pih = irq;

	return 0;
}

const char *
rmixl_pcie_intr_string(void *v, pci_intr_handle_t pih)
{
	const char *name = "(illegal)";
	int irq = (int)pih;

	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_XLS408LITE:
		switch (irq) {
		case 26:
		case 27:
			name = rmixl_intr_string(irq);
			break;
		}
		break;
	case MIPS_XLS616:
		switch (irq) {
		case 26:
		case 27:
		case 28:
		case 29:
			name = rmixl_intr_string(irq);
			break;
		}
		break;
	}

	return name;
}

const struct evcnt *
rmixl_pcie_intr_evcnt(void *v, pci_intr_handle_t pih)
{
	return NULL;
}

static int
rmixl_pcie_irq(pci_intr_handle_t pih)
{
	return (int)pih;
}

static void *
rmixl_pcie_intr_establish(void *v, pci_intr_handle_t pih, int ipl,
	int (*func)(void *), void *arg)
{
	return rmixl_intr_establish(rmixl_pcie_irq((int)pih), ipl,
		RMIXL_INTR_LEVEL, RMIXL_INTR_HIGH, func, arg);
}

static void
rmixl_pcie_intr_disestablish(void *v, void *ih)
{
	rmixl_intr_disestablish(ih);
}

#if defined(DEBUG) || defined(DDB)
/* this function exists to facilitate call from ddb */
int
rmixl_pcie_error_check(void)
{
	if (rmixl_pcie_v != 0)
		return _rmixl_pcie_error_check(rmixl_pcie_v);
	return -1;
}
#endif

STATIC int
_rmixl_pcie_error_check(void *v)
{
	int i, offset;
	pcireg_t r;
	pcitag_t tag;
	int err=0;
#ifdef DIAGNOSTIC
	pcireg_t regs[PCIE_ECFG_ERRS_OFFTAB_NENTRIES];
#endif

	tag = rmixl_pcie_make_tag(v, 0, 0, 0);	/* XXX */

	for (i=0; i < PCIE_ECFG_ERRS_OFFTAB_NENTRIES; i++) {
		offset = pcie_ecfg_errs_tab[i].offset;
		r = rmixl_pcie_conf_read(v, tag, offset);
#ifdef DIAGNOSTIC
		regs[i] = r;
#endif
		if (r != 0) {
			pcireg_t rw1c = r & pcie_ecfg_errs_tab[i].rw1c;
			if (rw1c != 0) {
				/* attempt to clear the error */
				rmixl_pcie_conf_write(v, tag, offset, rw1c);
			};
			if (offset == RMIXL_PCIE_ECFG_CESR)
				err |= 1;	/* correctable */
			else
				err |= 2;	/* uncorrectable */
		}
	}
#ifdef DIAGNOSTIC
	if (err != 0) {
		for (i=0; i < PCIE_ECFG_ERRS_OFFTAB_NENTRIES; i++) {
			offset = pcie_ecfg_errs_tab[i].offset;
			printf("%s: %#x: %#x\n", __func__, offset, regs[i]);
		}
	}
#endif

	return err;
}

static int
rmixl_pcie_error_intr(void *v)
{
	if (_rmixl_pcie_error_check(v) < 2)
		return 0;	/* correctable */

	/* uncorrectable */
#if DDB
	Debugger();
#endif

	/* XXX reset and recover? */

	panic("%s\n", __func__);
}
