/*	$NetBSD: vmevar.h,v 1.2 1997/07/17 01:56:29 jtk Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Definitions for VME autoconfiguration.
 */

#include <machine/bus.h>

/* 
 * Structures and definitions needed by the machine-dependent header.
 */
struct vmebus_attach_args;

#include <atari/vme/vme_machdep.h>

/*
 * VME bus attach arguments
 */
struct vmebus_attach_args {
	char	*vba_busname;		/* XXX should be common */
	bus_space_tag_t vba_iot;	/* vme i/o space tag */
	bus_space_tag_t vba_memt;	/* vme mem space tag */
	vme_chipset_tag_t vba_vc;
};

/*
 * VME driver attach arguments
 */
struct vme_attach_args {
	bus_space_tag_t va_iot;		/* vme i/o space tag */
	bus_space_tag_t va_memt;	/* vme mem space tag */

	vme_chipset_tag_t va_vc;

	int	va_iobase;		/* base i/o address */
	int	va_iosize;		/* span of ports used */
	int	va_irq;			/* interrupt request */
	int	va_maddr;		/* physical i/o mem addr */
	u_int	va_msize;		/* size of i/o memory */
	void	*va_aux;		/* driver specific */
};

#include "locators.h"

#define	IOBASEUNK	VMECF_IOPORT_DEFAULT	/* i/o address is unknown */
#define	IRQUNK		VMECF_IRQ_DEFAULT	/* interrupt request line is unknown */
#define	MADDRUNK	VMECF_MEM_DEFAULT	/* shared memory address is unknown */

#define		cf_iobase		cf_loc[VMECF_IOPORT]
#define		cf_iosize		cf_loc[VMECF_IOSIZE]
#define		cf_maddr		cf_loc[VMECF_MEM]
#define		cf_msize		cf_loc[VMECF_MEMSIZ]
#define		cf_irq			cf_loc[VMECF_IRQ]

/*
 * VME master bus
 */
struct vme_softc {
	struct	device sc_dev;		/* base device */
	bus_space_tag_t sc_iot;		/* vme io space tag */
	bus_space_tag_t sc_memt;	/* vme mem space tag */

	vme_chipset_tag_t sc_vc;
};

