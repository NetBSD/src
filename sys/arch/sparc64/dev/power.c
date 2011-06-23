/*	$NetBSD: power.c,v 1.11.10.1 2011/06/23 14:19:42 cherry Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: power.c,v 1.11.10.1 2011/06/23 14:19:42 cherry Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

#include <sparc64/dev/power.h>

static int powermatch(device_t, cfdata_t, void *);
static void powerattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(power, 0,
    powermatch, powerattach, NULL, NULL);

extern struct cfdriver power_cd;

/*
 * This is the driver for the "power" register available on some Sun4m
 * machines. This allows the machine to remove power automatically when
 * shutdown or halted or whatever.
 *
 * XXX: this capability is not used in the current kernel.
 */

static int
powermatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (CPU_ISSUN4M)
		return strcmp("power", ca->ca_ra.ra_name) == 0;

	return 0;
}

/* ARGSUSED */
static void
powerattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct romaux *ra = &ca->ca_ra;

	power_reg = mapiodev(ra->ra_reg, 0, sizeof(long));

	printf("\n");
}

void
powerdown(void)
{
	*POWER_REG |= POWER_OFF;
}
