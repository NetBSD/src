/*	$NetBSD: ebus.c,v 1.2 2011/06/12 03:52:13 tsutsui Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: ebus.c,v 1.2 2011/06/12 03:52:13 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>

#include "locators.h"

void
ebusattach(device_t parent, device_t self, void *aux)
{
	struct ebus_dev_attach_args *ida = aux;
	struct ebus_attach_args *ia;
	void *addr;
	int i;
	int locs[EBUSCF_NLOCS];

	printf("\n");

	/*
	 * Loop through the devices and attach them.  If a probe-size
	 * is specified, it's an optional item on the platform and
	 * do a badaddr() test to make sure it's there.
	 */
	for (i = 0; i < ida->ida_ndevs; i++) {
		ia = &ida->ida_devs[i];

#if 0 // DEBUG
		printf("PROBING %s %d@%x i=%d\n",
		    ia->ia_name, ia->ia_basz, ia->ia_paddr, ia->ia_cookie);
#endif
		if (ia->ia_basz != 0) {
			addr = (void *)mips_map_physmem(ia->ia_paddr,
			    ia->ia_basz);
			if (addr == NULL) {
				printf("Failed to map %s: phys %x size %d\n",
				    ia->ia_name, ia->ia_paddr, ia->ia_basz);
				continue;
			}
			ia->ia_vaddr = addr;
#if 0 // DEBUG
			printf("MAPPED at %p\n", ia->ia_vaddr);
#endif
		}

		locs[EBUSCF_ADDR] = ia->ia_paddr;

		if (NULL == config_found_sm_loc(self, "ebus", locs, ia,
		    ebusprint, config_stdsubmatch)) {
			/* do we need to say anything? */
			if (ia->ia_basz != 0) {
				mips_unmap_physmem((vaddr_t)ia->ia_vaddr,
				    ia->ia_basz);
			}
		}
	}
}

int
ebusprint(void *aux, const char *pnp)
{
	struct ebus_attach_args *ia = aux;

	if (pnp)
		aprint_normal("%s at %s", ia->ia_name, pnp);

	aprint_normal(" addr 0x%x", ia->ia_paddr);

	return UNCONF;
}

void
ebus_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *, void *), void *arg)
{

	(*platform.intr_establish)(dev, cookie, level, handler, arg);
}
