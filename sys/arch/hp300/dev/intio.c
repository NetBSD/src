/*	$NetBSD: intio.c,v 1.9 2002/03/17 05:44:49 gmcgarry Exp $	*/

/*-
 * Copyright (c) 1996, 1998, 2001 The NetBSD Foundation, Inc.
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
 * Autoconfiguration support for hp300 internal i/o space.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intio.c,v 1.9 2002/03/17 05:44:49 gmcgarry Exp $");                                                  

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h> 

#include <machine/hp300spu.h>

#include <hp300/dev/intioreg.h> 
#include <hp300/dev/intiovar.h>

int	intiomatch(struct device *, struct cfdata *, void *);
void	intioattach(struct device *, struct device *, void *);
int	intioprint(void *, const char *);

const struct cfattach intio_ca = {
	sizeof(struct device), intiomatch, intioattach
};

#if defined(HP320) || defined(HP330) || defined(HP340) || defined(HP345) || \
    defined(HP350) || defined(HP360) || defined(HP370) || defined(HP375) || \
    defined(HP380) || defined(HP385)
const struct intio_builtins intio_3xx_builtins[] = {
	{ "rtc",	0x020000,	-1},
	{ "hil",	0x028000,	1},
	{ "fb",		0x160000,	-1},
};
#define nintio_3xx_builtins \
	(sizeof(intio_3xx_builtins) / sizeof(intio_3xx_builtins[0]))
#endif

#if defined(HP400) || defined(HP425) || defined(HP433)
const struct intio_builtins intio_4xx_builtins[] = {
	{ "rtc",	0x020000,	-1},
	{ "frodo",	0x01c000,	5},
	{ "hil",	0x028000,	1},
	{ "fb",		0x160000,	-1},
};
#define nintio_4xx_builtins \
	(sizeof(intio_4xx_builtins) / sizeof(intio_4xx_builtins[0]))
#endif

static int intio_matched = 0;

int
intiomatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	/* Allow only one instance. */
	if (intio_matched)
		return (0);

	intio_matched = 1;
	return (1);
}

void
intioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct intio_attach_args ia;
	const struct intio_builtins *ib;
	int ndevs;
	int i;

	printf("\n");

	switch (machineid) {
#if defined(HP320) || defined(HP330) || defined(HP340) || defined(HP345) || \
    defined(HP350) || defined(HP360) || defined(HP370) || defined(HP375) || \
    defined(HP380) || defined(HP385)
	case HP_320:
	case HP_330:
	case HP_340:
	case HP_345:
	case HP_350:
	case HP_360:
	case HP_370:
	case HP_375:
	case HP_380:
	case HP_385:
		ib = intio_3xx_builtins;
		ndevs = nintio_3xx_builtins;
		break;
#endif
#if defined(HP400) || defined(HP425) || defined(HP433)
	case HP_400:
	case HP_425:
	case HP_433:
		ib = intio_4xx_builtins;
		ndevs = nintio_4xx_builtins;
		break;
#endif
	default:
		return;
	}

	memset(&ia, 0, sizeof(ia));

	for (i=0; i<ndevs; i++) {
		strncpy(ia.ia_modname, ib[i].ib_modname, INTIO_MOD_LEN);
		ia.ia_modname[INTIO_MOD_LEN] = '\0';
		ia.ia_bst = HP300_BUS_SPACE_INTIO;
		ia.ia_iobase = ib[i].ib_offset;
		ia.ia_addr = (bus_addr_t)(intiobase + ib[i].ib_offset);
		ia.ia_ipl = ib[i].ib_ipl;
		config_found(self, &ia, intioprint);
	}
}

int
intioprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct intio_attach_args *ia = aux;

	if (pnp != NULL)
		printf("%s at %s", ia->ia_modname, pnp);
	if (ia->ia_iobase != 0) {
		printf(" addr 0x%lx", (u_long)intiobase+ia->ia_iobase);
		if (ia->ia_ipl != -1 && pnp != NULL)
			printf(" ipl %d", ia->ia_ipl);
	}
	return (UNCONF);
}
