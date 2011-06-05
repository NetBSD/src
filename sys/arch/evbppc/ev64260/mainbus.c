/*	$NetBSD: mainbus.c,v 1.7 2011/06/05 17:03:18 matt Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.7 2011/06/05 17:03:18 matt Exp $");

#include "mainbus.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <evbppc/ev64260/ev64260.h>

#include "locators.h"

#if NCPU == 0
#error	A cpu device is now required
#endif


int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);
int	mainbus_cfprint(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
	mainbus_match, mainbus_attach, NULL, NULL);

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args mba;
	extern bus_addr_t gt_base;

	printf("\n");

	memset(&mba, 0, sizeof(mba));

	/*
	 * Always find the CPU
	 */
	mba.mba_name = "cpu";
	mba.mba_unit = 0;
	mba.mba_addr = MAINBUSCF_ADDR_DEFAULT;
	config_found(self, &mba, mainbus_cfprint);

#ifdef MULTIPROCESSOR
	/*
	 * Try for a second one...
	 */
	mba.mba_name = "cpu";
	mba.mba_unit = 1;
	mba.mba_addr = MAINBUSCF_ADDR_DEFAULT;
	config_found(self, &mba, mainbus_cfprint);
#endif

	/*
	 * Now try to configure the Discovery
	 */
	mba.mba_name = "gt";
	mba.mba_unit = -1;
	mba.mba_addr = gt_base;
	config_found(self, &mba, mainbus_cfprint);
}

int
mainbus_cfprint(void *aux, const char *pnp)
{
	struct mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_name, pnp);
	if (mba->mba_unit != -1)
		aprint_normal(" unit %d", mba->mba_unit);
	if (mba->mba_addr != MAINBUSCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%08x", mba->mba_addr);
	return UNCONF;
}


static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpu_match, cpu_attach, NULL, NULL);

int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *mba = aux;

	if (strcmp(mba->mba_name, "cpu") != 0)
		return 0;

	return 1;
}

void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *mba = aux;

	(void) cpu_attach_common(self, mba->mba_unit);
}
