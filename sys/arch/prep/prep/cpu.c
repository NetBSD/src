/*	$NetBSD: cpu.c,v 1.2 2001/06/17 15:57:13 nonaka Exp $	*/

/*-
 * Copyright (C) 2000, 2001 NONAKA Kimihiro.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/preptype.h>

int cpumatch(struct device *, struct cfdata *, void *);
void cpuattach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return (0);

	return (1);
}

void
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{

	printf("\n");

	switch (prep_model) {
	case IBM_6050:
	case IBM_7248:
	case IBM_6040:
	    {
		u_char l2ctrl, cpuinf;

		/* system control register */
		l2ctrl = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x81c);
		/* device status register */
		cpuinf = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x80c);

		/* Enable L2 cache */
		*(volatile u_char *)(PREP_BUS_SPACE_IO + 0x81c) = l2ctrl | 0xc0;

		printf("%s: ", dev->dv_xname);
		if (((cpuinf>>1) & 1) == 0)
			printf("Upgrade CPU, ");

		printf("L2 cache ");
		if ((cpuinf & 1) == 0) {
			printf("%s ",
			    ((cpuinf>>2) & 1) ? "256KB" : "unknown size");
			printf("%s",
			    ((cpuinf>>3) & 1) ? "copy-back" : "write-through");
		} else
			printf("not present");

		printf("\n");
	    }
		break;

	default:
		break;
	}
}
