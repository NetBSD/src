/*	$NetBSD: booke_autoconf.c,v 1.2.6.1 2011/06/23 14:19:27 cherry Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: booke_autoconf.c,v 1.2.6.1 2011/06/23 14:19:27 cherry Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <powerpc/booke/cpuvar.h>

void
e500_device_register(device_t dev, void *aux)
{
	device_t parent = device_parent(dev);

	if (device_is_a(dev, "etsec") && device_is_a(parent, "cpunode")) {
		/* Set the mac-addr of the on-chip Ethernet. */
		struct cpunode_attach_args *cna = aux;

		if (cna->cna_locs.cnl_instance < 4) {
			prop_data_t pd;
			char prop_name[15];

			snprintf(prop_name, sizeof(prop_name),
			    "etsec%d-mac-addr", cna->cna_locs.cnl_instance);

			pd = prop_dictionary_get(board_properties, prop_name);
			if (pd == NULL) {
				printf("WARNING: unable to get mac-addr "
				    "property from board properties\n");
				return;
			}
			if (prop_dictionary_set(device_properties(dev),
						"mac-address", pd) == false) {
				printf("WARNING: unable to set mac-addr "
				    "property for %s\n", device_xname(dev));
			}
		}
		return;
	}
}
