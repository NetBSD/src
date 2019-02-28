/* $NetBSD: pcihost_fdtvar.h,v 1.1 2019/02/28 00:47:10 jakllsch Exp $ */

/*-
 * Copyright (c) 2018 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Physical address format bit definitions */
#define	PHYS_HI_RELO			__BIT(31)
#define	PHYS_HI_PREFETCH		__BIT(30)
#define	PHYS_HI_ALIASED			__BIT(29)
#define	PHYS_HI_SPACE			__BITS(25,24)
#define	 PHYS_HI_SPACE_CFG		0
#define	 PHYS_HI_SPACE_IO		1
#define	 PHYS_HI_SPACE_MEM32		2
#define	 PHYS_HI_SPACE_MEM64		3
#define	PHYS_HI_BUS			__BITS(23,16)
#define	PHYS_HI_DEVICE			__BITS(15,11)
#define	PHYS_HI_FUNCTION		__BITS(10,8)
#define	PHYS_HI_REGISTER		__BITS(7,0)

extern int pcihost_segment;

enum pcihost_type {
	PCIHOST_CAM = 1,
	PCIHOST_ECAM,
};

struct pcih_bus_space {
	struct bus_space	bst;

	int		(*map)(void *, bus_addr_t, bus_size_t,
			      int, bus_space_handle_t *);
	struct space_range {
		bus_addr_t	bpci;
		bus_addr_t	bbus;
		bus_size_t	size;
	} 			ranges[4];
	size_t			nranges;
};

struct pcihost_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	enum pcihost_type	sc_type;

	u_int			sc_seg;
	u_int			sc_bus_min;
	u_int			sc_bus_max;

	struct arm32_pci_chipset sc_pc;

	struct pcih_bus_space	sc_io;
	struct pcih_bus_space	sc_mem;
};

void	pcihost_init2(struct pcihost_softc *);
void	pcihost_init(pci_chipset_tag_t, void *);
