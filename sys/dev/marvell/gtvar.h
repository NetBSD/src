/*	$NetBSD: gtvar.h,v 1.1 2003/03/05 22:08:23 matt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * gtvar.h -- placeholder for GT system controller driver
 */
#ifndef _DISCOVERY_DEV_GTVAR_H_
#define _DISCOVERY_DEV_GTVAR_H_

#include <sys/systm.h>

#define GTPCI_NBUS 2

struct gt_softc {
	struct device gt_dev;
	vaddr_t gt_vbase;		/* mapped GT base address */
#if 0
	vaddr_t gt_iobat_vbase;		/* I/O BAT virtual base */
	paddr_t gt_iobat_pbase;		/* I/O BAT physical base */
	vsize_t gt_iobat_mask;		/* I/O BAT mask (signif. bits only) */
#endif
	struct pci_chipset *gt_pcis[GTPCI_NBUS];
	bus_dma_tag_t gt_dmat;
	bus_space_tag_t gt_memt;
	bus_space_tag_t gt_pci0_memt;
	bus_space_tag_t gt_pci0_iot;
	bus_space_tag_t gt_pci1_memt;
	bus_space_tag_t gt_pci1_iot;
};

struct gt_attach_args {
	const char *ga_name;		/* class name of device */
	bus_dma_tag_t ga_dmat;		/* dma tag */
	bus_space_tag_t ga_memt;	/* GT bus space tag */
	int ga_unit;			/* instance of ga_name */
};

#ifdef _KERNEL
#include "locators.h"

#ifdef DEBUG
extern int gtpci_debug;
#endif

/*
 * Locators for GT private devices, as specified to config.
 */
#define gtcf_dev		cf_loc[GTCF_DEV]
#define	GT_UNK_DEV		GTCF_DEV_DEFAULT	/* wcarded 'dev' */

#define gtcf_function		cf_loc[GTCF_FUNCTION]
#define	GT_UNK_FUNCTION		GTCF_FUNCTION_DEFAULT	/* wcarded 'function' */

void	gt_attach_common(struct gt_softc *);
uint32_t gt_read_mpp(void);
int	gt_cfprint (void *, const char *);

void	gtpci_config(struct gt_softc *, int,
		     bus_space_tag_t, bus_space_tag_t, bus_dma_tag_t);
void	gteth_config (struct gt_softc *, int);
void	gtmpsc_config (struct gt_softc *, int);

/* int     gt_bs_extent_init(struct discovery_bus_space *, char *);  AKB */
int	gt_mii_read (struct device *, struct device *, int, int);
void	gt_mii_write (struct device *, struct device *, int, int, int);
int	gtget_macaddr(struct gt_softc *gt, int function, char *enaddr);
void	gtpci_config_bus(struct pci_chipset *pc, int busno);
void	gt_setup(struct device *gt);
void	gt_watchdog_service(void);

#define	gt_read(a,b)	gt_read_4(a,b)
#define	gt_write(a,b,c)	gt_write_4(a,b,c)

static __inline uint32_t
gt_read_4(struct device *dv, bus_addr_t off)
{
	struct gt_softc *gt = (struct gt_softc *) dv;
	uint32_t rv;

	__asm __volatile("eieio; lwbrx %0,%1,%2; eieio;"
	    : "=r"(rv)
	    : "b"(gt->gt_vbase), "r"(off));

	return rv;
}

static __inline void
gt_write_4(struct device *dv, bus_addr_t off, uint32_t v)
{
	struct gt_softc *gt = (struct gt_softc *) dv;

	__asm __volatile("eieio; stwbrx %0,%1,%2; eieio;"
	    ::	"r"(v), "b"(gt->gt_vbase), "r"(off));
}

#if defined(__powerpc__)
static volatile inline int atomic_add(volatile int *p, int v);

static volatile inline int
atomic_add(volatile int *p, int	v)
{
	int	rv;
	int	rtmp;

	__asm __volatile(
	"1:	lwarx	%0,0,%2\n"
	"	add	%1,%3,%0\n"
	"	stwcx.	%1,0,%2\n"
	"	bne-	1b\n"
	"	sync"
			: "=&r"(rv), "=&r"(rtmp)
			: "r"(p), "r"(v)
			: "cc");

	return rv;
}

#endif /* __powerpc__ */

#endif /* _KERNEL */

#endif /* _DISCOVERY_DEV_GTVAR_H_ */
