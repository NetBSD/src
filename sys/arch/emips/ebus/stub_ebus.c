/*	$NetBSD: stub_ebus.c,v 1.1.8.2 2011/06/06 09:05:16 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

/* Before including this file for "foo" you need to define
#define STUBSTRING       "foo"
#define STUBBANNER       "8MB flash memory"  -- tell the user what it is
#define STUBSTRUCT       _Flash   -- whatever that struct is defined in emipsreg.h
#define STUBMATCH(_f_)   (((_f_)->BaseAddressAndTag & FLASHBT_TAG) == PMTTAG_FLASH)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

/*
 * Device softc
 */
struct stub_softc {
	struct device sc_dev;
	struct STUBSTRUCT *sc_dp;
};

static int	stub_ebus_match (struct device *, struct cfdata *, void *);
static void	stub_ebus_attach (struct device *, struct device *, void *);

CFATTACH_DECL(stub_ebus, sizeof (struct stub_softc),
    stub_ebus_match, stub_ebus_attach, NULL, NULL);

static int
stub_ebus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ebus_attach_args *ia = aux;
	struct STUBSTRUCT *f = (struct STUBSTRUCT *)ia->ia_vaddr;

	if (strcmp(STUBSTRING, ia->ia_name) != 0)
		return (0);
	if ((f == NULL) || (! STUBMATCH(f)))
		return (0);

	return (1);
}

static void
stub_ebus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ebus_attach_args *ia =aux;
	struct stub_softc *sc = (struct stub_softc *)self;

	sc->sc_dp = (struct STUBSTRUCT*)ia->ia_vaddr;

#if DEBUG
	printf(" virt=%p", (void*)sc->sc_dp);
#endif
	printf(": %s\n", STUBBANNER);

}

/* Required funcs
 */
static int     stubopen(dev_t device, int flags, int fmt, struct lwp *process);
static int     stubclose(dev_t device, int flags, int fmt, struct lwp *process);

/* just define the character device handlers because that is all we need */
const struct cdevsw stub_cdevsw = {
	stubopen,
	stubclose,
	noread,
	nowrite,
	noioctl,
	nostop,
	notty,
	nopoll,
	nommap,
	nokqfilter,
};

/*
 * Handle an open request on the device.
 */
static int
stubopen(dev_t device, int flags, int fmt, struct lwp *process)
{
	return 0; /* this always succeeds */
}

/*
 * Handle the close request for the device.
 */
static int
stubclose(dev_t device, int flags, int fmt, struct lwp *process)
{
	return 0; /* again this always succeeds */
}
