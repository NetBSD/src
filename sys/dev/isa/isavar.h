/*	$NetBSD: isavar.h,v 1.39.6.1 2002/04/06 16:12:08 eeh Exp $	*/

/*-
 * Copyright (c) 1997, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1995 Chris G. Demetriou
 * Copyright (c) 1992 Berkeley Software Design, Inc.
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
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 * 4. The name of Berkeley Software Design must not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI Id: isavar.h,v 1.5 1992/12/01 18:06:00 karels Exp 
 */

#ifndef _DEV_ISA_ISAVAR_H_
#define	_DEV_ISA_ISAVAR_H_

/*
 * Definitions for ISA autoconfiguration.
 */

#include <sys/queue.h>
#include <machine/bus.h>

/* 
 * Structures and definitions needed by the machine-dependent header.
 */
struct isabus_attach_args;

#include <machine/isa_machdep.h>

/*
 * ISA bus attach arguments
 */
struct isabus_attach_args {
	char	*iba_busname;		/* XXX should be common */
	bus_space_tag_t iba_iot;	/* isa i/o space tag */
	bus_space_tag_t iba_memt;	/* isa mem space tag */
	bus_dma_tag_t iba_dmat;		/* isa DMA tag */
	isa_chipset_tag_t iba_ic;
};

/*
 * ISA bus resources.
 */

struct isa_io {
	int ir_addr;
	int ir_size;
};

struct isa_iomem {
	int ir_addr;
	int ir_size;
};

struct isa_irq {
	int ir_irq;
};

struct isa_drq {
	int ir_drq;
};

struct isa_pnpname {
	struct isa_pnpname *ipn_next;
	char *ipn_name;
};

/*
 * Machine-dependent code provides a list of these to describe
 * devices on the ISA bus which should be attached via direct
 * configuration.
 *
 * All of this information is dynamically allocated, so that
 * the ISA bus driver may free all of this information if the
 * bus does not support dynamic attach/detach of devices (e.g.
 * on a docking station).
 *
 * Some info on the "ik_key" field: This is a unique number for
 * each knowndev node.  If, when we need to re-enumerate the
 * knowndevs, we discover that a node with key N is in the old
 * list but not in the new, the device has disappeared.  Similarly,
 * if a node with key M is in the new list but not in the old,
 * the device is new.  Note that the knowndevs list must be
 * sorted in ascending "key" order.
 */
struct isa_knowndev {
	TAILQ_ENTRY(isa_knowndev) ik_list;
	uintptr_t ik_key;
	struct device *ik_claimed;

	/*
	 * The rest of these fields correspond to isa_attach_args
	 * fields.
	 */
	char *ik_pnpname;
	struct isa_pnpname *ik_pnpcompatnames;

	struct isa_io *ik_io;
	int ik_nio;

	struct isa_iomem *ik_iomem;
	int ik_niomem;

	struct isa_irq *ik_irq;
	int ik_nirq;

	struct isa_drq *ik_drq;
	int ik_ndrq;
};

/*
 * ISA driver attach arguments
 */
struct isa_attach_args {
	bus_space_tag_t ia_iot;		/* isa i/o space tag */
	bus_space_tag_t ia_memt;	/* isa mem space tag */
	bus_dma_tag_t ia_dmat;		/* DMA tag */

	isa_chipset_tag_t ia_ic;

	/*
	 * PNP (or other) names to with which we can match a device
	 * driver to a device that machine-dependent code tells us
	 * is there (i.e. support for direct-configuration of ISA
	 * devices).
	 */
	char *ia_pnpname;
	struct isa_pnpname *ia_pnpcompatnames;

	struct isa_io *ia_io;		/* I/O resources */
	int ia_nio;

	struct isa_iomem *ia_iomem;	/* memory resources */
	int ia_niomem;

	struct isa_irq *ia_irq;		/* IRQ resources */
	int ia_nirq;

	struct isa_drq *ia_drq;		/* DRQ resources */
	int ia_ndrq;

	void	*ia_aux;		/* driver specific */
};

/*
 * Test to determine if a given call to an ISA device probe routine
 * is actually an attempt to do direct configuration.
 */
#define	ISA_DIRECT_CONFIG(ia)						\
	((ia)->ia_pnpname != NULL || (ia)->ia_pnpcompatnames != NULL)

#include "locators.h"

/*
 * ISA master bus
 */
struct isa_softc {
	struct	device sc_dev;		/* base device */
	struct device *sc_devp;

	bus_space_tag_t sc_iot;		/* isa io space tag */
	bus_space_tag_t sc_memt;	/* isa mem space tag */
	bus_dma_tag_t sc_dmat;		/* isa DMA tag */

	isa_chipset_tag_t sc_ic;

	TAILQ_HEAD(, isa_knowndev) sc_knowndevs;
	int sc_dynamicdevs;
};

#define		cf_iobase		cf_loc[ISACF_PORT]
#define		cf_iosize		cf_loc[ISACF_SIZE]
#define		cf_maddr		cf_loc[ISACF_IOMEM]
#define		cf_msize		cf_loc[ISACF_IOSIZ]
#define		cf_irq			cf_loc[ISACF_IRQ]
#define		cf_drq			cf_loc[ISACF_DRQ]
#define		cf_drq2			cf_loc[ISACF_DRQ2]

/*
 * ISA interrupt handler manipulation.
 * 
 * To establish an ISA interrupt handler, a driver calls isa_intr_establish()
 * with the interrupt number, type, level, function, and function argument of
 * the interrupt it wants to handle.  Isa_intr_establish() returns an opaque
 * handle to an event descriptor if it succeeds, and returns NULL on failure.
 * (XXX: some drivers can't handle this, since the former behaviour was to
 * invoke panic() on failure). When the system does not accept any of the
 * interrupt types supported by the driver, the driver should fail the attach.
 * Interrupt handlers should return 0 for "interrupt not for me", 1  for
 * "I took care of it", or -1 for "I guess it was mine, but I wasn't
 * expecting it."
 *
 * To remove an interrupt handler, the driver calls isa_intr_disestablish() 
 * with the handle returned by isa_intr_establish() for that handler.
 *
 * The event counter (struct evcnt) associated with an interrupt line
 * (to be used as 'parent' for an ISA device's interrupt handler's evcnt)
 * can be obtained with isa_intr_evcnt().
 */

/* ISA interrupt sharing types */
char	*isa_intr_typename __P((int type));

/*
 * Some ISA devices (e.g. on a VLB) can perform 32-bit DMA.  This
 * flag is passed to bus_dmamap_create() to indicate that fact.
 */
#define	ISABUS_DMA_32BIT	BUS_DMA_BUS1

#endif /* _DEV_ISA_ISAVAR_H_ */
