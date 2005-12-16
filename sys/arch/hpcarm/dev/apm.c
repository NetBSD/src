/*	$NetBSD: apm.c,v 1.9 2005/12/16 04:02:14 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#include "apm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apm.c,v 1.9 2005/12/16 04:02:14 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/conf.h>

#include <machine/apmvar.h>

extern struct cfdriver apm_cd;
extern int hpcarm_apm_getpower(struct apm_power_info *, void *);

int apmmatch(struct device *, struct cfdata *, void *);
void apmattach(struct device *, struct device *, void *);

static dev_type_open(apmopen);
static dev_type_close(apmclose);
static dev_type_ioctl(apmioctl);

struct apm_softc {
	struct device sc_dev;
	void *sc_parent;
};

CFATTACH_DECL(apm, sizeof(struct apm_softc),
    apmmatch, apmattach, NULL, NULL);

const struct cdevsw apm_cdevsw = {
	apmopen, apmclose, noread, nowrite, apmioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

int
apmmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct apm_attach_args *apm_args = (struct apm_attach_args *)aux;

	return (apm_args->aaa_magic == APM_ATTACH_ARGS_MAGIC);
}

void
apmattach(struct device *parent, struct device *self, void *aux)
{
	struct apm_softc *sc = (struct apm_softc *)self;

	sc->sc_parent = (void *)parent;

	printf("\n");
}

static int
apmopen(dev_t dev, int flag, int mode, struct lwp *l)
{

	return 0;
}

static int
apmclose(dev_t dev, int flag, int mode, struct lwp *l)
{

	return 0;
}

static int
apmioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct apm_softc *sc;

	if ((apm_cd.cd_ndevs == 0) || (minor(dev) & 0xf0) || 
	    ((sc = apm_cd.cd_devs[minor(dev) & 0xf0]) == NULL))
		return ENXIO;

	switch (cmd) {
	case APM_IOC_GETPOWER:
		return hpcarm_apm_getpower((struct apm_power_info *)data, 
		    sc->sc_parent);
		/* NOTREACHED */
	default:
		return EINVAL;
	}

	return 0;
}
