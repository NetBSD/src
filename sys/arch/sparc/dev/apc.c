/*	$NetBSD: apc.c,v 1.1.2.2 2010/01/27 21:17:55 sborrill Exp $	*/

/*
 * Copyright (c) 2010 Manuel Bouyer.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apc.c,v 1.1.2.2 2010/01/27 21:17:55 sborrill Exp $");


/*
 * driver for the power management functions of the Aurora Personality Chip
 * on SPARCstation-4/5 and derivatives
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/autoconf.h>

#include <sparc/dev/apcreg.h>

static int apcmatch(device_t, struct cfdata *, void *);
static void apcattach(device_t, device_t, void *);
static void apc_cpu_sleep(struct cpu_info *);

struct apc_softc {
	device_t	sc_dev;
	bus_space_tag_t	sc_bt;
	bus_space_handle_t sc_bh;
};

struct apc_softc *apc = NULL;

CFATTACH_DECL_NEW(apc, sizeof(struct apc_softc),
    apcmatch, apcattach, NULL, NULL);


static int
apcmatch(device_t parent, struct cfdata *cf, void *aux)
{
	struct sbus_attach_args *sa = aux;
	if (apc != NULL) /* only one instance */
		return 0;
	return strcmp("power-management", sa->sa_name) == 0;
}

static void
apcattach(device_t parent, device_t self, void *aux)
{
	struct sbus_attach_args *sa = aux;
	struct apc_softc *sc = device_private(self);

	sc->sc_bt = sa->sa_bustag;
	if (sbus_bus_map(sa->sa_bustag,
	    sa->sa_slot, sa->sa_offset, APC_REG_SIZE, 0, &sc->sc_bh) != 0) {
		aprint_error(": cannot map registers\n");
		return;
	}
	aprint_normal("\n");
	apc = sc;
	curcpu()->idlespin = apc_cpu_sleep;
}

static void
apc_cpu_sleep(struct cpu_info *ci)
{
	uint8_t val;
	if (apc == NULL)
		return;
	val = bus_space_read_1(apc->sc_bt, apc->sc_bh, APC_IDLE_REG);
	bus_space_write_1(apc->sc_bt, apc->sc_bh, APC_IDLE_REG,
	    val | APC_IDLE_REG_IDLE);
}
