/* $NetBSD: ttwogavar.h,v 1.3.34.1 2012/04/17 00:05:57 yamt Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/pci_sgmap_pte64.h>

#define	_FSTORE	(EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long))

/*
 * T2 System Address Map info.
 */
struct ttwoga_sysmap {
	bus_addr_t	tsmap_sio_base;
	bus_size_t	tsmap_sio_syssize;
	bus_size_t	tsmap_sio_bussize;

	bus_addr_t	tsmap_smem_base;
	bus_size_t	tsmap_smem_syssize;
	bus_size_t	tsmap_smem_bussize;

	bus_addr_t	tsmap_dmem_base;
	bus_size_t	tsmap_dmem_syssize;
	bus_size_t	tsmap_dmem_bussize;

	bus_addr_t	tsmap_conf_base;
};

/*
 * A T2 Gate Array's configuration.
 *
 * All of the information that the T2-specific functions need to
 * do their dirty work (and more!).
 */
struct ttwoga_config {
	int	tc_initted;

	int	tc_rev;

	int	tc_hose;

	bus_addr_t tc_io_bus_start, tc_d_mem_bus_start, tc_s_mem_bus_start;

	const struct ttwoga_sysmap *tc_sysmap;

	struct alpha_bus_space tc_iot, tc_memt;
	struct alpha_pci_chipset tc_pc;

	struct alpha_bus_dma_tag tc_dmat_direct;
	struct alpha_bus_dma_tag tc_dmat_sgmap;

	struct alpha_sgmap tc_sgmap;
	int tc_use_tlb;			/* Gamma hardware bug */

	long tc_io_exstorage[_FSTORE];
	long tc_smem_exstorage[_FSTORE];
	long tc_dmem_exstorage[_FSTORE];

	struct extent *tc_io_ex, *tc_d_mem_ex, *tc_s_mem_ex;
	int	tc_mallocsafe;

	u_long	tc_vecbase;
	struct alpha_shared_intr *tc_intrtab;

	void	(*tc_enable_intr)(struct ttwoga_config *, int, int);
	void	(*tc_setlevel)(struct ttwoga_config *, int, int);
	void	(*tc_eoi)(struct ttwoga_config *, int);
};

extern cpuid_t ttwoga_conf_cpu;
extern struct ttwoga_config ttwoga_configuration[];

struct ttwoga_config *ttwoga_init(int, int);
void	ttwoga_pci_init(pci_chipset_tag_t, void *);
void	ttwoga_dma_init(struct ttwoga_config *);

void	ttwoga_bus_io_init(bus_space_tag_t, void *);
void	ttwoga_bus_mem_init(bus_space_tag_t, void *);
