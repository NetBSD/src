/*	$NetBSD: power.c,v 1.12 2002/03/11 16:27:02 pk Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Harvard University
 *	and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

#include <sparc/dev/power.h>

static int powermatch __P((struct device *, struct cfdata *, void *));
static void powerattach __P((struct device *, struct device *, void *));

struct cfattach power_ca = {
	sizeof(struct device), powermatch, powerattach
};

/*
 * This is the driver for the "power" register available on some Sun4m
 * machines. This allows the machine to remove power automatically when
 * shutdown or halted or whatever.
 */

static int
powermatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	if (uoba->uoba_isobio4 != 0)
		return (0);

	return (strcmp("power", sa->sa_name) == 0);
}

/* ARGSUSED */
static void
powerattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	bus_space_handle_t bh;

	/* Map the power configuration register. */
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot, sa->sa_offset, sizeof(u_int8_t),
			 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		printf("%s: cannot map register\n", self->dv_xname);
		return;
	}
	power_reg = (volatile u_int8_t *)bh;

	printf("\n");
}

void
powerdown()
{
	/* Only try if the power node was attached. */
	if (power_reg != NULL)
		*POWER_REG |= POWER_OFF;

	/*
	 * don't return too quickly; the PROMs on some sparcs
	 * report the powerdown as failed if we do.
	 */
	DELAY(1000000);
}
