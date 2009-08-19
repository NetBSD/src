/*	$NetBSD: mainbus.c,v 1.1.2.2 2009/08/19 18:45:56 yamt Exp $	*/

/*-
 * Copyright (c) 2008,2009 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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
/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1.2.2 2009/08/19 18:45:56 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/device.h>

#if 0
#include "mainbus.h"

#if NCPU == 0
#error	A cpu device is now required
#endif
#endif

void mbattach(struct device *, struct device *, void *);
int mbprint(void *, const char *);
int mbmatch(struct device *, struct cfdata *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mbmatch, mbattach, NULL, NULL);

static int mainbus_print (void *, const char *);


int
mbmatch(struct device *parent, struct cfdata *cfp, void *aux)
{

	return (1);
}

void
mbattach(struct device *parent, struct device *self, void *aux)
{

	printf("\n");

	/*
	 * Always find the CPU
	 */
	config_found_ia(self, "mainbus", __UNCONST("cpu"), mainbus_print);

	/*
	 * "find" all the things that should be there.
	 */
	if (is_a3000() || is_a4000())
		config_found(self, __UNCONST("a34kbbc"), simple_devprint);
	else
		config_found(self, __UNCONST("a2kbbc"), simple_devprint);
	config_found(self, __UNCONST("ser"), simple_devprint);
	config_found(self, __UNCONST("par"), simple_devprint);
	config_found(self, __UNCONST("kbd"), simple_devprint);
	config_found(self, __UNCONST("ms"), simple_devprint);
	config_found(self, __UNCONST("grfcc"), simple_devprint);
	config_found(self, __UNCONST("amidisplaycc"), simple_devprint);
	config_found(self, __UNCONST("fdc"), simple_devprint);
	if (is_a4000() || is_a1200()) {
		config_found(self, __UNCONST("wdc"), simple_devprint);
		config_found(self, __UNCONST("idesc"), simple_devprint);
	}
	if (is_a4000())			/* Try to configure A4000T SCSI */
		config_found(self, __UNCONST("afsc"), simple_devprint);
	if (is_a3000())
		config_found(self, __UNCONST("ahsc"), simple_devprint);
	if (is_a1200())
		config_found(self, __UNCONST("pccard"), simple_devprint);
	config_found(self, __UNCONST("aucc"), simple_devprint);

	config_found(self, __UNCONST("zbus"), simple_devprint);
}

int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("%s at %s", (char *)aux, pnp);
	return (UNCONF);
}

static int
mainbus_print(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("cpu0 at %s", pnp);
	return (UNCONF);
}
