/*	$NetBSD: cpu.c,v 1.6 2003/07/15 02:46:32 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.6 2003/07/15 02:46:32 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>
#include <machine/cpu.h>

static int cpu_match(struct device *, struct cfdata *, void *);
static void cpu_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct cpu_softc),
    cpu_match, cpu_attach, NULL, NULL);

int
cpu_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	char name[32];

	if (strcmp(oba->oba_busname, "cpu") != 0)
		return (0);

	if (OF_getprop(oba->oba_phandle, "device_type", name,
	    sizeof(name)) <= 3)
		return (0);

	if (strcmp(name, "cpu") == 0)
		return (1);

	return (0);
}

void
cpu_attach(struct device *parent, struct device *self, void *aux)
{
	struct cpu_softc *sc = (struct cpu_softc *) self;
	struct ofbus_attach_args *oba = aux;
	unsigned char data[4];
	int tbase, cpunum;

	sc->sc_ofnode = oba->oba_phandle;

	if (OF_getprop(sc->sc_ofnode, "reg",
	    data, sizeof(data)) < sizeof(data)) {
		printf(": unable to get CPU ID\n");
		return;
	}
	cpunum = of_decode_int(data);

	cpu_attach_common(self, cpunum);

	if (OF_getprop(oba->oba_phandle, "timebase-frequency",
	    data, sizeof(data)) < sizeof(data))
		printf("%s: unable to get timebase-frequence property\n",
		    sc->sc_dev.dv_xname);
	else {
		tbase = of_decode_int(data);
		if (cpu_timebase == 0)
			cpu_timebase = tbase;
		else if (tbase != cpu_timebase)
			printf("%s: WARNING: timebase %d != %d\n",
			    sc->sc_dev.dv_xname, tbase, cpu_timebase);
	}
}
