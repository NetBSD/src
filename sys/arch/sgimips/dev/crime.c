/*	$NetBSD: crime.c,v 1.1 2000/06/14 16:13:54 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

/*
 * O2 CRIME
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <sgimips/dev/crimereg.h>

#include "locators.h"

static int	crime_match(struct device *, struct cfdata *, void *);
static void	crime_attach(struct device *, struct device *, void *);

struct cfattach crime_ca = {
	sizeof(struct device), crime_match, crime_attach
};

static int
crime_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/*
	 * The CRIME is in the O2.
	 */
	switch (ma->ma_arch) {
	case 32:
		return 1;
	default:
		return 0;
	}
}

static void
crime_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	u_int32_t rev; /* really u_int64_t ! */
	int major, minor;

	rev = bus_space_read_4(ma->ma_iot, ma->ma_ioh, 4) & 0xff;

	major = rev > 4;
	minor = rev & 0x0f;

	if (major == 0 && minor == 0)
		printf(": petty\n");
	else
		printf(": rev %d.%d\n", major, minor);

#if 0
	*(volatile u_int64_t *)0xb4000018 = 0xffffffffffffffff;
#else
	/* enable all mace interrupts, but no crime devices */
	*(volatile u_int64_t *)0xb4000018 = 0x000000000000ffff;
#endif
}
