/*
 * Copyright (c) 1993 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *	  This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission
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
 *
 *	$Id: boot_tftp.c,v 1.3 1994/01/28 23:48:12 jtc Exp $
 */

#include <sys/types.h>
#include <netinet/in_systm.h>

#include "salibc.h"

#include "netboot.h"
#include "netif.h"

void
boot_tftp(kernel_override, machdep_hint)
	char *kernel_override;
	void *machdep_hint;
{
	/*
	 * 0. get common ether addr if exists
	 * 1. choose interface
	 * 2. set up interface
	 * 3. bootp or bootparam
	 * 4. load kernel
	 * 5. run kernel
	 */
	struct iodesc desc;
	struct netif *nif;
	caddr_t startaddr;
	int loaded;

	loaded = 0;

	netif_init();
	while (!loaded) {
		bzero(&desc, sizeof(desc));
		machdep_common_ether(desc.myea);
		nif = netif_select(machdep_hint);
		if (!nif) 
			panic("netboot: tried all interfaces, got nowhere");

		if (netif_probe(nif, machdep_hint)) {
			printf("netboot: couldn't probe %s%d\n",
			    nif->nif_driver->netif_bname, nif->nif_unit);
			continue;
		}
		netif_attach(nif, &desc, machdep_hint);

		get_bootinfo(&desc);
/*		loaded = tftp_load(&desc, kernel_override, &startaddr);*/
		netif_detach(nif);
#ifdef notdef
		if (loaded)
			call((u_long) startaddr);
#endif
	}
}
