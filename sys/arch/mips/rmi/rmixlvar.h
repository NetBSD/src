/*	$NetBSD: rmixlvar.h,v 1.6.12.1 2014/08/20 00:03:13 tls Exp $	*/

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

#include <mips/cpu.h>
#include <mips/locore.h>
#include <mips/rmi/rmixl_firmware.h>

#include <sys/bus.h>

#include <dev/pci/pcivar.h>

extern void rmixl_pcr_init_core(void);

static inline bool
cpu_rmixl(const struct pridtab *ct)
{
	if (ct->cpu_cid == MIPS_PRID_CID_RMI)
		return true;
	return false;
}

static inline bool
cpu_rmixlr(const struct pridtab *ct)
{
	u_int type = ct->cpu_cidflags & MIPS_CIDFL_RMI_TYPE;
	if (cpu_rmixl(ct) && type == CIDFL_RMI_TYPE_XLR)
		return true;
	return false;
}

static inline bool
cpu_rmixls(const struct pridtab *ct)
{
	u_int type = ct->cpu_cidflags & MIPS_CIDFL_RMI_TYPE;
	if (cpu_rmixl(ct) && type == CIDFL_RMI_TYPE_XLS)
		return true;
	return false;
}

static inline bool
cpu_rmixlp(const struct pridtab *ct)
{
	u_int type = ct->cpu_cidflags & MIPS_CIDFL_RMI_TYPE;
	if (cpu_rmixl(ct) && type == CIDFL_RMI_TYPE_XLP)
		return true;
	return false;
}

static inline int
cpu_rmixl_chip_type(const struct pridtab *ct)
{
	return ct->cpu_cidflags & MIPS_CIDFL_RMI_TYPE;
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

struct rmixl_config {
	uint64_t		 rc_io_pbase;	
	uint64_t		 rc_flash_pbase;	/* FLASH_BAR */
	uint64_t		 rc_flash_mask;		/* FLASH_BAR */
	bus_addr_t		 rc_pci_cfg_pbase;	
	bus_size_t		 rc_pci_cfg_size;
	bus_addr_t		 rc_pci_ecfg_pbase;	
	bus_size_t		 rc_pci_ecfg_size;
	bus_addr_t		 rc_pci_mem_pbase;	
	bus_size_t		 rc_pci_mem_size;
	bus_addr_t		 rc_pci_io_pbase;	
	bus_size_t		 rc_pci_io_size;
	struct mips_bus_space	 rc_obio_eb_memt; 	/* DEVIO -eb */
	struct mips_bus_space	 rc_obio_el_memt; 	/* DEVIO -el */
	struct mips_bus_space	 rc_iobus_memt; 	/* Peripherals IO Bus */
	struct mips_bus_space	 rc_pci_cfg_memt; 	/* PCI CFG  */
	struct mips_bus_space	 rc_pci_ecfg_memt; 	/* PCI ECFG */
	struct mips_bus_space	 rc_pci_memt; 		/* PCI MEM */
	struct mips_bus_space	 rc_pci_iot; 		/* PCI IO  */
	struct mips_bus_dma_tag	 rc_dma_tag;
	bus_dma_tag_t		 rc_64bit_dmat;
	bus_dma_tag_t		 rc_32bit_dmat;
	bus_dma_tag_t		 rc_29bit_dmat;
	struct extent		*rc_phys_ex;	/* Note: MB units */
	struct extent		*rc_obio_eb_ex;
	struct extent		*rc_obio_el_ex;
	struct extent		*rc_iobus_ex;
	struct extent		*rc_pci_cfg_ex;
	struct extent		*rc_pci_ecfg_ex;
	struct extent		*rc_pci_mem_ex;
	struct extent		*rc_pci_io_ex;
	int			 rc_mallocsafe;
	rmixlfw_info_t 		 rc_psb_info;
	rmixlfw_psb_type_t	 rc_psb_type;
	volatile struct rmixlfw_cpu_wakeup_info
				*rc_cpu_wakeup_info;
	const void		*rc_cpu_wakeup_end;
};

extern struct rmixl_config rmixl_configuration;

extern void rmixl_obio_eb_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_obio_el_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_iobus_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_cfg_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_ecfg_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_bus_mem_init(bus_space_tag_t, void *);
extern void rmixl_pci_bus_io_init(bus_space_tag_t, void *);

extern void rmixl_addr_error_init(void);
extern int  rmixl_addr_error_check(void);

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
	uint32_t r;
	int err;
	int s;

	s = splhigh();
	r = rmixl_cache_err_dis();
	(void)*va;			/* probe */
	err = rmixl_cache_err_check();
	rmixl_cache_err_restore(r);
	splx(s);

	return (err == 0);
}

#endif	/* _MIPS_RMI_RMIXLVAR_H_ */
