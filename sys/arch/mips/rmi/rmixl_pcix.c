/*	$NetBSD: rmixl_pcix.c,v 1.9.12.2 2014/08/20 00:03:13 tls Exp $	*/

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
 * PCI configuration support for RMI XLR SoC
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_pcix.c,v 1.9.12.2 2014/08/20 00:03:13 tls Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/intr.h>
#include <sys/malloc.h>
#include <sys/kernel.h>		/* for 'hz' */
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_pcixvar.h>

#include <mips/rmi/rmixl_obiovar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#ifdef	PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

#ifdef PCI_DEBUG
int rmixl_pcix_debug = PCI_DEBUG;
# define DPRINTF(x)	do { if (rmixl_pcix_debug) printf x ; } while (0)
#else
# define DPRINTF(x)
#endif

#ifndef DDB
# define STATIC static
#else
# define STATIC
#endif


/*
 * XLR PCI-X Extended Configuration Registers
 * Note:
 * - MSI-related regs are omitted
 * - Device mode regs are omitted
 */
#define RMIXL_PCIX_ECFG_HOST_BAR0_ADDR	0x100	/* Host BAR0 Address */
#define RMIXL_PCIX_ECFG_HOST_BAR1_ADDR	0x104	/* Host BAR1 Address */
#define RMIXL_PCIX_ECFG_HOST_BAR2_ADDR	0x108	/* Host BAR2 Address */
#define RMIXL_PCIX_ECFG_HOST_BAR3_ADDR	0x10c	/* Host BAR3 Address */
#define RMIXL_PCIX_ECFG_HOST_BAR4_ADDR	0x110	/* Host BAR4 Address */
#define RMIXL_PCIX_ECFG_HOST_BAR5_ADDR	0x114	/* Host BAR5 Address */
#define RMIXL_PCIX_ECFG_HOST_BAR0_SIZE	0x118	/* Host BAR0 Size */
#define RMIXL_PCIX_ECFG_HOST_BAR1_SIZE	0x11c	/* Host BAR1 Size */
#define RMIXL_PCIX_ECFG_HOST_BAR2_SIZE	0x120	/* Host BAR2 Size */
#define RMIXL_PCIX_ECFG_HOST_BAR3_SIZE	0x124	/* Host BAR3 Size */
#define RMIXL_PCIX_ECFG_HOST_BAR4_SIZE	0x128	/* Host BAR4 Size */
#define RMIXL_PCIX_ECFG_HOST_BAR5_SIZE	0x12c	/* Host BAR5 Size */
#define RMIXL_PCIX_ECFG_MATCH_BIT_ADDR	0x130	/* Match Bit Address BAR */
#define RMIXL_PCIX_ECFG_MATCH_BIT_SIZE	0x134	/* Match Bit Size BAR */
#define RMIXL_PCIX_ECFG_XLR_CONTROL	0x138	/* XLR Control reg */
#define RMIXL_PCIX_ECFG_INTR_CONTROL	0x13c	/* Interrupt Control reg */
#define RMIXL_PCIX_ECFG_INTR_STATUS	0x140	/* Interrupt Status reg */
#define RMIXL_PCIX_ECFG_INTR_ERR_STATUS	0x144	/* Interrupt Error Status reg */
#define RMIXL_PCIX_ECFG_HOST_MODE_STS	0x178	/* Host Mode Status */
#define RMIXL_PCIX_ECFG_XLR_MBLE	0x17c	/* XLR Match Byte Lane Enable */
#define RMIXL_PCIX_ECFG_HOST_XROM_ADDR	0x180	/* Host Expansion ROM Address */
#define RMIXL_PCIX_ECFG_HOST_XROM_SIZE	0x184	/* Host Expansion ROM Size */
#define RMIXL_PCIX_ECFG_HOST_MODE_CTL	0x18c	/* Host Mode Control */
#define RMIXL_PCIX_ECFG_TXCAL_CTL	0x1a0	/* TX Calibration Preset Control */
#define RMIXL_PCIX_ECFG_TXCAL_COUNT	0x1a4	/* TX Calibration Preset Count */

/*
 * RMIXL_PCIX_ECFG_INTR_CONTROL bit defines
 */
#define PCIX_INTR_CONTROL_RESV		__BITS(31,8)
#define PCIX_INTR_CONTROL_MSI1_MASK	__BIT(7)
#define PCIX_INTR_CONTROL_MSI0_MASK	__BIT(6)
#define PCIX_INTR_CONTROL_INTD_MASK	__BIT(5)
#define PCIX_INTR_CONTROL_INTC_MASK	__BIT(4)
#define PCIX_INTR_CONTROL_INTB_MASK	__BIT(3)
#define PCIX_INTR_CONTROL_INTA_MASK	__BIT(2)
#define PCIX_INTR_CONTROL_TMSI		__BIT(1)	/* Trigger MSI Interrupt */
#define PCIX_INTR_CONTROL_DIA		__BIT(0)	/* Device Interrupt through INTA Pin */
#define PCIX_INTR_CONTROL_MASK_ALL	\
		(PCIX_INTR_CONTROL_MSI1_MASK|PCIX_INTR_CONTROL_MSI0_MASK	\
		|PCIX_INTR_CONTROL_INTD_MASK|PCIX_INTR_CONTROL_INTC_MASK	\
		|PCIX_INTR_CONTROL_INTB_MASK|PCIX_INTR_CONTROL_INTA_MASK)

/*
 * RMIXL_PCIX_ECFG_INTR_STATUS bit defines
 */
#define PCIX_INTR_STATUS_RESV		__BITS(31,6)
#define PCIX_INTR_STATUS_MSI1		__BIT(5)
#define PCIX_INTR_STATUS_MSI0		__BIT(4)
#define PCIX_INTR_STATUS_INTD		__BIT(3)
#define PCIX_INTR_STATUS_INTC		__BIT(2)
#define PCIX_INTR_STATUS_INTB		__BIT(1)
#define PCIX_INTR_STATUS_INTA		__BIT(0)

/*
 * RMIXL_PCIX_ECFG_INTR_ERR_STATUS bit defines
 */
#define PCIX_INTR_ERR_STATUS_RESa	__BITS(31,5)
#define PCIX_INTR_ERR_STATUS_SERR	__BIT(4)	/* System Error */
#define PCIX_INTR_ERR_STATUS_RESb	__BIT(3)
#define PCIX_INTR_ERR_STATUS_TE		__BIT(2)	/* Target Error */
#define PCIX_INTR_ERR_STATUS_IE		__BIT(1)	/* Initiator Error */
#define PCIX_INTR_ERR_STATUS_RCE	__BIT(0)	/* Retry Count Expired */
#define PCIX_INTR_ERR_STATUS_RESV	\
		(PCIX_INTR_ERR_STATUS_RESa|PCIX_INTR_ERR_STATUS_RESb)

/*
 * RMIXL_PCIX_ECFG_HOST_MODE_CTL bit defines
 */
#define PCIX_HOST_MODE_CTL_HDMSTAT	__BIT(1)	/* Host/Dev Mode status
							 *  read-only
							 *  1 = host
							 *  0 = device
							 */
#define PCIX_HOST_MODE_CTL_HOSTSWRST	__BIT(0)	/* Host soft reset
							 *  set to 1 to reset
							 *  set to 0 to un-reset
							 */


#if BYTE_ORDER == BIG_ENDIAN
# define RMIXL_PCIXREG_BASE	RMIXL_IO_DEV_PCIX_EB
#else
# define RMIXL_PCIXREG_BASE	RMIXL_IO_DEV_PCIX_EL
#endif

#define RMIXL_PCIXREG_VADDR(o)				\
	(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(	\
		rmixl_configuration.rc_io_pbase		\
		+ RMIXL_PCIXREG_BASE + (o))

#define RMIXL_PCIXREG_READ(o)     (*RMIXL_PCIXREG_VADDR(o))
#define RMIXL_PCIXREG_WRITE(o,v)  *RMIXL_PCIXREG_VADDR(o) = (v)


#define RMIXL_PCIX_CONCAT3(a,b,c) a ## b ## c
#define RMIXL_PCIX_BAR_INIT(reg, bar, size, align) {			\
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
	bar = RMIXL_PCIX_CONCAT3(RMIXL_PCIX_,reg,_BAR)(ba, 1);		\
	DPRINTF(("PCIX %s BAR was not enabled by firmware\n"		\
		"enabling %s at phys %#" PRIxBUSADDR ", size %lu MB\n",	\
		__STRING(reg), __STRING(reg), ba, size));		\
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_BRIDGE + 			\
		RMIXL_PCIX_CONCAT3(RMIXLR_SBC_PCIX_,reg,_BAR), bar);	\
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE +			\
		RMIXL_PCIX_CONCAT3(RMIXLR_SBC_PCIX_,reg,_BAR));		\
	DPRINTF(("%s: %s BAR %#x\n", __func__, __STRING(reg), bar));	\
}


#define RMIXL_PCIX_EVCNT(sc, intrpin, cpu)	\
	&(sc)->sc_evcnts[(intrpin) * (ncpu) + (cpu)]


static int	rmixl_pcix_match(device_t, cfdata_t, void *);
static void	rmixl_pcix_attach(device_t, device_t, void *);
static void	rmixl_pcix_init(rmixl_pcix_softc_t *);
static void	rmixl_pcix_init_errors(rmixl_pcix_softc_t *);
static void	rmixl_pcix_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
static void	rmixl_pcix_intcfg(rmixl_pcix_softc_t *);
static void	rmixl_pcix_errata(rmixl_pcix_softc_t *);
static void	rmixl_conf_interrupt(void *, int, int, int, int, int *);
static int	rmixl_pcix_bus_maxdevs(void *, int);
static pcitag_t	rmixl_pcix_make_tag(void *, int, int, int);
static void	rmixl_pcix_decompose_tag(void *, pcitag_t, int *, int *, int *);
void		rmixl_pcix_tag_print(const char *restrict, void *, pcitag_t,				int, vaddr_t, u_long);
static int	rmixl_pcix_conf_setup(rmixl_pcix_softc_t *,
			pcitag_t, int *, bus_space_tag_t *,
			bus_space_handle_t *);
static pcireg_t	rmixl_pcix_conf_read(void *, pcitag_t, int);
static void	rmixl_pcix_conf_write(void *, pcitag_t, int, pcireg_t);

static int	rmixl_pcix_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *
		rmixl_pcix_intr_string(void *, pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *
		rmixl_pcix_intr_evcnt(void *, pci_intr_handle_t);
static pci_intr_handle_t
		rmixl_pcix_make_pih(u_int, u_int);
static void	rmixl_pcix_decompose_pih(pci_intr_handle_t, u_int *, u_int *);
static void	rmixl_pcix_intr_disestablish(void *, void *);
static void	*rmixl_pcix_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static rmixl_pcix_intr_t *
                rmixl_pcix_pip_add_1(rmixl_pcix_softc_t *, int, int);
static void     rmixl_pcix_pip_free_callout(rmixl_pcix_intr_t *);
static void     rmixl_pcix_pip_free(void *);
static int	rmixl_pcix_intr(void *);
static int	rmixl_pcix_error_intr(void *);


CFATTACH_DECL_NEW(rmixl_pcix, sizeof(rmixl_pcix_softc_t),
    rmixl_pcix_match, rmixl_pcix_attach, NULL, NULL); 


static int rmixl_pcix_found;


static int  
rmixl_pcix_match(device_t parent, cfdata_t cf, void *aux)
{        
	uint32_t r;

	/*
	 * PCI-X interface exists on XLR chips only
	 */
	if (! cpu_rmixlr(mips_options.mips_cpu))
		return 0;

	/* XXX
	 * for now there is only one PCI-X Interface on chip
	 * and only one chip in the system
	 * this could change with furture RMI XL family designs
	 * or when we have multi-chip systems.
	 */
	if (rmixl_pcix_found)
		return 0;

	/* read Host Mode Control register */
	r = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_HOST_MODE_CTL);
	r &= PCIX_HOST_MODE_CTL_HDMSTAT;
	if (r == 0)
		return 0;	/* strapped for Device Mode */

	return 1;
}    

static void 
rmixl_pcix_attach(device_t parent, device_t self, void *aux)
{
	rmixl_pcix_softc_t *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct rmixl_config *rcp = &rmixl_configuration;
        struct pcibus_attach_args pba;
	uint32_t bar;

	rmixl_pcix_found = 1;
	sc->sc_dev = self;
	sc->sc_29bit_dmat = obio->obio_29bit_dmat;
	sc->sc_32bit_dmat = obio->obio_32bit_dmat;
	sc->sc_64bit_dmat = obio->obio_64bit_dmat;
	sc->sc_tmsk = obio->obio_tmsk;

	aprint_normal(": RMI XLR PCI-X Interface\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_HIGH);

	rmixl_pcix_intcfg(sc);

	rmixl_pcix_errata(sc);

	/*
	 * check XLR Control Register
	 */
	DPRINTF(("%s: XLR_CONTROL=%#x\n", __func__, 
		RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_XLR_CONTROL)));

	/*
	 * HBAR[0]   if a 32 bit BAR, or
	 * HBAR[0,1] if a 64 bit BAR pair 
	 * must cover all RAM
	 */
	extern u_quad_t mem_cluster_maxaddr;
	uint64_t hbar_addr;
	uint64_t hbar_size;
	uint32_t hbar_size_lo, hbar_size_hi;
	uint32_t hbar_addr_lo, hbar_addr_hi;

	hbar_addr_lo = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_HOST_BAR0_ADDR);
	hbar_addr_hi = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_HOST_BAR1_ADDR);
	hbar_size_lo = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_HOST_BAR0_SIZE);
	hbar_size_hi = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_HOST_BAR1_SIZE);

	hbar_addr = (u_quad_t)(hbar_addr_lo & PCI_MAPREG_MEM_ADDR_MASK);
	hbar_size = hbar_size_lo;
	if ((hbar_size_lo & PCI_MAPREG_MEM_TYPE_64BIT) != 0) {
		hbar_addr |= (uint64_t)hbar_addr_hi << 32;
		hbar_size |= (uint64_t)hbar_size_hi << 32;
	}
	if ((hbar_addr != 0) || (hbar_size < mem_cluster_maxaddr)) {
		int error;

		aprint_error_dev(self, "HostBAR0 addr %#x, size %#x\n",
			hbar_addr_lo, hbar_size_lo);
		if ((hbar_size_lo & PCI_MAPREG_MEM_TYPE_64BIT) != 0)
			aprint_error_dev(self, "HostBAR1 addr %#x, size %#x\n",
				hbar_addr_hi, hbar_size_hi);
		aprint_error_dev(self, "WARNING: firmware PCI-X setup error: "
			"RAM %#"PRIx64"..%#"PRIx64" not accessible by Host BAR, "
			"enabling DMA bounce buffers\n",
			hbar_size, mem_cluster_maxaddr-1);

		/*
		 * force use of bouce buffers for inaccessible RAM addrs
		 */
		if (hbar_size < ((uint64_t)1 << 32)) {
			error = bus_dmatag_subregion(sc->sc_32bit_dmat,
				0, (bus_addr_t)hbar_size, &sc->sc_32bit_dmat,
				BUS_DMA_NOWAIT);
			if (error)
				panic("%s: failed to subregion 32-bit dma tag:"
					 " error %d", __func__, error);
			sc->sc_64bit_dmat = NULL;
		} else {
			error = bus_dmatag_subregion(sc->sc_64bit_dmat,
				0, (bus_addr_t)hbar_size, &sc->sc_64bit_dmat,
				BUS_DMA_NOWAIT);
			if (error)
				panic("%s: failed to subregion 64-bit dma tag:"
					" error %d", __func__, error);
		}
	}

	/*
	 * check PCI-X interface byteswap setup
	 * ensure 'Match Byte Lane' is disabled
	 */
	uint32_t mble;
	mble = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_XLR_MBLE);
#ifdef PCI_DEBUG
	uint32_t mba, mbs;
	mba  = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_MATCH_BIT_ADDR);
	mbs  = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_MATCH_BIT_SIZE);
	DPRINTF(("%s: MBLE=%#x, MBA=%#x, MBS=%#x\n", __func__, mble, mba, mbs));
#endif
	if ((mble & __BIT(40)) != 0)
		RMIXL_PCIXREG_WRITE(RMIXL_PCIX_ECFG_XLR_MBLE, 0);

	/*
	 * get PCI config space base addr from SBC PCIe CFG BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLR_SBC_PCIX_CFG_BAR);
	DPRINTF(("%s: PCIX_CFG_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIX_CFG_BAR_ENB) == 0) {
		u_long n = RMIXL_PCIX_CFG_SIZE / (1024 * 1024);
		RMIXL_PCIX_BAR_INIT(CFG, bar, n, n);
	}
	rcp->rc_pci_cfg_pbase = (bus_addr_t)RMIXL_PCIX_CFG_BAR_TO_BA(bar);
	rcp->rc_pci_cfg_size  = (bus_size_t)RMIXL_PCIX_CFG_SIZE;

	/*
	 * get PCI MEM space base [addr, size] from SBC PCIe MEM BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLR_SBC_PCIX_MEM_BAR);
	DPRINTF(("%s: PCIX_MEM_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIX_MEM_BAR_ENB) == 0) {
		u_long n = 256;				/* 256 MB */
		RMIXL_PCIX_BAR_INIT(MEM, bar, n, n);
	}
	rcp->rc_pci_mem_pbase = (bus_addr_t)RMIXL_PCIX_MEM_BAR_TO_BA(bar);
	rcp->rc_pci_mem_size  = (bus_size_t)RMIXL_PCIX_MEM_BAR_TO_SIZE(bar);

	/*
	 * get PCI IO space base [addr, size] from SBC PCIe IO BAR
	 * initialize it if necessary
 	 */
	bar = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLR_SBC_PCIX_IO_BAR);
	DPRINTF(("%s: PCIX_IO_BAR %#x\n", __func__, bar));
	if ((bar & RMIXL_PCIX_IO_BAR_ENB) == 0) {
		u_long n = 32;				/* 32 MB */
		RMIXL_PCIX_BAR_INIT(IO, bar, n, n);
	}
	rcp->rc_pci_io_pbase = (bus_addr_t)RMIXL_PCIX_IO_BAR_TO_BA(bar);
	rcp->rc_pci_io_size  = (bus_size_t)RMIXL_PCIX_IO_BAR_TO_SIZE(bar);

	/*
	 * initialize the PCI CFG bus space tag
	 */
	rmixl_pci_cfg_bus_mem_init(&rcp->rc_pci_cfg_memt, rcp);
	sc->sc_pci_cfg_memt = &rcp->rc_pci_cfg_memt;

	/*
	 * initialize the PCI MEM and IO bus space tags
	 */
	rmixl_pci_bus_mem_init(&rcp->rc_pci_memt, rcp);
	rmixl_pci_bus_io_init(&rcp->rc_pci_iot, rcp);

	/*
	 * initialize the extended configuration regs
	 */
	rmixl_pcix_init_errors(sc);

	/*
	 * initialize the PCI chipset tag
	 */
	rmixl_pcix_init(sc);

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
 * rmixl_pcix_intcfg - init PCI-X interrupt control
 */
static void
rmixl_pcix_intcfg(rmixl_pcix_softc_t *sc)
{
	size_t size;
	rmixl_pcix_evcnt_t *ev;

	DPRINTF(("%s\n", __func__));

	/* mask all interrupts until they are established */
	RMIXL_PCIXREG_WRITE(RMIXL_PCIX_ECFG_INTR_CONTROL,
		PCIX_INTR_CONTROL_MASK_ALL);

	/*
	 * read-to-clear any pre-existing interrupts
	 * XXX MSI bits in STATUS are also documented as write 1 to clear in PRM
	 */
	(void)RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_STATUS); 
	(void)RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_ERR_STATUS); 

	/* initialize the (non-error interrupt) dispatch handles */
	sc->sc_intr = NULL;

	/*
	 * allocate per-cpu, per-pin interrupt event counters
	 */
	size = ncpu * PCI_INTERRUPT_PIN_MAX * sizeof(rmixl_pcix_evcnt_t);
	ev = malloc(size, M_DEVBUF, M_NOWAIT);
	if (ev == NULL)
		panic("%s: cannot malloc evcnts\n", __func__);
	sc->sc_evcnts = ev;
	for (int pin=PCI_INTERRUPT_PIN_A; pin <= PCI_INTERRUPT_PIN_MAX; pin++) {
		for (int cpu=0; cpu < ncpu; cpu++) {
			ev = RMIXL_PCIX_EVCNT(sc, pin - 1, cpu);
			snprintf(ev->name, sizeof(ev->name),
				"cpu%d, pin %d", cpu, pin);
			evcnt_attach_dynamic(&ev->evcnt, EVCNT_TYPE_INTR,
				NULL, "rmixl_pcix", ev->name);
		}
	}

	/*
	 * establish PCIX error interrupt handler
	 */
	sc->sc_fatal_ih = rmixl_intr_establish(24, sc->sc_tmsk,
		IPL_VM, RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
		rmixl_pcix_error_intr, sc, false);
	if (sc->sc_fatal_ih == NULL)
		panic("%s: cannot establish irq %d", __func__, 24);
}

static void
rmixl_pcix_errata(rmixl_pcix_softc_t *sc)
{
	/* nothing */
}

static void
rmixl_pcix_init(rmixl_pcix_softc_t *sc)
{
	pci_chipset_tag_t pc = &sc->sc_pci_chipset;
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
#endif

	pc->pc_conf_v = (void *)sc;
	pc->pc_attach_hook = rmixl_pcix_attach_hook;
	pc->pc_bus_maxdevs = rmixl_pcix_bus_maxdevs;
	pc->pc_make_tag = rmixl_pcix_make_tag;
	pc->pc_decompose_tag = rmixl_pcix_decompose_tag;
	pc->pc_conf_read = rmixl_pcix_conf_read;
	pc->pc_conf_write = rmixl_pcix_conf_write;

	pc->pc_intr_v = (void *)sc;
	pc->pc_intr_map = rmixl_pcix_intr_map;
	pc->pc_intr_string = rmixl_pcix_intr_string;
	pc->pc_intr_evcnt = rmixl_pcix_intr_evcnt;
	pc->pc_intr_establish = rmixl_pcix_intr_establish;
	pc->pc_intr_disestablish = rmixl_pcix_intr_disestablish;
	pc->pc_conf_interrupt = rmixl_conf_interrupt;

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	/*
	 * Configure the PCI bus.
	 */
	struct rmixl_config *rcp = &rmixl_configuration;

	aprint_normal_dev(sc->sc_dev, "%s: configuring PCI bus\n");

	ioext  = extent_create("pciio",
		rcp->rc_pci_io_pbase,
		rcp->rc_pci_io_pbase + rcp->rc_pci_io_size - 1,
		M_DEVBUF, NULL, 0, EX_NOWAIT);

	memext = extent_create("pcimem",
		rcp->rc_pci_mem_pbase,
		rcp->rc_pci_mem_pbase + rcp->rc_pci_mem_size - 1,
		M_DEVBUF, NULL, 0, EX_NOWAIT);

	pci_configure_bus(pc, ioext, memext, NULL, 0,
	    mips_cache_info.mci_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

static void
rmixl_pcix_init_errors(rmixl_pcix_softc_t *sc)
{
	/* nothing */
}

void
rmixl_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *iline)
{
	DPRINTF(("%s: %p, %d, %d, %d, %d, %p\n",
		__func__, v, bus, dev, ipin, swiz, iline));
}

void
rmixl_pcix_attach_hook(device_t parent, device_t self,
	struct pcibus_attach_args *pba)
{
	DPRINTF(("%s: pba_bus %d, pba_bridgetag %p, pc_conf_v %p\n",
		__func__, pba->pba_bus, pba->pba_bridgetag,
		pba->pba_pc->pc_conf_v));
}

int
rmixl_pcix_bus_maxdevs(void *v, int busno)
{
	return (32);	/* XXX depends on the family of XLS SoC */
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
rmixl_pcix_make_tag(void *v, int bus, int dev, int fun)
{
	return ((bus << 16) | (dev << 11) | (fun << 8));
}

void
rmixl_pcix_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

void
rmixl_pcix_tag_print(const char *restrict s, void *v, pcitag_t tag, int offset,
	vaddr_t va, u_long r)
{
	int bus, dev, fun;

	rmixl_pcix_decompose_tag(v, tag, &bus, &dev, &fun);
	printf("%s: %d/%d/%d/%d - %#" PRIxVADDR ":%#lx\n",
		s, bus, dev, fun, offset, va, r);
}

static int
rmixl_pcix_conf_setup(rmixl_pcix_softc_t *sc,
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
rmixl_pcix_conf_read(void *v, pcitag_t tag, int offset)
{
	rmixl_pcix_softc_t *sc = v;
	static bus_space_handle_t bsh;
	bus_space_tag_t bst;
	pcireg_t rv;
	uint64_t cfg0;

	mutex_enter(&sc->sc_mutex);

	if (rmixl_pcix_conf_setup(sc, tag, &offset, &bst, &bsh) == 0) {
		cfg0 = rmixl_cache_err_dis();
		rv = bus_space_read_4(bst, bsh, (bus_size_t)offset);
		if (rmixl_cache_err_check() != 0) {
#ifdef DIAGNOSTIC
			int bus, dev, fun;

			rmixl_pcix_decompose_tag(v, tag, &bus, &dev, &fun);
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
rmixl_pcix_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	rmixl_pcix_softc_t *sc = v;
	static bus_space_handle_t bsh;
	bus_space_tag_t bst;
	uint64_t cfg0;

	mutex_enter(&sc->sc_mutex);

	if (rmixl_pcix_conf_setup(sc, tag, &offset, &bst, &bsh) == 0) {
		cfg0 = rmixl_cache_err_dis();
		bus_space_write_4(bst, bsh, (bus_size_t)offset, val);
		if (rmixl_cache_err_check() != 0) {
#ifdef DIAGNOSTIC
			int bus, dev, fun;

			rmixl_pcix_decompose_tag(v, tag, &bus, &dev, &fun);
			printf("%s: %d/%d/%d, offset %#x: bad address\n",
				__func__, bus, dev, fun, offset);
#endif
		}
		rmixl_cache_err_restore(cfg0);
	}

	mutex_exit(&sc->sc_mutex);
}

int
rmixl_pcix_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *pih)
{
	const u_int irq = 16;	/* PCIX index in IRT */

#ifdef DEBUG
	DPRINTF(("%s: ps_bus %d, pa_intrswiz %#x, pa_intrtag %#lx,"
		" pa_intrpin %d,  pa_intrline %d, pa_rawintrpin %d\n",
		__func__, pa->pa_bus, pa->pa_intrswiz, pa->pa_intrtag,
		pa->pa_intrpin, pa->pa_intrline, pa->pa_rawintrpin));
#endif

	if (pa->pa_intrpin != PCI_INTERRUPT_PIN_NONE)
		*pih = rmixl_pcix_make_pih(pa->pa_intrpin - 1, irq);
	else
		*pih = ~0;

	return 0;
}

const char *
rmixl_pcix_intr_string(void *v, pci_intr_handle_t pih, char *buf, size_t len)
{
	u_int bitno, irq;

	rmixl_pcix_decompose_pih(pih, &bitno, &irq);

	if (! cpu_rmixlr(mips_options.mips_cpu))
		panic("%s: cpu %#x not supported\n",
			__func__, mips_options.mips_cpu_id);

	strlcpy(buf, rmixl_intr_string(RMIXL_IRT_VECTOR(irq)), len);
	return buf;
}

const struct evcnt *
rmixl_pcix_intr_evcnt(void *v, pci_intr_handle_t pih)
{
	return NULL;
}

static pci_intr_handle_t
rmixl_pcix_make_pih(u_int bitno, u_int irq)
{
	pci_intr_handle_t pih;

	KASSERT(bitno < 64);
	KASSERT(irq < 32);

	pih  = (irq << 6);
	pih |= bitno;

	return pih;
}

static void
rmixl_pcix_decompose_pih(pci_intr_handle_t pih, u_int *bitno, u_int *irq)
{
	*bitno = (u_int)(pih & 0x3f);
	*irq = (u_int)(pih >> 6);

	KASSERT(*bitno < 64);
	KASSERT(*irq < 31);
}

static void
rmixl_pcix_intr_disestablish(void *v, void *ih)
{
	rmixl_pcix_softc_t *sc = v;
	rmixl_pcix_dispatch_t *dip = ih;
	rmixl_pcix_intr_t *pip = sc->sc_intr;
	bool busy;

	DPRINTF(("%s: pin=%d irq=%d\n",
		__func__, dip->bitno + 1, dip->irq));
	KASSERT(dip->bitno < RMIXL_PCIX_NINTR);

	mutex_enter(&sc->sc_mutex);

	dip->func = NULL;	/* prevent further dispatch */

	/*
	 * if no other dispatch handle is using this interrupt,
	 * we can disable it
	 */
	busy = false;
	for (int i=0; i < pip->dispatch_count; i++) {
		rmixl_pcix_dispatch_t *d = &pip->dispatch_data[i];
		if (d == dip)
			continue;
		if (d->bitno == dip->bitno) {
			busy = true;
			break;
		}
	}
	if (! busy) {
		uint32_t bit = 1 << (dip->bitno + 2);
		uint32_t r;

		r = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_CONTROL);
		r |= bit;		/* set mask */
		RMIXL_PCIXREG_WRITE(RMIXL_PCIX_ECFG_INTR_CONTROL, r);
		DPRINTF(("%s: disabled pin %d\n", __func__, dip->bitno + 1));

		pip->intenb &= ~(1 << dip->bitno);

		if ((r & PCIX_INTR_CONTROL_MASK_ALL) == 0) {
			/* tear down interrupt for this pcix */
			rmixl_intr_disestablish(pip->ih);

			/* commit NULL interrupt set */
			sc->sc_intr = NULL;

			/* schedule delayed free of the old interrupt set */
			rmixl_pcix_pip_free_callout(pip);
		}
	}

	mutex_exit(&sc->sc_mutex);
}

static void *
rmixl_pcix_intr_establish(void *v, pci_intr_handle_t pih, int ipl,
        int (*func)(void *), void *arg)
{
	rmixl_pcix_softc_t *sc = v;
	u_int bitno, irq;
	rmixl_pcix_intr_t *pip;
	rmixl_pcix_dispatch_t *dip = NULL;

	if (pih == ~0) {
		DPRINTF(("%s: bad pih=%#lx, implies PCI_INTERRUPT_PIN_NONE\n",
			__func__, pih));
		return NULL;
	}

	rmixl_pcix_decompose_pih(pih, &bitno, &irq);
	DPRINTF(("%s: pin=%d irq=%d\n", __func__, bitno + 1, irq));

	KASSERT(bitno < RMIXL_PCIX_NINTR);

	/*
	 * all PCI-X device intrs get same ipl
	 */
	KASSERT(ipl == IPL_VM);

	mutex_enter(&sc->sc_mutex);

	pip = rmixl_pcix_pip_add_1(sc, irq, ipl); 
	if (pip == NULL)
		return NULL;

	/*
	 * initializae our new interrupt, the last element in dispatch_data[] 
	 */
	dip = &pip->dispatch_data[pip->dispatch_count - 1];
	dip->bitno = bitno;
	dip->irq = irq;
	dip->func = func;
	dip->arg = arg;
	dip->counts = RMIXL_PCIX_EVCNT(sc, bitno, 0);
#if NEVER
	snprintf(dip->count_name, sizeof(dip->count_name),
		"pin %d", bitno + 1);
	evcnt_attach_dynamic(&dip->count, EVCNT_TYPE_INTR, NULL,
		"rmixl_pcix", dip->count_name);
#endif

	/* commit the new interrupt set */
	sc->sc_intr = pip;

	/* enable this interrupt in the PCIX controller, if necessary */
	if ((pip->intenb & (1 << bitno)) == 0) {
		uint32_t bit = 1 << (bitno + 2);
		uint32_t r;

		r = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_CONTROL); 
		r &= ~bit;	/* clear mask */
		RMIXL_PCIXREG_WRITE(RMIXL_PCIX_ECFG_INTR_CONTROL, r); 

		pip->sc = sc;
		pip->ipl = ipl;
		pip->intenb |= 1 << bitno;
		DPRINTF(("%s: enabled pin %d\n", __func__, bitno + 1));
	}

	mutex_exit(&sc->sc_mutex);
	return dip;
}

rmixl_pcix_intr_t *
rmixl_pcix_pip_add_1(rmixl_pcix_softc_t *sc, int irq, int ipl)
{
	rmixl_pcix_intr_t *pip_old = sc->sc_intr;
	rmixl_pcix_intr_t *pip_new;
	u_int dispatch_count;
	size_t size;

	dispatch_count = 1;
	size = sizeof(rmixl_pcix_intr_t);
	if (pip_old != NULL) {
		/*
		 * count only those dispatch elements still in use
		 * unused ones will be pruned during copy
		 * i.e. we are "lazy" there is no rmixl_pcix_pip_sub_1
		 */
		for (int i=0; i < pip_old->dispatch_count; i++) {
			if (pip_old->dispatch_data[i].func != NULL) {
				dispatch_count++;
				size += sizeof(rmixl_pcix_intr_t);
			}
		}
	}

	/*
	 * allocate and initialize softc intr struct
	 * with one or more dispatch handles
	 */
	pip_new = malloc(size, M_DEVBUF, M_NOWAIT|M_ZERO);
	if (pip_new == NULL) {
#ifdef DIAGNOSTIC
		printf("%s: cannot malloc\n", __func__);
#endif
		return NULL;
	}

	if (pip_old == NULL) {
		/* initialize the interrupt struct */
		pip_new->sc = sc;
		pip_new->ipl = ipl;
		pip_new->ih = rmixl_intr_establish(irq, sc->sc_tmsk,
			ipl, RMIXL_TRIG_LEVEL, RMIXL_POLR_HIGH,
			rmixl_pcix_intr, pip_new, false);
		if (pip_new->ih == NULL)
			panic("%s: cannot establish irq %d", __func__, irq);
	} else {
		/*
		 * all intrs on a softc get same ipl and sc
		 * first intr established sets the standard
		 */
		KASSERT(sc == pip_old->sc);
		if (sc != pip_old->sc) {
			printf("%s: sc %p mismatch\n", __func__, sc); 
			free(pip_new, M_DEVBUF);
			return NULL;
		}
		KASSERT (ipl == pip_old->ipl);
		if (ipl != pip_old->ipl) {
			printf("%s: ipl %d mismatch\n", __func__, ipl); 
			free(pip_new, M_DEVBUF);
			return NULL;
		}
		/*
		 * copy pip_old to pip_new, skipping unused dispatch elemets
		 */
		memcpy(pip_new, pip_old, sizeof(rmixl_pcix_intr_t));
		for (int j=0, i=0; i < pip_old->dispatch_count; i++) {
			if (pip_old->dispatch_data[i].func != NULL) {
				memcpy(&pip_new->dispatch_data[j],
					&pip_old->dispatch_data[i],
					sizeof(rmixl_pcix_dispatch_t));
				j++;
			}
		}

		/*
		 * schedule delayed free of old interrupt set
		 */
		rmixl_pcix_pip_free_callout(pip_old);
	}
	pip_new->dispatch_count = dispatch_count;

	return pip_new;
}

/*
 * delay free of the old interrupt set
 * to allow anyone still using it to do so safely
 * XXX 2 seconds should be plenty?
 */
static void
rmixl_pcix_pip_free_callout(rmixl_pcix_intr_t *pip)
{
	callout_init(&pip->callout, 0);
	callout_reset(&pip->callout, 2 * hz, rmixl_pcix_pip_free, pip);
}       
        
static void
rmixl_pcix_pip_free(void *arg)
{       
	rmixl_pcix_intr_t *pip = arg;

	callout_destroy(&pip->callout);
	free(pip, M_DEVBUF);
}

static int
rmixl_pcix_intr(void *arg)
{
	rmixl_pcix_intr_t *pip = arg;
	int rv = 0;

	uint32_t status = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_STATUS); 
	DPRINTF(("%s: %#x\n", __func__, status));

	if (status != 0) {
		for (int i=0; i < pip->dispatch_count; i++) {
			rmixl_pcix_dispatch_t *dip = &pip->dispatch_data[i];
			uint32_t bit = 1 << dip->bitno;
			int (*func)(void *) = dip->func;
			if ((func != NULL) && (status & bit) != 0) {
				(void)(*func)(dip->arg);
				dip->counts[cpu_index(curcpu())].evcnt.ev_count++;
				rv = 1;
			}
		}
	}
	return rv;
}

static int
rmixl_pcix_error_intr(void *arg)
{
	rmixl_pcix_softc_t *sc = arg;
	uint32_t error_status;

	error_status = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_ERR_STATUS); 

#ifdef DIAGNOSTIC
	printf("%s: error status %#x\n", __func__, error_status);
#endif

#if DDB
	Debugger();
#endif

	/* XXX reset and recover? */

	panic("%s: error %#x\n", device_xname(sc->sc_dev), error_status);
}

/*
 * rmixl_physaddr_init_pcix:
 *	called from rmixl_physaddr_init to get region addrs & sizes
 *	from PCIX CFG, ECFG, IO, MEM BARs
 */
void
rmixl_physaddr_init_pcix(struct extent *ext)
{
	u_long base;
	u_long size;
	uint32_t r;

	r = RMIXL_PCIXREG_READ(RMIXLR_SBC_PCIX_CFG_BAR);
	if ((r & RMIXL_PCIX_CFG_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIX_CFG_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)RMIXL_PCIX_CFG_SIZE / (1024 * 1024);
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "CFG", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLR_SBC_PCIX_MEM_BAR);
	if ((r & RMIXL_PCIX_MEM_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIX_MEM_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)(RMIXL_PCIX_MEM_BAR_TO_SIZE((uint64_t)r)
			/ (1024 * 1024));
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "MEM", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	r = RMIXL_IOREG_READ(RMIXL_IO_DEV_BRIDGE + RMIXLR_SBC_PCIX_IO_BAR);
	if ((r & RMIXL_PCIX_IO_BAR_ENB) != 0) {
		base = (u_long)(RMIXL_PCIX_IO_BAR_TO_BA((uint64_t)r)
			/ (1024 * 1024));
		size = (u_long)(RMIXL_PCIX_IO_BAR_TO_SIZE((uint64_t)r)
			/ (1024 * 1024));
		DPRINTF(("%s: %d: %s: 0x%08x -- 0x%010lx:%ld MB\n", __func__,
			__LINE__, "IO", r, base * 1024 * 1024, size));
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}
}

#ifdef DDB
int rmixl_pcix_intr_chk(void);
int
rmixl_pcix_intr_chk(void)
{
	uint32_t control, status, error_status;

	control = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_CONTROL); 
	status = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_STATUS); 
	error_status = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_ERR_STATUS); 

	printf("%s: %#x, %#x, %#x\n", __func__, control, status, error_status);

	control |= PCIX_INTR_CONTROL_DIA;
	RMIXL_PCIXREG_WRITE(RMIXL_PCIX_ECFG_INTR_CONTROL, control); 

	control = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_CONTROL); 
	status = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_STATUS); 
	error_status = RMIXL_PCIXREG_READ(RMIXL_PCIX_ECFG_INTR_ERR_STATUS); 

	printf("%s: %#x, %#x, %#x\n", __func__, control, status, error_status);

	return 0;
}
#endif
