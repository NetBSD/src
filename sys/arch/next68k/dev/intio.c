/*	$NetBSD: intio.c,v 1.10 2005/06/30 17:03:53 drochner Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Autoconfiguration support for next68k internal i/o space.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intio.c,v 1.10 2005/06/30 17:03:53 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h> 
#include <sys/reboot.h>

#include <machine/autoconf.h>

#include <next68k/dev/intiovar.h>

int	intiomatch(struct device *, struct cfdata *, void *);
void	intioattach(struct device *, struct device *, void *);
int	intioprint(void *, const char *);
int	intiosearch(struct device *, struct cfdata *,
		    const locdesc_t *, void *);

CFATTACH_DECL(intio, sizeof(struct device),
    intiomatch, intioattach, NULL, NULL);

#if 0
struct cfdriver intio_cd = {
	NULL, "intio", DV_DULL
};
#endif

static int intio_attached = 0;

int
intiomatch(struct device *parent, struct cfdata *match, void *aux)
{
	/* Allow only one instance. */
	if (intio_attached)
		return (0);

	return (1);
}

void
intioattach(struct device *parent, struct device *self, void *aux)
{

	printf("\n");

	/* Search for and attach children. */
	config_search_ia(intiosearch, self, "intio", aux);

	intio_attached = 1;
}

int
intioprint(void *aux, const char *pnp)
{
	struct intio_attach_args *ia = aux;

	if (ia->ia_addr)
		aprint_normal(" addr %p", ia->ia_addr);

	return (UNCONF);
}

int
intiosearch(struct device *parent, struct cfdata *cf,
	    const locdesc_t *ldesc, void *aux)
{
	struct mainbus_attach_args *mba = (struct mainbus_attach_args *) aux;
	struct intio_attach_args ia;

	do {
		ia.ia_addr = NULL;
		ia.ia_bst = NEXT68K_INTIO_BUS_SPACE;
		ia.ia_dmat = mba->mba_dmat;
		
		if (config_match(parent, cf, &ia) == 0)
			break;
		config_attach(parent, cf, &ia, intioprint);
	} while (cf->cf_fstate == FSTATE_STAR);

	return (0);
}
