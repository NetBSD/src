/*	$NetBSD: stub_ebus.c,v 1.3.2.1 2014/08/10 06:53:54 tls Exp $	*/

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
	struct STUBSTRUCT *sc_dp;
};

static int	stub_ebus_match (device_t, cfdata_t, void *);
static void	stub_ebus_attach (device_t, device_t, void *);

CFATTACH_DECL_NEW(stub_ebus, sizeof (struct stub_softc),
    stub_ebus_match, stub_ebus_attach, NULL, NULL);

static int
stub_ebus_match(device_t parent, cfdata_t match, void *aux)
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
stub_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct ebus_attach_args *ia =aux;
	struct stub_softc *sc = device_private(self);

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
	.d_open = stubopen,
	.d_close = stubclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
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
