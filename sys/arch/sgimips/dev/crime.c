/*	$NetBSD: crime.c,v 1.12 2003/07/15 03:35:51 lukem Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * O2 CRIME
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: crime.c,v 1.12 2003/07/15 03:35:51 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/machtype.h>

#include <dev/pci/pcivar.h>

#include <sgimips/dev/macereg.h>
#include <sgimips/dev/crimereg.h>

#include "locators.h"

static int	crime_match(struct device *, struct cfdata *, void *);
static void	crime_attach(struct device *, struct device *, void *);
void *		crime_intr_establish(int, int, int, int (*)(void *), void *);
void		crime_intr(u_int);

CFATTACH_DECL(crime, sizeof(struct device),
    crime_match, crime_attach, NULL, NULL);

#define CRIME_NINTR 32 	/* XXX */

struct {
	int	(*func)(void *);
	void	*arg;
} crime[CRIME_NINTR];

static int
crime_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	/*
	 * The CRIME is in the O2.
	 */
	if (mach_type == MACH_SGI_IP32)
		return (1);

	return (0);
}

static void
crime_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	u_int64_t crm_id;

	crm_id = bus_space_read_8(ma->ma_iot, ma->ma_ioh, 0);

	aprint_naive(": system ASIC");

	switch ((crm_id & CRIME_ID_IDBITS) >> CRIME_ID_IDSHIFT) {
	case 0x0b:
		aprint_normal(": rev 1.5");
		break;

	case 0x0a:
		if ((crm_id >> 32) == 0)
			aprint_normal(": rev 1.1");
		else if ((crm_id >> 32) == 1)
			aprint_normal(": rev 1.3");
		else
			aprint_normal(": rev 1.4");
		break;

	case 0x00:
		aprint_normal(": Petty CRIME");
		break;

	default:
		aprint_normal(": Unknown CRIME");
		break;
	}

	aprint_normal(" (CRIME_ID: %llx)\n", crm_id);

	/* All interrupts off.  Turned on as we register devices */
	*(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_INTMASK) = 0;
	*(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_INTSTAT) = 0;
	*(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_SOFTINT) = 0;
	*(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_HARDINT) = 0;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_ISA_INT_STATUS) = 0;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_ISA_INT_MASK) = 0;
}

/*
 * XXX: sharing interrupts?
 */

void *
crime_intr_establish(irq, type, level, func, arg)
	int irq;
	int type;
	int level;
	int (*func)(void *);
	void *arg;
{
	if (crime[irq].func != NULL)
		return NULL;	/* panic("Cannot share CRIME interrupts!"); */

	crime[irq].func = func;
	crime[irq].arg = arg;

	return (void *)&crime[irq];
}

void
crime_intr(pendmask)
	u_int pendmask;
{
	int i;

	for (i = 0; i < CRIME_NINTR; i++) {
		if ((pendmask & (1 << i)) && crime[i].func != NULL)
			(*crime[i].func)(crime[i].arg);
	}
}

