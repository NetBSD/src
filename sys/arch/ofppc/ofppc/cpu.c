/*	$NetBSD: cpu.c,v 1.7.38.1 2007/06/21 18:49:46 garbled Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro; by Jason R. Thorpe.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.7.38.1 2007/06/21 18:49:46 garbled Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/pio.h>

static int cpu_match(struct device *, struct cfdata *, void *);
static void cpu_attach(struct device *, struct device *, void *);
void cpu_OFgetspeed(struct device *, struct cpu_info *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

#define HH_ARBCONF	0xf8000090

int
cpu_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct confargs *ca = aux;
	int *reg = ca->ca_reg;
	int node;

	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return 0;

	node = OF_finddevice("/cpus");
	if (node != -1) {
		for (node = OF_child(node); node != 0; node = OF_peer(node)) {
			uint32_t cpunum;
			int l;

			l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
			if (l == 4 && reg[0] == cpunum)
				return 1;
		}
	}
	switch (reg[0]) {
	case 0: /* primary CPU */
		return 1;
	case 1: /* secondary CPU */
		if (OF_finddevice("/hammerhead") != -1)
			if (in32rb(HH_ARBCONF) & 0x02)
				return 1;
		break;
	}
	return 0;
}

void
cpu_OFgetspeed(struct device *self, struct cpu_info *ci)
{
	int node;
	node = OF_finddevice("/cpus");
	if (node != -1) {
		for (node = OF_child(node); node; node = OF_peer(node)) {
			uint32_t cpunum;
			int l;

			l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
			if (l == 4 && ci->ci_cpuid == cpunum) {
				uint32_t cf;

				l = OF_getprop(node, "clock-frequency",
				    &cf, sizeof(cf));
				if (l == 4)
					ci->ci_khz = cf / 1000;
				break;
			}
		}
	}
	if (ci->ci_khz)
		aprint_normal("%s: %u.%02u MHz\n", self->dv_xname,
		    ci->ci_khz / 1000, (ci->ci_khz / 10) % 100);
}

void
cpu_attach(struct device *parent, struct device *self, void *aux)
{
	struct cpu_info *ci;
	struct confargs *ca = aux;
	int id = ca->ca_reg[0];

	ci = cpu_attach_common(self, id);
	if (ci == NULL)
		return;

	if (ci->ci_khz == 0)
		cpu_OFgetspeed(self, ci);

	if (id > 0)
		return;
}
