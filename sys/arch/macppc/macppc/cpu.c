/*	$NetBSD: cpu.c,v 1.3 1999/02/16 15:20:51 tsubai Exp $	*/

/*-
 * Copyright (C) 1998	Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>

static int cpumatch __P((struct device *, struct cfdata *, void *));
static void cpuattach __P((struct device *, struct device *, void *));

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

extern void *mapiodev();

int
cpumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name))
		return 0;

	return 1;
}

void
cpuattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	u_int x;
	u_int *cache_reg;
	u_int node;
	int addr = -1;

	node = OF_finddevice("/hammerhead");
	OF_getprop(node, "reg", &addr, sizeof(addr));

	if (addr == -1) {
		printf("\n");
		return;
	}

#if 0
	/* enable L2 cache */
	cache_reg = mapiodev(addr, NBPG);
	if (((cache_reg[2] >> 24) & 0x0f) >= 3) {
		x = cache_reg[4];
		if ((x & 0x10) == 0)
                	x |= 0x04000000;
		else
                	x |= 0x04000020;

		cache_reg[4] = x;
		printf(": L2 cache enabled");
	}
#endif

	printf("\n");
}
