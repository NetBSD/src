/*	$NetBSD: isavar.h,v 1.38 2000/06/04 19:15:12 cgd Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * ISA driver attach arguments
 */
struct isa_attach_args {
	bus_space_tag_t ia_iot;		/* isa i/o space tag */
	bus_space_tag_t ia_memt;	/* isa mem space tag */
	bus_dma_tag_t ia_dmat;		/* DMA tag */

	isa_chipset_tag_t ia_ic;

	int	ia_iobase;		/* base i/o address */
	int	ia_iosize;		/* span of ports used */
	int	ia_irq;			/* interrupt request */
	int	ia_drq;			/* DMA request */
	int	ia_drq2;		/* second DMA request */
	int	ia_maddr;		/* physical i/o mem addr */
	u_int	ia_msize;		/* size of i/o memory */
	void	*ia_aux;		/* driver specific */
};

#include "locators.h"

#define	IOBASEUNK	ISACF_PORT_DEFAULT	/* i/o address is unknown */
#define	IRQUNK		ISACF_IRQ_DEFAULT	/* interrupt request line is unknown */
#define	DRQUNK		ISACF_DRQ_DEFAULT	/* DMA request line is unknown */
#define	MADDRUNK	ISACF_IOMEM_DEFAULT	/* shared memory address is unknown */

/*
 * ISA master bus
 */
struct isa_softc {
	struct	device sc_dev;		/* base device */

	bus_space_tag_t sc_iot;		/* isa io space tag */
	bus_space_tag_t sc_memt;	/* isa mem space tag */
	bus_dma_tag_t sc_dmat;		/* isa DMA tag */

	isa_chipset_tag_t sc_ic;
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
