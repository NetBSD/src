/*	$NetBSD: ibm4xx_autoconf.c,v 1.8.6.2 2006/05/24 15:48:19 tron Exp $	*/
/*	Original Tag: ibm4xxgpx_autoconf.c,v 1.2 2004/10/23 17:12:22 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibm4xx_autoconf.c,v 1.8.6.2 2006/05/24 15:48:19 tron Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/cpu.h>

#include <powerpc/ibm4xx/dev/opbvar.h>

void
ibm4xx_device_register(struct device *dev, void *aux)
{
	struct device *parent = device_parent(dev);

	if (device_is_a(dev, "emac") && device_is_a(parent, "opb")) {
		/* Set the mac-addr of the on-chip Ethernet. */
		struct opb_attach_args *oaa = aux;

		if (oaa->opb_instance < 10) {
			prop_data_t pd;
			unsigned char prop_name[15];

			snprintf(prop_name, sizeof(prop_name),
				"emac%d-mac-addr", oaa->opb_instance);

			pd = prop_dictionary_get(board_properties, prop_name);
			if (pd == NULL) {
				printf("WARNING: unable to get mac-addr "
				    "property from board properties\n");
				return;
			}
			if (prop_dictionary_set(device_properties(dev),
						"mac-addr", pd) == FALSE) {
				printf("WARNING: unable to set mac-addr "
				    "property for %s\n", dev->dv_xname);
			}
		}
		return;
	}
}
