/*	$NetBSD: cpu.c,v 1.9 2008/02/07 19:48:37 garbled Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.9 2008/02/07 19:48:37 garbled Exp $");

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
			if (l == sizeof(uint32_t) && ci->ci_cpuid == cpunum) {
				uint32_t cf;

				l = OF_getprop(node, "clock-frequency",
				    &cf, sizeof(cf));
				if (l == sizeof(uint32_t))
					ci->ci_khz = cf / 1000;
				break;
			}
		}
	}
	if (ci->ci_khz)
		aprint_normal_dev(self, "%u.%02u MHz\n",
		    ci->ci_khz / 1000, (ci->ci_khz / 10) % 100);
}

static void
cpu_print_cache_config(uint32_t size, uint32_t line)
{
	char cbuf[7];

	format_bytes(cbuf, sizeof(cbuf), size);
	aprint_normal("%s %dB/line", cbuf, line);
}

static void
cpu_OFprintcacheinfo(int node)
{
	int l;
	uint32_t dcache=0, icache=0, dline=0, iline=0;

	OF_getprop(node, "i-cache-size", &icache, sizeof(icache));
	OF_getprop(node, "d-cache-size", &dcache, sizeof(dcache));
	OF_getprop(node, "i-cache-line-size", &iline, sizeof(iline));
	OF_getprop(node, "d-cache-line-size", &dline, sizeof(dline));
	if (OF_getprop(node, "cache-unified", &l, sizeof(l)) != -1) {
		aprint_normal("cache ");
		cpu_print_cache_config(icache, iline);
	} else {
		aprint_normal("I-cache ");
		cpu_print_cache_config(icache, iline);
		aprint_normal(", D-cache ");
		cpu_print_cache_config(dcache, dline);
	}
	aprint_normal("\n");
}

static void
cpu_OFgetcache(struct device *self, struct cpu_info *ci)
{
	int node, cpu=-1;
	char name[32];

	node = OF_finddevice("/cpus");
	if (node == -1)
		return;

	for (node = OF_child(node); node; node = OF_peer(node)) {
		uint32_t cpunum;
		int l;

		l = OF_getprop(node, "reg", &cpunum, sizeof(cpunum));
		if (l == sizeof(uint32_t) && ci->ci_cpuid == cpunum) {
			cpu = node;
			break;
		}
	}
	if (cpu == -1)
		return;
	/* now we have cpu */
	aprint_normal_dev(self, "L1 ");
	cpu_OFprintcacheinfo(cpu);
	for (node = OF_child(cpu); node; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) != -1) {
			if (strcmp("l2-cache", name) == 0) {
				aprint_normal_dev(self, "L2 ");
				cpu_OFprintcacheinfo(node);
			} else if (strcmp("l3-cache", name) == 0) {
				aprint_normal_dev(self, "L3 ");
				cpu_OFprintcacheinfo(node);
			}
		}
	}
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

	cpu_OFgetcache(self, ci);

	if (id > 0)
		return;
}
