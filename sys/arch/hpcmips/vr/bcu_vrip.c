/*	$NetBSD: bcu_vrip.c,v 1.10.4.1 2001/10/01 12:39:16 fvdl Exp $	*/

/*-
 * Copyright (c) 1999-2001 SATO Kazumi. All rights reserved.
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

#include "opt_vr41xx.h"
#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/bcureg.h>
#include <hpcmips/vr/bcuvar.h>

static int vrbcu_match(struct device *, struct cfdata *, void *);
static void vrbcu_attach(struct device *, struct device *, void *);

static void vrbcu_write(struct vrbcu_softc *, int, unsigned short);
static unsigned short vrbcu_read(struct vrbcu_softc *, int);

static void vrbcu_dump_regs(void);

char	*vr_cpuname=NULL;
int	vr_major=-1;
int	vr_minor=-1;
int	vr_cpuid=-1;

struct cfattach vrbcu_ca = {
	sizeof(struct vrbcu_softc), vrbcu_match, vrbcu_attach
};

struct vrbcu_softc *the_bcu_sc = NULL;

static inline void
vrbcu_write(struct vrbcu_softc *sc, int port, unsigned short val)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrbcu_read(struct vrbcu_softc *sc, int port)
{

	return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, port));
}

static int
vrbcu_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (2);
}

static void
vrbcu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vrbcu_softc *sc = (struct vrbcu_softc *)self;

	sc->sc_iot = va->va_iot;
	bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
	    0, /* no flags */
	    &sc->sc_ioh);

	printf("\n");
	the_bcu_sc = sc;
	vrbcu_dump_regs();
}

static void
vrbcu_dump_regs()
{
	struct vrbcu_softc *sc = the_bcu_sc;
	int cpuclock = 0, tclock = 0, vtclock = 0, cpuid;
#if !defined(ONLY_VR4102)
	int spdreg;
#endif
#ifdef VRBCUDEBUG
	int reg;
#endif /* VRBCUDEBUG */

	cpuid = vrbcu_vrip_getcpuid();
#if !defined(ONLY_VR4181) && !defined(ONLY_VR4102)
	if (cpuid != BCUREVID_FIXRID_4181 
	    && cpuid <= BCUREVID_RID_4131
	    && cpuid >= BCUREVID_RID_4111) {
		spdreg = vrbcu_read(sc, BCUCLKSPEED_REG_W);
#ifdef VRBCUDEBUG
		printf("vrbcu: CLKSPEED %x: \n",  spdreg);
#endif /* VRBCUDEBUG */
	}
#endif
#if defined VR4181
	if (cpuid == BCUREVID_FIXRID_4181){
		spdreg = vrbcu_read(sc, BCU81CLKSPEED_REG_W);
#ifdef VRBCUDEBUG
		printf("vrbcu: CLKSPEED %x: \n",  spdreg);
#endif /* VRBCUDEBUG */
	}
#endif

	cpuclock = vrbcu_vrip_getcpuclock();

	switch (cpuid) {
#if defined VR4181
	case BCUREVID_FIXRID_4181:
		switch ((spdreg & BCU81CLKSPEED_DIVTMASK) >>
		    BCU81CLKSPEED_DIVTSHFT){
		case BCU81CLKSPEED_DIVT1:
			vtclock = tclock = cpuclock;
			break;
		case BCU81CLKSPEED_DIVT2:
			vtclock = tclock = cpuclock/2;
			break;
		case BCU81CLKSPEED_DIVT3:
			vtclock = tclock = cpuclock/3;
			break;
		case BCU81CLKSPEED_DIVT4:
			vtclock = tclock = cpuclock/4;
			break;
		default:
			vtclock = tclock = 0;
		}
		break;
#endif /* VR4181 */
	case BCUREVID_RID_4101:
	case BCUREVID_RID_4102:
		vtclock = tclock = cpuclock/2;
		break;
#if defined VR4111
	case BCUREVID_RID_4111:
		if ((spdreg&BCUCLKSPEED_DIVT2B) == 0) 
			vtclock = tclock = cpuclock/2;
		else if ((spdreg&BCUCLKSPEED_DIVT3B) == 0) 
			vtclock = tclock = cpuclock/3;
		else if ((spdreg&BCUCLKSPEED_DIVT4B) == 0) 
			vtclock = tclock = cpuclock/4;
		else
			vtclock = tclock = 0; /* XXX */
		break;
#endif /* VR4111 */
#if defined VR4121
	case BCUREVID_RID_4121:
	{
		int vt;
		tclock = cpuclock / ((spdreg & BCUCLKSPEED_DIVTMASK) >>
		    BCUCLKSPEED_DIVTSHFT);
		vt = ((spdreg & BCUCLKSPEED_DIVVTMASK) >>
		    BCUCLKSPEED_DIVVTSHFT);
		if (vt == 0)
			vtclock = 0; /* XXX */
		else if (vt < 0x9)
			vtclock = cpuclock / vt;
		else
			vtclock = cpuclock / ((vt - 8)*2+1) * 2;
	}
	break;
#endif /* VR4121 */
#if defined VR4122 || defined VR4131
	case BCUREVID_RID_4122:
	case BCUREVID_RID_4131:
	{
		int vtdiv;

		vtdiv = ((spdreg & BCUCLKSPEED_VTDIVMODE) >>
		    BCUCLKSPEED_VTDIVSHFT);
		if (vtdiv == 0 || vtdiv > BCUCLKSPEED_VTDIV6)
			vtclock = 0; /* XXX */
		else
			vtclock = cpuclock / vtdiv;
		tclock = vtclock /
		    (((spdreg & BCUCLKSPEED_TDIVMODE) >>
			BCUCLKSPEED_TDIVSHFT) ? 4 : 2);
	}
	break;
#endif /* VR4122 || VR4131 */
	default:
		break;
	}
	if (tclock)
		printf("%s: cpu %d.%03dMHz, bus %d.%03dMHz, ram %d.%03dMHz\n",
		    sc->sc_dev.dv_xname,
		    cpuclock/1000000, (cpuclock%1000000)/1000,
		    tclock/1000000, (tclock%1000000)/1000,
		    vtclock/1000000, (vtclock%1000000)/1000);
	else {
		printf("%s: cpu %d.%03dMHz\n",
		    sc->sc_dev.dv_xname,
		    cpuclock/1000000, (cpuclock%1000000)/1000);
		printf("%s: UNKNOWN BUS CLOCK SPEED:"
		    " CPU is UNKNOWN or NOT CONFIGURED\n",
		    sc->sc_dev.dv_xname);
	}
#ifdef VRBCUDEBUG
	reg = vrbcu_read(sc, BCUCNT1_REG_W);
	printf("vrbcu: CNT1 %x: ",  reg);
	bitdisp16(reg);
#if !defined(ONLY_VR4181)
	if (cpuid != BCUREVID_FIXRID_4181 
	    && cpuid <= BCUREVID_RID_4121
	    && cpuid >= BCUREVID_RID_4102) {
		reg = vrbcu_read(sc, BCUCNT2_REG_W);
		printf("vrbcu: CNT2 %x: ",  reg);
		bitdisp16(reg);
	}
#endif /* !defined ONLY_VR4181 */
#if !defined(ONLY_VR4181) || !defined(ONLY_VR4122_4131)
	if (cpuid != BCUREVID_FIXRID_4181
	    && cpuid <= BCUREVID_RID_4121
	    && cpuid >= BCUREVID_RID_4102) {
		reg = vrbcu_read(sc, BCUSPEED_REG_W);
		printf("vrbcu: SPEED %x: ",  reg);
		bitdisp16(reg);
		reg = vrbcu_read(sc, BCUERRST_REG_W);
		printf("vrbcu: ERRST %x: ",  reg);
		bitdisp16(reg);
		reg = vrbcu_read(sc, BCURFCNT_REG_W);
		printf("vrbcu: RFCNT %x\n",  reg);
		reg = vrbcu_read(sc, BCUREFCOUNT_REG_W);
		printf("vrbcu: RFCOUNT %x\n",  reg);
	}
#endif /* !defined(ONLY_VR4181) || !defined(ONLY_VR4122_4131) */
#if !defined(ONLY_VR4181)
	if (cpuid != BCUREVID_FIXRID_4181 
	    && cpuid <= BCUREVID_RID_4131
	    && cpuid >= BCUREVID_RID_4111)
	{
		reg = vrbcu_read(sc, BCUCNT3_REG_W);
		printf("vrbcu: CNT3 %x: ",  reg);
		bitdisp16(reg);
	}
#endif /* !defined ONLY_VR4181 */
#endif /* VRBCUDEBUG */

}

static char *cpuname[] = {
	"VR4101",	/* 0 */
	"VR4102",	/* 1 */
	"VR4111",	/* 2 */
	"VR4121",	/* 3 */
	"VR4122",	/* 4 */
	"VR4131",	/* 5 */
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"VR4181",	/* 0x10 + 0 */
};

int
vrbcu_vrip_getcpuid(void)
{
	volatile u_int16_t *revreg;

	if (vr_cpuid != -1)
		return (vr_cpuid); 

	if (vr_cpuid == -1) {
		if (VRIP_BCU_ADDR == VR4181_BCU_ADDR)
			revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1
			    ((VRIP_BCU_ADDR+BCU81REVID_REG_W));
		else
			revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1
			    ((VRIP_BCU_ADDR+BCUREVID_REG_W));

		vr_cpuid = *revreg;
		vr_cpuid = (vr_cpuid&BCUREVID_RIDMASK)>>BCUREVID_RIDSHFT;
		if (VRIP_BCU_ADDR == VR4181_BCU_ADDR 
		    && vr_cpuid == BCUREVID_RID_4181) /* conflict vr4101 */
			vr_cpuid = BCUREVID_FIXRID_4181;
	}
	return (vr_cpuid);
}	

char *
vrbcu_vrip_getcpuname(void)
{
	int cpuid;	

	if (vr_cpuname != NULL)
		return (vr_cpuname);

	cpuid = vrbcu_vrip_getcpuid();
	vr_cpuname = cpuname[cpuid];

	return (vr_cpuname);
}	


int
vrbcu_vrip_getcpumajor(void)
{
	volatile u_int16_t *revreg;

	if (vr_major != -1)
		return (vr_major);

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1
	    ((VRIP_BCU_ADDR+BCUREVID_REG_W));

	vr_major = *revreg;
	vr_major = (vr_major&BCUREVID_MJREVMASK)>>BCUREVID_MJREVSHFT;

	return (vr_major);
}	

int
vrbcu_vrip_getcpuminor(void)
{
	volatile u_int16_t *revreg;

	if (vr_minor != -1)
		return (vr_minor);

	revreg = (u_int16_t *)MIPS_PHYS_TO_KSEG1
	    ((VRIP_BCU_ADDR+BCUREVID_REG_W));

	vr_minor = *revreg;
	vr_minor = (vr_minor&BCUREVID_MNREVMASK)>>BCUREVID_MNREVSHFT;

	return (vr_minor);
}	

#define CLKX	18432000	/* CLKX1,CLKX2: 18.432MHz */
#define MHZ	1000000

int
vrbcu_vrip_getcpuclock(void)
{
	u_int16_t clksp;
	int cpuid, cpuclock;

	cpuid = vrbcu_vrip_getcpuid();
	if (cpuid != BCUREVID_FIXRID_4181 && cpuid >= BCUREVID_RID_4111) {
		clksp = *(u_int16_t *)MIPS_PHYS_TO_KSEG1
		    ((VRIP_BCU_ADDR+BCUCLKSPEED_REG_W)) &
		    BCUCLKSPEED_CLKSPMASK;
	} else if (cpuid == BCUREVID_FIXRID_4181) {
		clksp = *(u_int16_t *)MIPS_PHYS_TO_KSEG1
		    ((VRIP_BCU_ADDR+BCU81CLKSPEED_REG_W)) &
		    BCUCLKSPEED_CLKSPMASK;
	}

	switch (cpuid) {
	case BCUREVID_FIXRID_4181:
		cpuclock = CLKX / clksp * 64;
		/* branch delay is 1 clock; 2 clock/loop */
		cpuspeed = (cpuclock / 2 + MHZ / 2) / MHZ;
		break;
	case BCUREVID_RID_4101:
		/* assume 33MHz */
		cpuclock = 33000000;
		/* branch delay is 1 clock; 2 clock/loop */
		cpuspeed = (cpuclock / 2 + MHZ / 2) / MHZ;
		break;
	case BCUREVID_RID_4102:
		cpuclock = CLKX / clksp * 32;
		/* branch delay is 1 clock; 2 clock/loop */
		cpuspeed = (cpuclock / 2 + MHZ / 2) / MHZ;
		break;
	case BCUREVID_RID_4111:
		cpuclock = CLKX / clksp * 64;
		/* branch delay is 1 clock; 2 clock/loop */
		cpuspeed = (cpuclock / 2 + MHZ / 2) / MHZ;
		break;
	case BCUREVID_RID_4121:
		cpuclock = CLKX / clksp * 64;
		/* branch delay is 2 clock; 3 clock/loop */
		cpuspeed = (cpuclock / 3 + MHZ / 2) / MHZ;
		break;
	case BCUREVID_RID_4122:
		cpuclock = CLKX / clksp * 98;
		/* branch delay is 2 clock; 3 clock/loop */
		cpuspeed = (cpuclock / 3 + MHZ / 2) / MHZ;
		break;
	case BCUREVID_RID_4131:
		cpuclock = CLKX / clksp * 98;
		/* branch delay is 2 clock; 3 clock/loop */
		cpuspeed = (cpuclock / 3 + MHZ / 2) / MHZ;
		break;
	default:
		panic("unknown CPU type %d\n", cpuid);
		break;
	}

	return (cpuclock);
}
