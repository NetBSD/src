/*	$NetBSD: obiovar.h,v 1.1 2002/05/02 15:17:58 nonaka Exp $	*/

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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 *      This product includes software developed by Berkeley Software
 *      Design, Inc.
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
 *      BSDI Id: isavar.h,v 1.5 1992/12/01 18:06:00 karels Exp 
 */

#ifndef _PREP_OBIOVAR_H_
#define _PREP_OBIOVAR_H_

#include <sys/queue.h>
#include <machine/bus.h>

/*
 * driver attach arguments
 */
struct obio_attach_args {
	bus_space_tag_t oa_iot;		/* i/o space tag */
	bus_space_tag_t oa_memt;	/* mem space tag */

	int	oa_iobase;		/* base i/o address */
	int	oa_iosize;		/* span of ports used */
	int	oa_irq;			/* interrupt request */
	int	oa_maddr;		/* physical i/o mem addr */
	u_int	oa_msize;		/* size of i/o memory */
	void	*oa_aux;		/* driver specific */
};

#include "locators.h"

#define	IOBASEUNK	OBIOCF_PORT_DEFAULT	/* i/o address is unknown */
#define	IRQUNK		OBIOCF_IRQ_DEFAULT	/* interrupt request line is unknown */
#define	MADDRUNK	OBIOCF_IOMEM_DEFAULT	/* shared memory address is unknown */

/*
 * master bus
 */
struct obio_softc {
	struct	device sc_dev;		/* base device */

	bus_space_tag_t sc_iot;		/* io space tag */
	bus_space_tag_t sc_memt;	/* mem space tag */
};

#define		cf_iobase		cf_loc[OBIOCF_PORT]
#define		cf_iosize		cf_loc[OBIOCF_SIZE]
#define		cf_maddr		cf_loc[OBIOCF_IOMEM]
#define		cf_msize		cf_loc[OBIOCF_IOSIZ]
#define		cf_irq			cf_loc[OBIOCF_IRQ]

void	*obio_intr_establish(int irq, int type, int level,
	    int (*ih_fun)(void *), void *ih_arg);
void	obio_intr_disestablish(void *handler);
void	obio_reserve_resource_map(void);
void	obio_reserve_resource_unmap(void);

#endif /* _PREP_OBIOVAR_H_ */
