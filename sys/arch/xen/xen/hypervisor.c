/* $NetBSD: hypervisor.c,v 1.5 2004/04/25 00:24:08 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: hypervisor.c,v 1.5 2004/04/25 00:24:08 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include "xencons.h"
#include "xennet.h"
#include "xbd.h"
#include "xenkbc.h"
#include "vga_xen.h"
#include "npx.h"

#include "opt_xen.h"

#include <machine/xen.h>
#include <machine/hypervisor.h>

#if NXENNET > 0
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <machine/if_xennetvar.h>
#endif

#if NXBD > 0
#include <sys/buf.h>
#include <sys/disk.h>
#include <dev/dkvar.h>
#include <machine/xbdvar.h>
#endif

#if NXENKBC > 0
#include <dev/pckbport/pckbportvar.h>
#include <machine/xenkbcvar.h>
#endif

#if NVGA_XEN > 0
#include <machine/bus.h>
#include <machine/vga_xenvar.h>
#endif

int	hypervisor_match(struct device *, struct cfdata *, void *);
void	hypervisor_attach(struct device *, struct device *, void *);

CFATTACH_DECL(hypervisor, sizeof(struct device),
    hypervisor_match, hypervisor_attach, NULL, NULL);

int	hypervisor_print(void *, const char *);

union hypervisor_attach_cookie {
	const char *hac_device;		/* first elem of all */
#if NXENKBC > 0
	struct xenkbc_attach_args hac_xenkbc;
#endif
#if NVGA_XEN > 0
	struct xen_vga_attach_args hac_vga_xen;
#endif
#if NXENCONS > 0
	struct xencons_attach_args hac_xencons;
#endif
#if NXENNET > 0
	struct xennet_attach_args hac_xennet;
#endif
#if NXBD > 0
	struct xbd_attach_args hac_xbd;
#endif
#if NNPX > 0
	struct xen_npx_attach_args hac_xennpx;
#endif
};


/*
 * Probe for the hypervisor; always succeeds.
 */
int
hypervisor_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct hypervisor_attach_args *haa = aux;

	if (strcmp(haa->haa_busname, "hypervisor") == 0)
		return 1;
	return 0;
}

/*
 * Attach the hypervisor.
 */
void
hypervisor_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union hypervisor_attach_cookie hac;

	printf("\n");

#if NXENKBC > 0
	hac.hac_xenkbc.xa_device = "xenkbc";
	config_found(self, &hac.hac_xenkbc, hypervisor_print);
#endif

#if NVGA_XEN > 0
	hac.hac_vga_xen.xa_device = "vga_xen";
	hac.hac_vga_xen.xa_iot = X86_BUS_SPACE_IO;
	hac.hac_vga_xen.xa_memt = X86_BUS_SPACE_MEM;
	config_found(self, &hac.hac_vga_xen, hypervisor_print);
#endif

#if NXENCONS > 0
	hac.hac_xencons.xa_device = "xencons";
	config_found(self, &hac.hac_xencons, hypervisor_print);
#endif
#if NXENNET > 0
	hac.hac_xennet.xa_device = "xennet";
	xennet_scan(self, &hac.hac_xennet, hypervisor_print);
#endif
#if NXBD > 0
	hac.hac_xbd.xa_device = "xbd";
	xbd_scan(self, &hac.hac_xbd, hypervisor_print);
#endif
#if NNPX > 0
	hac.hac_xennpx.xa_device = "npx";
	config_found(self, &hac.hac_xennpx, hypervisor_print);
#endif
}

int
hypervisor_print(aux, parent)
	void *aux;
	const char *parent;
{
	union hypervisor_attach_cookie *hac = aux;

	if (parent)
		aprint_normal("%s at %s", hac->hac_device, parent);
	return (UNCONF);
}
