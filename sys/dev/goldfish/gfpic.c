/*	$NetBSD: gfpic.c,v 1.2 2024/04/24 14:41:13 thorpej Exp $	*/

/*-     
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

/*
 * Support for the Goldfish virtual Programmable Interrupt Controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gfpic.c,v 1.2 2024/04/24 14:41:13 thorpej Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <dev/goldfish/gfpicvar.h>

/*
 * Goldfish PIC registers.
 */
#define	GFPIC_STATUS		(0 << 2) /* pending interrupt count */
#define	GFPIC_PENDING		(1 << 2) /* pending interrupt bitmask */
#define	GFPIC_DISABLE_ALL	(2 << 2) /* disables / clears all interrupts */
#define	GFPIC_DISABLE		(3 << 2) /* disable IRQs (bitmask) */
#define	GFPIC_ENABLE		(4 << 2) /* enable IRQs (bitmask) */

#define	REG_READ(sc, r)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (r))
#define	REG_WRITE(sc, r, v)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (r), (v))

/*
 * gfpic_attach --
 *	Attach a Goldfish virtual PIC.
 */
void
gfpic_attach(struct gfpic_softc *sc)
{
	aprint_naive("\n");
	aprint_normal(": Google Goldfish PIC\n");

	REG_WRITE(sc, GFPIC_DISABLE_ALL, 0);
}

/*
 * gfpic_enable --
 *	Enable the specified IRQ on a Goldfish virtual PIC.
 */
void
gfpic_enable(struct gfpic_softc *sc, int pirq)
{
	KASSERT(pirq >= 0);
	KASSERT(pirq <= 31);

	REG_WRITE(sc, GFPIC_ENABLE, (1U << pirq));
}

/*
 * gfpic_disable --
 *	Disable the specified IRQ on a Goldfish virtual PIC.
 */
void
gfpic_disable(struct gfpic_softc *sc, int pirq)
{
	KASSERT(pirq >= 0);
	KASSERT(pirq <= 31);

	REG_WRITE(sc, GFPIC_DISABLE, (1U << pirq));
}

/*
 * gfpic_pending --
 *	Return the next pending IRQ on a Goldfish virtual PIC,
 *	or -1 if there are none.
 */
int
gfpic_pending(struct gfpic_softc *sc)
{
	return ffs(REG_READ(sc, GFPIC_PENDING)) - 1;
}
