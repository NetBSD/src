/*	$NetBSD: ioblix_zbus.c,v 1.3.8.2 2002/02/28 04:06:48 nathanw Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ioblix_zbus.c,v 1.3.8.2 2002/02/28 04:06:48 nathanw Exp $");

/* IOBlix Zorro driver */
/* XXX to be done: we need to probe the com clock speed! */

#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>

#include <machine/bus.h>
#include <machine/conf.h>

#include <amiga/include/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/drcustom.h>

#include <amiga/dev/supio.h>
#include <amiga/dev/zbusvar.h>


struct iobz_softc {
	struct device sc_dev;
	struct bus_space_tag sc_bst;
};

int iobzmatch(struct device *, struct cfdata *, void *);
void iobzattach(struct device *, struct device *, void *);
int iobzprint(void *auxp, const char *);
void iobz_shutdown(void *);

struct cfattach iobl_zbus_ca = {
	sizeof(struct iobz_softc), iobzmatch, iobzattach
};

int
iobzmatch(struct device *parent, struct cfdata *cfp, void *auxp)
{

	struct zbus_args *zap;

	zap = auxp;

	if (zap->manid != 4711)
		return (0);

	if (zap->prodid != 1)
		return (0);

	return (1);
}

struct iobz_devs {
	char *name;
	unsigned off;
	int arg;
} iobzdevices[] = {
	{ "com", 0x100, 24000000 },	/* XXX see below */
	{ "com", 0x108, 24000000 },
	{ "com", 0x110, 24000000 },
	{ "com", 0x118, 24000000 },
	{ "lpt", 0x200, 0 },
	{ "lpt", 0x300, 0 },
	{ 0, 0, 0}
};

#ifndef IOBZCLOCK
#define IOBZCLOCK 22118400;
#endif
int iobzclock = IOBZCLOCK;		/* patchable! */

void
iobzattach(struct device *parent, struct device *self, void *auxp)
{
	struct iobz_softc *iobzsc;
	struct iobz_devs  *iobzd;
	struct zbus_args *zap;
	struct supio_attach_args supa;
	extern const struct amiga_bus_space_methods amiga_bus_stride_16;
	volatile u_int8_t *p;


	iobzsc = (struct iobz_softc *)self;
	zap = auxp;

	if (parent)
		printf("\n");

	iobzsc->sc_bst.base = (u_long)zap->va;
	iobzsc->sc_bst.absm = &amiga_bus_stride_16;

	supa.supio_iot = &iobzsc->sc_bst;
	supa.supio_ipl = 6;

	iobzd = iobzdevices;

	while (iobzd->name) {
		supa.supio_name = iobzd->name;
		supa.supio_iobase = iobzd->off;
		supa.supio_arg = iobzclock /* XXX iobzd->arg */;
		config_found(self, &supa, iobzprint); /* XXX */
		++iobzd;
	}

	p = (volatile u_int8_t *)zap->va + 2;
	(void)shutdownhook_establish(iobz_shutdown, (void *)p);
	*p = ((*p) & 0x1F) | 0x80;
}

int
iobzprint(void *auxp, const char *pnp)
{
	struct supio_attach_args *supa;
	supa = auxp;

	if (pnp == NULL)
		return(QUIET);

	printf("%s at %s port 0x%02x",
	    supa->supio_name, pnp, supa->supio_iobase);

	return(UNCONF);
}

/*
 * Disable board interupts at shutdown time.
 */

void
iobz_shutdown(void *p) {
	volatile int8_t *q;

	q = p;

	*q &= 0x1F;
}
