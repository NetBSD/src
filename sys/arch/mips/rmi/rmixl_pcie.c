/*	$NetBSD: rmixl_pcie.c,v 1.9.6.2 2014/08/20 00:03:13 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_pcie.c,v 1.9.6.2 2014/08/20 00:03:13 tls Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/kernel.h>		/* for 'hz' */
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_pcievar.h>

#include <mips/rmi/rmixl_obiovar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#ifdef	PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

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

typedef struct rmixl_pcie_int_csr {
	uint r0;
	uint r1;
} rmixl_pcie_int_csr_t;

static const rmixl_pcie_int_csr_t int_enb_offset[4] = {
	{ RMIXL_PCIE_LINK0_INT_ENABLE0, RMIXL_PCIE_LINK0_INT_ENABLE1 },
	{ RMIXL_PCIE_LINK1_INT_ENABLE0, RMIXL_PCIE_LINK1_INT_ENABLE1 },
	{ RMIXL_PCIE_LINK2_INT_ENABLE0, RMIXL_PCIE_LINK2_INT_ENABLE1 },
	{ RMIXL_PCIE_LINK3_INT_ENABLE0, RMIXL_PCIE_LINK3_INT_ENABLE1 },
};

static const rmixl_pcie_int_csr_t int_sts_offset[4] = {
	{ RMIXL_PCIE_LINK0_INT_STATUS0, RMIXL_PCIE_LINK0_INT_STATUS1 },
	{ RMIXL_PCIE_LINK1_INT_STATUS0, RMIXL_PCIE_LINK1_INT_STATUS1 },
	{ RMIXL_PCIE_LINK2_INT_STATUS0, RMIXL_PCIE_LINK2_INT_STATUS1 },
	{ RMIXL_PCIE_LINK3_INT_STATUS0, RMIXL_PCIE_LINK3_INT_STATUS1 },
};

static const u_int msi_enb_offset[4] = {
	RMIXL_PCIE_LINK0_MSI_ENABLE,
	RMIXL_PCIE_LINK1_MSI_ENABLE,
	RMIXL_PCIE_LINK2_MSI_ENABLE,
	RMIXL_PCIE_LINK3_MSI_ENABLE
};

#define RMIXL_PCIE_LINK_STATUS0_ERRORS	__BITS(6,4)
#define RMIXL_PCIE_LINK_STATUS1_ERRORS	__BITS(10,0)
#define RMIXL_PCIE_LINK_STATUS_ERRORS					\
		((((uint64_t)RMIXL_PCIE_LINK_STATUS1_ERRORS) << 32) |	\
		   (uint64_t)RMIXL_PCIE_LINK_STATUS0_ERRORS)

#define RMIXL_PCIE_EVCNT(sc, link, bitno, cpu)	\
		&(sc)->sc_evcnts[link][(bitno) * (ncpu) + (cpu)]

static int	rmixl_pcie_match(device_t, cfdata_t, void *);
static void	rmixl_pcie_attach(device_t, device_t, void *);
static void	rmixl_pcie_init(struct rmixl_pcie_softc *);
static void	rmixl_pcie_init_ecfg(struct rmixl_pcie_softc *);
static void	rmixl_pcie_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
static void	rmixl_pcie_lnkcfg_4xx(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg_408Lite(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg_2xx(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg_1xx(rmixl_pcie_lnktab_t *, uint32_t);
static void	rmixl_pcie_lnkcfg(struct rmixl_pcie_softc *);
static void	rmixl_pcie_intcfg(struct rmixl_pcie_softc *);
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

static int	rmixl_pcie_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *
		rmixl_pcie_intr_string(void *, pci_intr_handle_t, char *,
		    size_t);
static const struct evcnt *
		rmixl_pcie_intr_evcnt(void *, pci_intr_handle_t);
static pci_intr_handle_t
		rmixl_pcie_make_pih(u_int, u_int, u_int);
static void	rmixl_pcie_decompose_pih(pci_intr_handle_t, u_int *, u_int *, u_int *);
static void	rmixl_pcie_intr_disestablish(void *, void *);
static void	*rmixl_pcie_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static rmixl_pcie_link_intr_t *
		rmixl_pcie_lip_add_1(rmixl_pcie_softc_t *, u_int, int, int);
static void	rmixl_pcie_lip_free_callout(rmixl_pcie_link_intr_t *);
static void	rmixl_pcie_lip_free(void *);
static int	rmixl_pcie_intr(void *);
static void	rmixl_pcie_link_error_intr(u_int, uint32_t, uint32_t);
#if defined(DEBUG) || defined(DDB)
int		rmixl_pcie_error_check(void);
#endif
static int	_rmixl_pcie_error_check(void *);
static int	rmixl_pcie_error_intr(void *);


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
		RMIXL_PCIE_CONCAT3(RMIXLS_SBC_PCIE_,reg,_BAR), bar);	\
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE +			\
		RMIXL_PCIE_CONCAT3(RMIXLS_SBC_PCIE_,reg,_BAR));		\
	DPRINTF(("%s: %s BAR %#x\n", __func__, __STRING(reg), bar));	\
}


#if defined(DEBUG) || defined(DDB)
static void *rmixl_pcie_v;
#endif

CFATTACH_DECL_NEW(rmixl_pcie, sizeof(struct rmixl_pcie_softc),
    rmixl_pcie_match, rmixl_pcie_attach, NULL, NULL); 

static int rmixl_pcie_found;

static int  
rmixl_pcie_match(device_t parent, cfdata_t cf, void *aux)
{        
	uint32_t r;

	/*
	 * PCIe interface exists on XLS chips only
	 */
	if (! cpu_rmixls(mips_options.mips_cpu))
		return 0;

	/* XXX
	 * for now there is only one PCIe Interface on chip
	 * this could change with furture RMI XL family designs
	 */
	if (rmixl_pcie_found)
		return 0;

	/* read GPIO Reset Configuration register */
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

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_HIGH);

	rmixl_pcie_lnkcfg(sc);

	rmixl_pcie_intcfg(sc);

	rmixl_pcie_errata(sc);

	sc->sc_29bit_dmat = obio->obio_29bit_dmat;
	sc->sc_32bit_dmat = obio->obio_32bit_dmat;
	sc->sc_64bit_dmat = obio->obio_64bit_dmat;

	sc->sc_tmsk = obio->obio_tmsk;

	/*
	 * get PCI config space base addr from SBC PCIe CFG BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLS_SBC_PCIE_CFG_BAR);
	DPRINTF(("%s: PCIE_CFG_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIE_CFG_BAR_ENB) == 0) {
		u_long n = RMIXL_PCIE_CFG_SIZE / (1024 * 1024);
		RMIXL_PCIE_BAR_INIT(CFG, bar, n, n);
	}
	rcp->rc_pci_cfg_pbase = (bus_addr_t)RMIXL_PCIE_CFG_BAR_TO_BA(bar);
	rcp->rc_pci_cfg_size  = (bus_size_t)RMIXL_PCIE_CFG_SIZE;

	/*
	 * get PCIE Extended config space base addr from SBC PCIe ECFG BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLS_SBC_PCIE_ECFG_BAR);
	DPRINTF(("%s: PCIE_ECFG_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIE_ECFG_BAR_ENB) == 0) {
		u_long n = RMIXL_PCIE_ECFG_SIZE / (1024 * 1024);
		RMIXL_PCIE_BAR_INIT(ECFG, bar, n, n);
	}
	rcp->rc_pci_ecfg_pbase = (bus_addr_t)RMIXL_PCIE_ECFG_BAR_TO_BA(bar);
	rcp->rc_pci_ecfg_size  = (bus_size_t)RMIXL_PCIE_ECFG_SIZE;

	/*
	 * get PCI MEM space base [addr, size] from SBC PCIe MEM BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLS_SBC_PCIE_MEM_BAR);
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
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLS_SBC_PCIE_IO_BAR);
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
	rmixl_pci_cfg_bus_mem_init(&rcp->rc_pci_cfg_memt, rcp);
	sc->sc_pci_cfg_memt = &rcp->rc_pci_cfg_memt;

	rmixl_pci_ecfg_bus_mem_init(&rcp->rc_pci_ecfg_memt, rcp);
	sc->sc_pci_ecfg_memt = &rcp->rc_pci_ecfg_memt;

	/*
	 * initialize the PCI MEM and IO bus space tags
	 */
	rmixl_pci_bus_mem_init(&rcp->rc_pci_memt, rcp);
	rmixl_pci_bus_io_init(&rcp->rc_pci_iot, rcp);

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
	pba.pba_dmat = sc->sc_32bit_dmat;
	pba.pba_dmat64 = sc->sc_64bit_dmat;
	pba.pba_pc = &sc->sc_pci_chipset;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
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
		"1EPx4",
		"1RCx4",
		"1EPx1, 3RCx1",
		"4RCx1"
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
		"4EPx4",
		"1RCx4",
		"1EPx1, 1RCx1",
		"2RCx1"
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
		"1EPx1, 3RCx1",
		"4RCx1",
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
		"1EPx1, 1RCx1",
		"2RCx1",
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

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
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

/*
 * rmixl_pcie_intcfg - init PCIe Link interrupt enables
 */
static void
rmixl_pcie_intcfg(struct rmixl_pcie_softc *sc)
{
	int link;
	size_t size;
	rmixl_pcie_evcnt_t *ev;

	DPRINTF(("%s: disable all link interrupts\n", __func__));
	for (link=0; link < sc->sc_pcie_lnktab.ncfgs; link++) {
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + int_enb_offset[link].r0,
			RMIXL_PCIE_LINK_STATUS0_ERRORS); 
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + int_enb_offset[link].r1,
			RMIXL_PCIE_LINK_STATUS1_ERRORS); 
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + msi_enb_offset[link], 0); 
		sc->sc_link_intr[link] = NULL;

		/*
		 * allocate per-cpu, per-pin interrupt event counters
		 */
		size = ncpu * PCI_INTERRUPT_PIN_MAX * sizeof(rmixl_pcie_evcnt_t);
		ev = malloc(size, M_DEVBUF, M_NOWAIT);
		if (ev == NULL)
			panic("%s: cannot malloc evcnts\n", __func__);
		sc->sc_evcnts[link] = ev;
		for (int pin=PCI_INTERRUPT_PIN_A; pin <= PCI_INTERRUPT_PIN_MAX; pin++) {
			for (int cpu=0; cpu < ncpu; cpu++) {
				ev = RMIXL_PCIE_EVCNT(sc, link, pin - 1, cpu);
				snprintf(ev->name, sizeof(ev->name),
					"cpu%d, link %d, pin %d", cpu, link, pin);
				evcnt_attach_dynamic(&ev->evcnt, EVCNT_TYPE_INTR,
					NULL, "rmixl_pcie", ev->name);
			}
		}
	}
}

static void
rmixl_pcie_errata(struct rmixl_pcie_softc *sc)
{
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;
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
		NULL, 0, EX_NOWAIT);

	memext = extent_create("pcimem",
		rcp->rc_pci_mem_pbase,
		rcp->rc_pci_mem_pbase + rcp->rc_pci_mem_size - 1,
		NULL, 0, EX_NOWAIT);

	pci_configure_bus(pc, ioext, memext, NULL, 0,
	    mips_cache_info.mci_dcache_align);

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

	/*
	 * establish ISR for PCIE Fatal Error interrupt
	 * - for XLS4xxLite, XLS2xx, XLS1xx only
	 */
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS204:
	case MIPS_XLS208:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		sc->sc_fatal_ih = rmixl_intr_establish(29, sc->sc_tmsk,
			IPL_HIGH, RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
			rmixl_pcie_error_intr, v, false);
		break;
	default:
		break;
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
rmixl_pcie_attach_hook(device_t parent, device_t self,
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
		bst = sc->sc_pci_cfg_memt;
		ba = rcp->rc_pci_cfg_pbase;
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
		bst = sc->sc_pci_ecfg_memt;
		ba = rcp->rc_pci_ecfg_pbase;
		ba += (tag & ~mask);
		*offp += (tag & mask);
		if (ba != ecfg_oba) {
			size = (bus_size_t)(mask + 1);
			if (ecfg_oba != -1)
				bus_space_unmap(bst, ecfg_bsh, size);
			err = bus_space_map(bst, ba, size, 0, &ecfg_bsh);
			if (err != 0) {
#ifdef DEBUG
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

	mutex_enter(&sc->sc_mutex);

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

	mutex_exit(&sc->sc_mutex);

	return rv;
}

void
rmixl_pcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct rmixl_pcie_softc *sc = v;
	static bus_space_handle_t bsh;
	bus_space_tag_t bst;
	uint64_t cfg0;

	mutex_enter(&sc->sc_mutex);

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

	mutex_exit(&sc->sc_mutex);
}

int
rmixl_pcie_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *pih)
{
	int device;
	u_int link;
	u_int irq;

	/*
	 * The bus is unimportant since it can change depending on the
	 * configuration.  We are tied to device # of PCIe bridge we are
	 * ultimately attached to.
	 */
	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag,
	    NULL, &device, NULL);

#ifdef DEBUG
	DPRINTF(("%s: ps_bus %d, pa_intrswiz %#x, pa_intrtag %#lx,"
		" pa_intrpin %d,  pa_intrline %d, pa_rawintrpin %d\n",
		__func__, pa->pa_bus, pa->pa_intrswiz, pa->pa_intrtag,
		pa->pa_intrpin, pa->pa_intrline, pa->pa_rawintrpin));
#endif

	/*
	 * PCIe Link INT irq assignment is cpu implementation specific
	 */
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		if (device > 1)
			panic("%s: bad bus %d", __func__, device);
		link = device;
		irq = device + 26;
		break;
	case MIPS_XLS204:
	case MIPS_XLS208: {
		if (device > 3)
			panic("%s: bad bus %d", __func__, device);
		link = device;
		irq = device + (device & 2 ? 21 : 26);
		break;
	}
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608: 
	case MIPS_XLS616:
		if (device > 3)
			panic("%s: bad bus %d", __func__, device);
		link = device;
		irq = device + 26;
		break;
	default:
		panic("%s: cpu IMPL %#x not supported\n",
		    __func__, MIPS_PRID_IMPL(mips_options.mips_cpu_id));
	}

	if (pa->pa_intrpin != PCI_INTERRUPT_PIN_NONE)
		*pih = rmixl_pcie_make_pih(link, pa->pa_intrpin - 1, irq);
	else
		*pih = ~0;

	return 0;
}

const char *
rmixl_pcie_intr_string(void *v, pci_intr_handle_t pih, char *buf, size_t len)
{
	const char *name = "(illegal)";
	u_int link, bitno, irq;

	rmixl_pcie_decompose_pih(pih, &link, &bitno, &irq);

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		switch (irq) {
		case 26:
		case 27:
			name = rmixl_intr_string(RMIXL_IRT_VECTOR(irq));
			break;
		}
		break;
	case MIPS_XLS204:
	case MIPS_XLS208:
		switch (irq) {
		case 23:
		case 24:
		case 26:
		case 27:
			name = rmixl_intr_string(RMIXL_IRT_VECTOR(irq));
			break;
		}
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608: 
	case MIPS_XLS616:
		switch (irq) {
		case 26:
		case 27:
		case 28:
		case 29:
			name = rmixl_intr_string(RMIXL_IRT_VECTOR(irq));
			break;
		}
		break;
	default:
		panic("%s: cpu IMPL %#x not supported\n",
			__func__, MIPS_PRID_IMPL(mips_options.mips_cpu_id));
	}

	strlcpy(buf, name, len);
	return buf;
}

const struct evcnt *
rmixl_pcie_intr_evcnt(void *v, pci_intr_handle_t pih)
{
	return NULL;
}

static pci_intr_handle_t
rmixl_pcie_make_pih(u_int link, u_int bitno, u_int irq)
{
	pci_intr_handle_t pih;

	KASSERT(link < RMIXL_PCIE_NLINKS_MAX);
	KASSERT(bitno < 64);
	KASSERT(irq < 32);

	pih  = (irq << 10);
	pih |= (bitno << 4);
	pih |= link;

	return pih;
}

static void
rmixl_pcie_decompose_pih(pci_intr_handle_t pih, u_int *link, u_int *bitno, u_int *irq)
{
	*link = (u_int)(pih & 0xf);
	*bitno = (u_int)((pih >> 4) & 0x3f);
	*irq  = (u_int)(pih >> 10);

	KASSERT(*link < RMIXL_PCIE_NLINKS_MAX);
	KASSERT(*bitno < 64);
	KASSERT(*irq < 32);
}

static void
rmixl_pcie_intr_disestablish(void *v, void *ih)
{
	rmixl_pcie_softc_t *sc = v;
	rmixl_pcie_link_dispatch_t *dip = ih;
	rmixl_pcie_link_intr_t *lip = sc->sc_link_intr[dip->link];
	uint32_t r;
	uint32_t bit;
	u_int offset;
	u_int other;
	bool busy;

	DPRINTF(("%s: link=%d pin=%d irq=%d\n",
		__func__, dip->link, dip->bitno + 1, dip->irq));

	mutex_enter(&sc->sc_mutex);

	dip->func = NULL;	/* mark unused, prevent further dispatch */

	/*
	 * if no other dispatch handle is using this interrupt,
	 * we can disable it
	 */
	busy = false;
	for (int i=0; i < lip->dispatch_count; i++) {
		rmixl_pcie_link_dispatch_t *d = &lip->dispatch_data[i];
		if (d == dip)
			continue;
		if (d->bitno == dip->bitno) {
			busy = true;
			break;
		}
	}
	if (! busy) {
		if (dip->bitno < 32) {
			bit = 1 << dip->bitno;
			offset = int_enb_offset[dip->link].r0;
			other  = int_enb_offset[dip->link].r1;
		} else {
			bit = 1 << (dip->bitno - 32);
			offset = int_enb_offset[dip->link].r1;
			other  = int_enb_offset[dip->link].r0;
		}

		/* disable this interrupt in the PCIe bridge */
		r = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + offset);
		r &= ~bit;
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + offset, r);

		/*
		 * if both ENABLE0 and ENABLE1 are 0
		 * disable the link interrupt
		 */
		if (r == 0) {
			/* check the other reg */
			if (RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + other) == 0) {
				DPRINTF(("%s: disable link %d\n", __func__, lip->link));

				/* tear down interrupt on this link */
				rmixl_intr_disestablish(lip->ih);

				/* commit NULL interrupt set */
				sc->sc_link_intr[dip->link] = NULL;

				/* schedule delayed free of the old link interrupt set */
				rmixl_pcie_lip_free_callout(lip);
			}
		}
	}

	mutex_exit(&sc->sc_mutex);
}

static void *
rmixl_pcie_intr_establish(void *v, pci_intr_handle_t pih, int ipl,
        int (*func)(void *), void *arg)
{
	rmixl_pcie_softc_t *sc = v;
	u_int link, bitno, irq;
	uint32_t r;
	rmixl_pcie_link_intr_t *lip;
	rmixl_pcie_link_dispatch_t *dip = NULL;
	uint32_t bit;
	u_int offset;

	if (pih == ~0) {
		DPRINTF(("%s: bad pih=%#lx, implies PCI_INTERRUPT_PIN_NONE\n",
			__func__, pih));
		return NULL;
	}

	rmixl_pcie_decompose_pih(pih, &link, &bitno, &irq);
	DPRINTF(("%s: link=%d pin=%d irq=%d\n",
		__func__, link, bitno + 1, irq));

	mutex_enter(&sc->sc_mutex);

	lip = rmixl_pcie_lip_add_1(sc, link, irq, ipl);
	if (lip == NULL)
		return NULL;

	/*
	 * initializae our new interrupt, the last element in dispatch_data[]
	 */
	dip = &lip->dispatch_data[lip->dispatch_count - 1];
	dip->link = link;
	dip->bitno = bitno;
	dip->irq = irq;
	dip->func = func;
	dip->arg = arg;
	dip->counts = RMIXL_PCIE_EVCNT(sc, link, bitno, 0);

	if (bitno < 32) {
		offset = int_enb_offset[link].r0;
		bit = 1 << bitno;
	} else {
		offset = int_enb_offset[link].r1;
		bit = 1 << (bitno - 32);
	}

	/* commit the new link interrupt set */
	sc->sc_link_intr[link] = lip;

	/* enable this interrupt in the PCIe bridge */
	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + offset); 
	r |= bit;
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PCIE_LE + offset, r); 

	mutex_exit(&sc->sc_mutex);
	return dip;
}

rmixl_pcie_link_intr_t *
rmixl_pcie_lip_add_1(rmixl_pcie_softc_t *sc, u_int link, int irq, int ipl)
{
	rmixl_pcie_link_intr_t *lip_old = sc->sc_link_intr[link];
	rmixl_pcie_link_intr_t *lip_new;
	u_int dispatch_count;
	size_t size;

	dispatch_count = 1;
	size = sizeof(rmixl_pcie_link_intr_t);
	if (lip_old != NULL) {
		/*
		 * count only those dispatch elements still in use
		 * unused ones will be pruned during copy
		 * i.e. we are "lazy" there is no rmixl_pcie_lip_sub_1
                 */     
		for (int i=0; i < lip_old->dispatch_count; i++) {
			if (lip_old->dispatch_data[i].func != NULL) {
				dispatch_count++;
				size += sizeof(rmixl_pcie_link_intr_t);
			}
		}
	}

	/*
	 * allocate and initialize link intr struct
	 * with one or more dispatch handles
	 */
	lip_new = malloc(size, M_DEVBUF, M_NOWAIT);
	if (lip_new == NULL) {
#ifdef DIAGNOSTIC
		printf("%s: cannot malloc\n", __func__);
#endif
		return NULL;
	}

	if (lip_old == NULL) {
		/* initialize the link interrupt struct */
		lip_new->sc = sc;
		lip_new->link = link;
		lip_new->ipl = ipl;
		lip_new->ih = rmixl_intr_establish(irq, sc->sc_tmsk,
			ipl, RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
			rmixl_pcie_intr, lip_new, false);
		if (lip_new->ih == NULL)
			panic("%s: cannot establish irq %d", __func__, irq);
	} else {
		/*
		 * all intrs on a link get same ipl and sc
		 * first intr established sets the standard
		 */
		KASSERT(sc == lip_old->sc);
		if (sc != lip_old->sc) {
			printf("%s: sc %p mismatch\n", __func__, sc); 
			free(lip_new, M_DEVBUF);
			return NULL;
		}
		KASSERT (ipl == lip_old->ipl);
		if (ipl != lip_old->ipl) {
			printf("%s: ipl %d mismatch\n", __func__, ipl); 
			free(lip_new, M_DEVBUF);
			return NULL;
		}
		/*
		 * copy lip_old to lip_new, skipping unused dispatch elemets
		 */
		memcpy(lip_new, lip_old, sizeof(rmixl_pcie_link_intr_t));
		for (int j=0, i=0; i < lip_old->dispatch_count; i++) {
			if (lip_old->dispatch_data[i].func != NULL) {
				memcpy(&lip_new->dispatch_data[j],
					&lip_old->dispatch_data[i],
					sizeof(rmixl_pcie_link_dispatch_t));
				j++;
			}
		}

		/*
		 * schedule delayed free of old link interrupt set
		 */
		rmixl_pcie_lip_free_callout(lip_old);
	}
	lip_new->dispatch_count = dispatch_count;

	return lip_new;
}

/*
 * delay free of the old link interrupt set
 * to allow anyone still using it to do so safely
 * XXX 2 seconds should be plenty?
 */
static void
rmixl_pcie_lip_free_callout(rmixl_pcie_link_intr_t *lip)
{
	callout_init(&lip->callout, 0);
	callout_reset(&lip->callout, 2 * hz, rmixl_pcie_lip_free, lip);
}

static void
rmixl_pcie_lip_free(void *arg)
{
	rmixl_pcie_link_intr_t *lip = arg;
	
	callout_destroy(&lip->callout);
	free(lip, M_DEVBUF);
}

static int
rmixl_pcie_intr(void *arg)
{
	rmixl_pcie_link_intr_t *lip = arg;
	u_int link = lip->link;
	int rv = 0;

	uint32_t status0 = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + int_sts_offset[link].r0); 
	uint32_t status1 = RMIXL_IOREG_READ(RMIXL_IO_DEV_PCIE_LE + int_sts_offset[link].r1); 
	uint64_t status = ((uint64_t)status1 << 32) | status0;
	DPRINTF(("%s: %d:%#"PRIx64"\n", __func__, link, status));

	if (status != 0) {
		rmixl_pcie_link_dispatch_t *dip;

		if (status & RMIXL_PCIE_LINK_STATUS_ERRORS)
			rmixl_pcie_link_error_intr(link, status0, status1);

		for (u_int i=0; i < lip->dispatch_count; i++) {
			dip = &lip->dispatch_data[i];
			int (*func)(void *) = dip->func;
			if (func != NULL) {
				uint64_t bit = 1 << dip->bitno;
				if ((status & bit) != 0) {
					(void)(*func)(dip->arg);
					dip->counts[cpu_index(curcpu())].evcnt.ev_count++;
					rv = 1;
				}
			}
		}
	}

	return rv;
}

static void
rmixl_pcie_link_error_intr(u_int link, uint32_t status0, uint32_t status1)
{
	printf("%s: mask %#"PRIx64"\n", 
		__func__, RMIXL_PCIE_LINK_STATUS_ERRORS);
	printf("%s: PCIe Link Error: link=%d status0=%#x status1=%#x\n",
		__func__, link, status0, status1);
#if defined(DDB) && defined(DEBUG)
	Debugger();
#endif
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

/*
 * rmixl_physaddr_init_pcie:
 *	called from rmixl_physaddr_init to get region addrs & sizes
 *	from PCIE CFG, ECFG, IO, MEM BARs
 */
void
rmixl_physaddr_init_pcie(struct extent *ext)
{
	u_long base;
	u_long size;
	uint32_t r;

	r = RMIXL_IOREG_READ(RMIXLS_SBC_PCIE_CFG_BAR);
	if ((r & RMIXL_PCIE_CFG_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_CFG_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)RMIXL_PCIE_CFG_SIZE / (1024 * 1024);
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "CFG", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	r = RMIXL_IOREG_READ(RMIXLS_SBC_PCIE_ECFG_BAR);
	if ((r & RMIXL_PCIE_ECFG_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_ECFG_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)RMIXL_PCIE_ECFG_SIZE / (1024 * 1024);
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "ECFG", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	r = RMIXL_IOREG_READ(RMIXLS_SBC_PCIE_MEM_BAR);
	if ((r & RMIXL_PCIE_MEM_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_MEM_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)(RMIXL_PCIE_MEM_BAR_TO_SIZE((uint64_t)r)
			/ (1024 * 1024));
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "MEM", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	r = RMIXL_IOREG_READ(RMIXLS_SBC_PCIE_IO_BAR);
	if ((r & RMIXL_PCIE_IO_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIE_IO_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)(RMIXL_PCIE_IO_BAR_TO_SIZE((uint64_t)r)
			/ (1024 * 1024));
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "IO", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}
}
