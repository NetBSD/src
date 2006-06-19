/* $NetBSD: mkclock_pnpbus.c,v 1.1.2.2 2006/06/19 03:45:06 chap Exp $ */
/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

/*
 * Mostek MK48T18 time-of-day chip attachment to pnpbus, using two
 * 8-bit ports for address selection and one 8-bit port for data.
 *
 * Note that this is essentially a stub driver. We cannot attach the clock
 * with this driver because the clock and nvram part are one in the same, and
 * share the same IO ports.  If we attach the clock, the NVRAM driver will
 * fail to attach.  Instead, we note that we have an mk48txx device, and in
 * the nvram driver, we will attach the clock goop there.
 *
 * Therefore the probe will allways fail.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mkclock_pnpbus.c,v 1.1.2.2 2006/06/19 03:45:06 chap Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/residual.h>

#include <dev/isa/isavar.h>
#include <prep/pnpbus/pnpbusvar.h>

extern int prep_clock_mk48txx;

static int	mkclock_pnpbus_probe(struct device *, struct cfdata *, void *);

CFATTACH_DECL(mkclock_pnpbus, 0, mkclock_pnpbus_probe, NULL, NULL, NULL);

static int
mkclock_pnpbus_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;

	if (strcmp(pna->pna_devid, "PNP0B00") == 0 &&
	    pna->subtype == RealTimeClock && pna->interface == 129)
		prep_clock_mk48txx = 1;

	return 0;
}
