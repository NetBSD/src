/*	$NetBSD: bcu_vrip.c,v 1.3 1999/12/16 09:36:19 sato Exp $	*/

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

static int vrbcu_match __P((struct device *, struct cfdata *, void *));
static void vrbcu_attach __P((struct device *, struct device *, void *));

static void vrbcu_write __P((struct vrbcu_softc *, int, unsigned short));
static unsigned short vrbcu_read __P((struct vrbcu_softc *, int));

char	*vr_cpuname=NULL;
int	vr_major=-1;
int	vr_minor=-1;
int	vr_cpuid=-1;

struct cfattach vrbcu_ca = {
	sizeof(struct vrbcu_softc), vrbcu_match, vrbcu_attach
};

struct vrbcu_softc *the_bcu_sc = NULL;

static inline void
vrbcu_write(sc, port, val)
	struct vrbcu_softc *sc;
	int port;
	unsigned short val;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrbcu_read(sc, port)
	struct vrbcu_softc *sc;
	int port;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, port);
}

static int
vrbcu_match(parent, cf, aux)
     struct device *parent;
     struct cfdata *cf;
     void *aux;
{
	return 2;
}

static void
vrbcu_attach(parent, self, aux)
     struct device *parent;
     struct device *self;
     void *aux;
{
	struct vrip_attach_args *va = aux;
	struct vrbcu_softc *sc = (struct vrbcu_softc *)self;

	sc->sc_iot = va->va_iot;
	bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
		0, /* no flags */
		&sc->sc_ioh);

	printf("\n");
	the_bcu_sc = sc;
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

int
vrbcu_vrip_getcpuid(void)
{
	volatile u_int16_t *revreg;

	if (vr_cpuid != -1)
		return vr_cpuid; 

	if (vr_cpuid == -1) {
		revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1((VRIP_BCU_ADDR+BCUREVID_REG_W));

		vr_cpuid = *revreg;
		vr_cpuid = (vr_cpuid&BCUREVID_RIDMASK)>>BCUREVID_RIDSHFT;
	}
	return vr_cpuid;
}	

char *
vrbcu_vrip_getcpuname(void)
{
	int cpuid;	

	if (vr_cpuname != NULL)
		return vr_cpuname;

	cpuid = vrbcu_vrip_getcpuid();
	vr_cpuname = cpuname[cpuid];
	return vr_cpuname;
}	


int
vrbcu_vrip_getcpumajor(void)
{
	volatile u_int16_t *revreg;

	if (vr_major != -1)
		return vr_major;

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1((VRIP_BCU_ADDR+BCUREVID_REG_W));

	vr_major = *revreg;
	vr_major = (vr_major&BCUREVID_MJREVMASK)>>BCUREVID_MJREVSHFT;
	return vr_major;
}	

int
vrbcu_vrip_getcpuminor(void)
{
	volatile u_int16_t *revreg;

	if (vr_minor != -1)
		return vr_minor;

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1((VRIP_BCU_ADDR+BCUREVID_REG_W));

	vr_minor = *revreg;
	vr_minor = (vr_minor&BCUREVID_MNREVMASK)>>BCUREVID_MNREVSHFT;
	return vr_minor;
}	
