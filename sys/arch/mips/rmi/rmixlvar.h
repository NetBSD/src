/*	$NetBSD: rmixlvar.h,v 1.1.2.22 2011/12/27 19:58:19 matt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef _MIPS_RMI_RMIXLVAR_H_
#define _MIPS_RMI_RMIXLVAR_H_

#include <sys/bus.h>
#include <sys/extent.h>

#include <dev/pci/pcivar.h>

#include <mips/cpu.h>
#include <mips/locore.h>

#include <mips/rmi/rmixl_firmware.h>
#include <mips/rmi/rmixlreg.h>

void rmixl_pcr_init_core(bool);

static inline int
cpu_rmixl_chip_type(const struct pridtab *ct)
{
	return ct->cpu_cidflags & MIPS_CIDFL_RMI_TYPE;
}

static inline bool
cpu_rmixl(const struct pridtab *ct)
{
	return ct->cpu_cid == MIPS_PRID_CID_RMI;
}

static inline bool
cpu_rmixlr(const struct pridtab *ct)
{
	return cpu_rmixl(ct) && cpu_rmixl_chip_type(ct) == CIDFL_RMI_TYPE_XLR;
}

static inline bool
cpu_rmixls(const struct pridtab *ct)
{
	return cpu_rmixl(ct) && cpu_rmixl_chip_type(ct) == CIDFL_RMI_TYPE_XLS;
}

static inline bool
cpu_rmixlp(const struct pridtab *ct)
{
	return cpu_rmixl(ct) && cpu_rmixl_chip_type(ct) == CIDFL_RMI_TYPE_XLP;
}

typedef enum {
	PSB_TYPE_UNKNOWN=0,
	PSB_TYPE_RMI,
	PSB_TYPE_DELL,
} rmixlfw_psb_type_t;

static inline const char *
rmixlfw_psb_type_name(rmixlfw_psb_type_t type)
{
	switch(type) {
	case PSB_TYPE_UNKNOWN:
		return "unknown";
	case PSB_TYPE_RMI:
		return "RMI";
	case PSB_TYPE_DELL:
		return "DELL";
	default:
		return "undefined";
	}
}

typedef enum {
	RMIXLP_8XX,
	RMIXLP_4XX,
	/* These next 4 need to be in this order */
	RMIXLP_3XX,
	RMIXLP_3XXL,
	RMIXLP_3XXH,
	RMIXLP_3XXQ,
	RMIXLP_ANY,		/* must be last */
} rmixlp_variant_t;

struct rmixl_region {
	bus_addr_t		r_pbase;
	bus_size_t		r_size;
};

struct rmixl_config {
	struct rmixl_region	rc_io;	
	struct rmixl_region	rc_flash[4];		/* FLASH_BAR */
	struct rmixl_region	rc_pci_cfg;
	struct rmixl_region	rc_pci_ecfg;
	struct rmixl_region	rc_pci_mem;
	struct rmixl_region	rc_pci_io;
	struct rmixl_region	rc_pci_link_mem[4];
	struct rmixl_region	rc_pci_link_io[4];
	struct rmixl_region	rc_srio_mem;
	struct rmixl_region	rc_norflash[8];		/* XLP 8 CS for NOR */
	struct mips_bus_space	rc_obio_eb_memt; 	/* DEVIO -eb */
	struct mips_bus_space	rc_obio_el_memt; 	/* DEVIO -el */
	struct mips_bus_space	rc_iobus_memt; 		/* Peripherals IO Bus */
	struct mips_bus_space	rc_pci_cfg_memt; 	/* PCI CFG */
	struct mips_bus_space	rc_pci_ecfg_eb_memt; 	/* PCI ECFG */
	struct mips_bus_space	rc_pci_ecfg_el_memt; 	/* PCI ECFG */
	struct mips_bus_space	rc_pci_memt; 		/* PCI MEM */
	struct mips_bus_space	rc_pci_iot; 		/* PCI IO */
	struct mips_bus_space	rc_srio_memt; 		/* SRIO MEM */
	struct mips_bus_dma_tag	rc_dma_tag;
	struct mips_pci_chipset rc_pci_chipset;		/* pci_chipset_t */
	bus_space_handle_t	rc_pci_cfg_memh;
	bus_space_handle_t	rc_pci_ecfg_eb_memh;
	bus_space_handle_t	rc_pci_ecfg_el_memh;
	bus_dma_tag_t		rc_dmat64;
	bus_dma_tag_t		rc_dmat32;
	bus_dma_tag_t		rc_dmat29;
	struct extent *		rc_phys_ex;		/* Note: MB units */
	struct extent *		rc_obio_eb_ex;
	struct extent *		rc_obio_el_ex;
	struct extent *		rc_iobus_ex;
	struct extent *		rc_pci_mem_ex;
	struct extent *		rc_pci_io_ex;
	struct extent *		rc_srio_mem_ex;
	int			rc_mallocsafe;
	rmixlfw_info_t 		rc_psb_info;
	rmixlfw_psb_type_t	rc_psb_type;
	volatile struct rmixlfw_cpu_wakeup_info *
				rc_cpu_wakeup_info;
	const void *		rc_cpu_wakeup_end;
};

extern struct rmixl_config rmixl_configuration;
extern const char *rmixl_cpuname;

extern void rmixl_obio_eb_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_obio_el_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_iobus_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_cfg_el_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_cfg_eb_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_ecfg_el_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_ecfg_eb_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_bus_io_init(bus_space_tag_t, void *);

void	rmixlp_pcie_pc_init(void);

extern void rmixl_addr_error_init(void);
extern int  rmixl_addr_error_check(void);
extern rmixlp_variant_t rmixl_xlp_variant;

extern uint64_t rmixl_mfcr(u_int);
extern void rmixl_mtcr(uint64_t, u_int);

extern void rmixl_eirr_ack(uint64_t, uint64_t, uint64_t);


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

static inline int
rmixl_probe_4(volatile uint32_t *va)
{
	uint32_t tmp;
	uint32_t r;
	int err;
	int s;

	s = splhigh();
	r = rmixl_cache_err_dis();
	tmp = *va;			/* probe */
	err = rmixl_cache_err_check();
	rmixl_cache_err_restore(r);
	splx(s);

	return (err == 0);
}

static inline uint32_t
rmixlp_read_4(uint32_t tag, bus_size_t offset)
{
#if 0
	const struct rmixl_config * const rcp = &rmixl_configuration;

	return bus_space_read_4(rcp->rc_pci_ecfg_memt, rcp->rc_pci_ecfg_memh,
	    offset);
#else
	const paddr_t ecfg_addr = rmixl_configuration.rc_pci_ecfg.r_pbase
	    + tag + offset;

	return be32toh(*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(ecfg_addr));
#endif
}

static inline uint64_t
rmixlp_read_8(uint32_t tag, bus_size_t offset)
{
#if 0
	const struct rmixl_config * const rcp = &rmixl_configuration;

	return bus_space_read_8(rcp->rc_pci_ecfg_memt, rcp->rc_pci_ecfg_memh,
	    offset);
#else
	const paddr_t ecfg_addr = rmixl_configuration.rc_pci_ecfg.r_pbase
	    + tag + offset;

	return be64toh(*(volatile uint64_t *)MIPS_PHYS_TO_KSEG1(ecfg_addr));
#endif
}

static inline void
rmixlp_write_4(uint32_t tag, bus_size_t offset, uint32_t v)
{
	const paddr_t ecfg_addr = rmixl_configuration.rc_pci_ecfg.r_pbase
	    + tag + offset;

	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(ecfg_addr) = htobe32(v);
	__asm __volatile("sync");
}

static inline void
rmixlp_write_8(uint32_t tag, bus_size_t offset, uint64_t v)
{
	const paddr_t ecfg_addr = rmixl_configuration.rc_pci_ecfg.r_pbase
	    + tag + offset;

	*(volatile uint64_t *)MIPS_PHYS_TO_KSEG1(ecfg_addr) = htobe64(v);
	__asm __volatile("sync");
}

static inline void
rmixl_physaddr_add(struct extent *ext, const char *name,
    struct rmixl_region *rp, bus_addr_t xpbase, bus_size_t xsize)
{
	rp->r_pbase = xpbase;
	rp->r_size = xsize;
	u_long base = xpbase >> 20;
	u_long size = xsize >> 20;
	if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0) {
		panic("%s: %s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
		    "failed", __func__, name, ext, base, size, EX_NOWAIT);
	}
}

#endif	/* _MIPS_RMI_RMIXLVAR_H_ */
