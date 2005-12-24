/*	$NetBSD: gtvar.h,v 1.10 2005/12/24 20:27:42 perry Exp $	*/

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
#define	_DISCOVERY_DEV_GTVAR_H_

#include <sys/systm.h>

struct gt_softc {
	struct device gt_dev;
	bus_dma_tag_t gt_dmat;
	bus_space_tag_t gt_memt;	/* the GT itself */
	bus_space_tag_t gt_pci0_memt;	/* PCI0 mem space */
	bus_space_tag_t gt_pci0_iot;	/* PCI0 i/o space */
	boolean_t gt_pci0_host;		/* We're host on PCI0 if TRUE */
	bus_space_tag_t gt_pci1_memt;	/* PCI1 mem space */
	bus_space_tag_t gt_pci1_iot;	/* PCI1 i/o space */
	boolean_t gt_pci1_host;		/* We're host on PCI1 if TRUE */

	bus_space_handle_t gt_memh;	/* to access the GT registers */
	int gt_childmask;		/* what children are present */
};

#define	GT_CHILDOK(gt, ga, cd, pos, max) \
	(((ga)->ga_unit) < (max) &&  \
	    !((gt)->gt_childmask & (1 << (((ga)->ga_unit) + (pos)))) && \
	    !strcmp((ga)->ga_name, (cd)->cd_name))

#define	GT_MPSCOK(gt, ga, cd)		GT_CHILDOK((gt), (ga), (cd), 0, 2)
#define	GT_PCIOK(gt, ga, cd)		GT_CHILDOK((gt), (ga), (cd), 2, 2)
#define	GT_ETHEROK(gt, ga, cd)		GT_CHILDOK((gt), (ga), (cd), 4, 3)
#define	GT_OBIOOK(gt, ga, cd)		GT_CHILDOK((gt), (ga), (cd), 7, 5)
#define	GT_I2COK(gt, ga, cd)		GT_CHILDOK((gt), (ga), (cd), 12, 1)

#define	GT_CHILDFOUND(gt, ga, pos) \
	((void)(((gt)->gt_childmask |= (1 << (((ga)->ga_unit) + (pos))))))

#define	GT_MPSCFOUND(gt, ga)		GT_CHILDFOUND((gt), (ga), 0)
#define	GT_PCIFOUND(gt, ga)		GT_CHILDFOUND((gt), (ga), 2)
#define	GT_ETHERFOUND(gt, ga)		GT_CHILDFOUND((gt), (ga), 4)
#define	GT_OBIOFOUND(gt, ga)		GT_CHILDFOUND((gt), (ga), 7)
#define	GT_I2CFOUND(gt, ga)		GT_CHILDFOUND((gt), (ga), 12)

struct gt_attach_args {
	const char *ga_name;		/* class name of device */
	bus_dma_tag_t ga_dmat;		/* dma tag */
	bus_space_tag_t ga_memt;	/* GT bus space tag */
	bus_space_handle_t ga_memh;	/* GT bus space handle */
	int ga_unit;			/* instance of ga_name */
};

struct obio_attach_args {
	const char *oa_name;		/* call name of device */
	bus_space_tag_t oa_memt;	/* bus space tag */
	bus_addr_t oa_offset;		/* offset (absolute) to device */
	bus_size_t oa_size;		/* size (strided) of device */
	int oa_irq;			/* irq */
};

#ifdef _KERNEL
#include "locators.h"

#ifdef DEBUG
extern int gtpci_debug;
#endif

/*
 * Locators for GT private devices, as specified to config.
 */
#define	GT_UNK_UNIT		GTCF_UNIT_DEFAULT	/* wcarded 'function' */

#define	OBIO_UNK_OFFSET		OBIOCF_OFFSET_DEFAULT	/* wcarded 'offset' */

#define	OBIO_UNK_SIZE		OBIOCF_SIZE_DEFAULT	/* wcarded 'size' */

#define	OBIO_UNK_IRQ		OBIOCF_IRQ_DEFAULT	/* wcarded 'irq' */

void	gt_attach_common(struct gt_softc *);
uint32_t gt_read_mpp(void);
int	gt_cfprint(void *, const char *);

/* int     gt_bs_extent_init(struct discovery_bus_space *, char *);  AKB */
int	gt_mii_read(struct device *, struct device *, int, int);
void	gt_mii_write(struct device *, struct device *, int, int, int);
int	gtget_macaddr(struct gt_softc *,int, char *);
void	gt_watchdog_service(void);
bus_addr_t gt_dma_phys_to_bus_mem(bus_dma_tag_t, bus_addr_t);
bus_addr_t gt_dma_bus_mem_to_phys(bus_dma_tag_t, bus_addr_t);

#define	gt_read(gt,o) \
	bus_space_read_4((gt)->gt_memt, (gt)->gt_memh, (o))
#define	gt_write(gt,o,v) \
	bus_space_write_4((gt)->gt_memt, (gt)->gt_memh, (o), (v))

#if defined(__powerpc__)
static inline volatile int
atomic_add(volatile int *p, int	v)
{
	int	rv;
	int	rtmp;

	__asm volatile(
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
