/*	$NetBSD: vrpmu.c,v 1.6 2000/03/14 08:23:24 sato Exp $	*/

/*
 * Copyright (c) 1999 M. Warner Losh.  All rights reserved.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/config_hook.h>

#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrpmuvar.h>
#include <hpcmips/vr/vrpmureg.h>

#include "vrbcu.h"
#if NVRBCU > 0
#include <hpcmips/vr/bcuvar.h>
#include <hpcmips/vr/bcureg.h>
#endif

int vrpmu_pwsw = 0;

#ifdef VRPMUDEBUG
#define DEBUG_BOOT	0x1	/* boot time */
#define DEBUG_INTR	0x2	/* intr */
#define DEBUG_IO	0x4	/* I/O */
#ifndef VRPMUDEBUG_CONF
#define VRPMUDEBUG_CONF 0
#endif /* VRPMUDEBUG_CONF */
int vrpmudebug = VRPMUDEBUG_CONF;
#define DPRINTF(flag, arg) if (vrpmudebug&flag) printf arg;
#define DDUMP_INTR2(flag, arg1, arg2) if (vrpmudebug&flag) vrpmu_dump_intr2(arg1,arg2);
#define DDUMP_REGS(flag, arg) if (vrpmudebug&flag) vrpmu_dump_regs(arg);
#else /* VRPMUDEBUG */
#define DPRINTF(flag, arg)
#define DDUMP_INTR2(flag, arg1, arg2)
#define DDUMP_REGS(flag, arg)
#endif /* VRPMUDEBUG */

static int vrpmumatch __P((struct device *, struct cfdata *, void *));
static void vrpmuattach __P((struct device *, struct device *, void *));

static void vrpmu_write __P((struct vrpmu_softc *, int, unsigned short));
static unsigned short vrpmu_read __P((struct vrpmu_softc *, int));

int vrpmu_intr __P((void *));
void vrpmu_dump_intr __P((void *));
void vrpmu_dump_intr2 __P((unsigned int, unsigned int));
void vrpmu_dump_regs __P((void *));

struct cfattach vrpmu_ca = {
	sizeof(struct vrpmu_softc), vrpmumatch, vrpmuattach
};

static inline void
vrpmu_write(sc, port, val)
	struct vrpmu_softc *sc;
	int port;
	unsigned short val;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrpmu_read(sc, port)
	struct vrpmu_softc *sc;
	int port;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, port);
}

static int
vrpmumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

static void
vrpmuattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrpmu_softc *sc = (struct vrpmu_softc *)self;
	struct vrip_attach_args *va = aux;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	if (!(sc->sc_handler = 
	      vrip_intr_establish(va->va_vc, va->va_intr, IPL_TTY,
				  vrpmu_intr, sc))) {
		printf (": can't map interrupt line.\n");
		return;
	}

	printf("\n");
	/* dump current intrrupt states */
	vrpmu_dump_intr(sc);
	DDUMP_REGS(DEBUG_BOOT, sc);
	/* clear interrupt status */
	vrpmu_write(sc, PMUINT_REG_W, PMUINT_ALL);
	vrpmu_write(sc, PMUINT2_REG_W, PMUINT2_ALL);
}

/*
 * dump PMU intr status regs
 *
 */
void
vrpmu_dump_intr(arg)
	void *arg;
{
        struct vrpmu_softc *sc = arg;
	unsigned int intstat1;
	unsigned int intstat2;
	intstat1 = vrpmu_read(sc, PMUINT_REG_W);
	intstat2 = vrpmu_read(sc, PMUINT2_REG_W);
	vrpmu_dump_intr2(intstat1, intstat2);

}

/*
 * dump PMU intr status regs
 */
void
vrpmu_dump_intr2(intstat1, intstat2)
unsigned int intstat1, intstat2;
{
	if (intstat1 & PMUINT_GPIO3)
		printf("vrpmu: GPIO[3] activation\n");
	if (intstat1 & PMUINT_GPIO2)
		printf("vrpmu: GPIO[2] activation\n");
	if (intstat1 & PMUINT_GPIO1)
		printf("vrpmu: GPIO[1] activation\n");
	if (intstat1 & PMUINT_GPIO0)
		printf("vrpmu: GPIO[0] activation\n");

	if (intstat1 & PMUINT_RTC)
		printf("vrpmu: RTC alarm detected\n");
	if (intstat1 & PMUINT_BATT)
		printf("vrpmu: Battery low during activation\n");

	if (intstat1 & PMUINT_TIMOUTRST)
		printf("vrpmu: HAL timer reset\n");
	if (intstat1 & PMUINT_RTCRST)
		printf("vrpmu: RTC reset detected\n");
	if (intstat1 & PMUINT_RSTSWRST)
		printf("vrpmu: RESET switch detected\n");
	if (intstat1 & PMUINT_DMSWRST)
		printf("vrpmu: Deadman's switch detected\n");
	if (intstat1 & PMUINT_BATTINTR)
		printf("vrpmu: Battery low during normal ops\n");
	if (intstat1 & PMUINT_POWERSW)
		printf("vrpmu: POWER switch detected\n");

	if (intstat2 & PMUINT_GPIO12)
		printf("vrpmu: GPIO[12] activation\n");
	if (intstat2 & PMUINT_GPIO11)
		printf("vrpmu: GPIO[11] activation\n");
	if (intstat2 & PMUINT_GPIO10)
		printf("vrpmu: GPIO[10] activation\n");
	if (intstat2 & PMUINT_GPIO9)
		printf("vrpmu: GPIO[9] activation\n");
}

/*
 * dump PMU registers
 *
 */
void
vrpmu_dump_regs(arg)
	void *arg;
{
        struct vrpmu_softc *sc = arg;
	unsigned int intstat1;
	unsigned int intstat2;
	unsigned int reg;
#if NVRBCU > 0
	int cpuid;
#endif
	intstat1 = vrpmu_read(sc, PMUINT_REG_W);
	intstat2 = vrpmu_read(sc, PMUINT2_REG_W);

	/* others? XXXX */
	reg = vrpmu_read(sc, PMUCNT_REG_W);
	printf("vrpmu: cnt 0x%x: ", reg);
	bitdisp16(reg);
	reg = vrpmu_read(sc, PMUCNT2_REG_W);
	printf("vrpmu: cnt2 0x%x: ", reg);
	bitdisp16(reg);
#if NVRBCU > 0
	cpuid = vrbcu_vrip_getcpuid();
	if (cpuid >= BCUREVID_RID_4111){
		reg = vrpmu_read(sc, PMUWAIT_REG_W);
		printf("vrpmu: wait 0x%x", reg);
	}
	if (cpuid >= BCUREVID_RID_4121){
		reg = vrpmu_read(sc, PMUDIV_REG_W);
		printf(" div 0x%x", reg);
	}
	printf("\n");
#endif /*  NVRBCU > 0 */
}

/*
 * PMU interrupt handler.
 * XXX
 *
 * In the following interrupt routine we should actually DO something
 * with the knowledge that we've gained.  For now we just report it.
 */
int
vrpmu_intr(arg)
	void *arg;
{
        struct vrpmu_softc *sc = arg;
	unsigned int intstat1;
	unsigned int intstat2;

	intstat1 = vrpmu_read(sc, PMUINT_REG_W);
	/* clear interrupt status */
	vrpmu_write(sc, PMUINT_REG_W, intstat1);


	intstat2 = vrpmu_read(sc, PMUINT2_REG_W);
	/* clear interrupt status */
	vrpmu_write(sc, PMUINT2_REG_W, intstat2);

	DDUMP_INTR2(DEBUG_INTR, intstat1, intstat2);

	if (intstat1 & PMUINT_GPIO3)
		;
	if (intstat1 & PMUINT_GPIO2)
		;
	if (intstat1 & PMUINT_GPIO1)
		;
	if (intstat1 & PMUINT_GPIO0)
		;

	if (intstat1 & PMUINT_RTC)
		;
	if (intstat1 & PMUINT_BATT)
		;

	if (intstat1 & PMUINT_TIMOUTRST)
		;
	if (intstat1 & PMUINT_RTCRST)
		;
	if (intstat1 & PMUINT_RSTSWRST)
		;
	if (intstat1 & PMUINT_DMSWRST)
		;
	if (intstat1 & PMUINT_BATTINTR)
		;
	if (intstat1 & PMUINT_POWERSW) {
		vrpmu_pwsw = !vrpmu_pwsw;
		config_hook_call(CONFIG_HOOK_BUTTONEVENT,
				 CONFIG_HOOK_BUTTONEVENT_POWER,
				 (void*)vrpmu_pwsw);
	}

	if (intstat2 & PMUINT_GPIO12)
		;
	if (intstat2 & PMUINT_GPIO11)
		;
	if (intstat2 & PMUINT_GPIO10)
		;
	if (intstat2 & PMUINT_GPIO9)
		;

	return 0;
}
