/*	$NetBSD: bcu_vrip.c,v 1.1.1.1 1999/09/16 12:23:31 takemura Exp $	*/

/*-
 * Copyright (c) 1999 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>

#include <mips/cpuregs.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/bcureg.h>
#include <hpcmips/vr/bcuvar.h>

struct vrbcu_vrip_softc {
  struct vrbcu_softc sc_vrbcu;
};

/* Definition of the mainbus driver. */
static int	vrbcu_vrip_match __P((struct device *,
				      struct cfdata *, void *));
static void	vrbcu_vrip_attach __P((struct device *,

				       struct device *, void*));
char	*vrbcu_vrip_getcpuname __P((void));
int	vrbcu_vrip_getcpumajor __P((void));
int	vrbcu_vrip_getcpuminor __P((void));

struct cfattach vrbcu_ca = {
	sizeof(struct vrbcu_vrip_softc), vrbcu_vrip_match, vrbcu_vrip_attach
};

static int
vrbcu_vrip_match(parent, cf, aux)
     struct device *parent;
     struct cfdata *cf;
     void *aux;
{
	return 1;
}

static void
vrbcu_vrip_attach(parent, self, aux)
     struct device *parent;
     struct device *self;
     void *aux;
{
	struct vrip_attach_args *va = aux;
	struct vrbcu_vrip_softc *sc = (struct vrbcu_vrip_softc *)self;

	sc->sc_vrbcu.sc_iot = va->va_iot;
	bus_space_map(sc->sc_vrbcu.sc_iot, va->va_addr, va->va_size,
		0, /* no flags */
		&sc->sc_vrbcu.sc_ioh);

	printf("\n");
}

static char *cpuname[] = {
	"VR4101",
	"VR4102",
	"VR4111",
	"VR4121",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN" };

char *
vrbcu_vrip_getcpuname(void)
{
	volatile u_int16_t *revreg;
	int cpuid;

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1((VRIP_BCU_ADDR+BCUREVID_REG_W));

	cpuid = *revreg;
	cpuid = (cpuid&BCUREVID_RIDMASK)>>BCUREVID_RIDSHFT;
	return cpuname[cpuid];
}	

int
vrbcu_vrip_getcpumajor(void)
{
	volatile u_int16_t *revreg;
	int major;

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1((VRIP_BCU_ADDR+BCUREVID_REG_W));

	major = *revreg;
	major = (major&BCUREVID_MJREVMASK)>>BCUREVID_MJREVSHFT;
	return major;
}	

int
vrbcu_vrip_getcpuminor(void)
{
	volatile u_int16_t *revreg;
	int minor;

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1((VRIP_BCU_ADDR+BCUREVID_REG_W));

	minor = *revreg;
	minor = (minor&BCUREVID_MNREVMASK)>>BCUREVID_MNREVSHFT;
	return minor;
}	
