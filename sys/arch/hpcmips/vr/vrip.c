/*	$NetBSD: vrip.c,v 1.1.1.1.8.1 1999/12/27 18:32:15 wrstuden Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/icureg.h>
#include "locators.h"

int	vripmatch __P((struct device*, struct cfdata*, void*));
void	vripattach __P((struct device*, struct device*, void*));
int	vrip_print __P((void*, const char*));
int	vrip_search __P((struct device*, struct cfdata*, void*));
int	vrip_intr __P((void*, u_int32_t, u_int32_t));

static void vrip_dump_level2mask __P((vrip_chipset_tag_t, void*));

struct cfattach vrip_ca = {
	sizeof(struct vrip_softc), vripmatch, vripattach
};

#define MAX_LEVEL1 32

struct vrip_softc *the_vrip_sc = NULL;

static struct intrhand {
	int	 (*ih_fun) __P((void *));
	void *ih_arg;
	int ih_l1line;
	int	ih_ipl;
	bus_addr_t	ih_lreg;
	bus_addr_t	ih_mlreg;
	bus_addr_t	ih_hreg;
	bus_addr_t	ih_mhreg;
} intrhand[MAX_LEVEL1] = {
	[5] = { 0, 0, 0, 0, PIUINT_REG_W,	MPIUINT_REG_W	},
	[6] = { 0, 0, 0, 0, AIUINT_REG_W,	MAIUINT_REG_W	},
	[7] = { 0, 0, 0, 0, KIUINT_REG_W,	MKIUINT_REG_W	},
	[8] = { 0, 0, 0, 0, GIUINT_L_REG_W,	MGIUINT_L_REG_W, GIUINT_H_REG_W,	MGIUINT_H_REG_W	},
	[20] = { 0, 0, 0, 0, FIRINT_REG_W,	MFIRINT_REG_W	},
	[21] = { 0, 0, 0, 0, DSIUINT_REG_W,	MDSIUINT_REG_W	}
};
#define	LEGAL_LEVEL1(x)	((x) >= 0 && (x) < MAX_LEVEL1)

void
bitdisp16 (u_int16_t a)
{
	u_int16_t j;
	for (j = 0x8000; j > 0; j >>=1)
		printf ("%c", a&j ?'|':'.');
	printf ("\n");
}

void
bitdisp32 (u_int32_t a)
{
	u_int32_t j;
	for (j = 0x80000000; j > 0; j >>=1)
		printf ("%c" , a&j ? '|' : '.');
	printf ("\n");
}

void
bitdisp64(u_int32_t a[2])
{
	u_int32_t j;
	for( j = 0x80000000 ; j > 0 ; j >>=1 )
		printf("%c" , a[1]&j ?';':',' );
	for( j = 0x80000000 ; j > 0 ; j >>=1 )
		printf("%c" , a[0]&j ?'|':'.' );
	printf("\n");
}

int
vripmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
    
	if (strcmp(ma->ma_name, match->cf_driver->cd_name))
		return 0;
	return 1;
}

void
vripattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	struct vrip_softc *sc = (struct vrip_softc*)self;

	printf("\n");
	/*
	 *  Map ICU (Interrupt Control Unit) register space.
	 */
	sc->sc_iot = ma->ma_iot;
	if (bus_space_map(sc->sc_iot, VRIP_ICU_ADDR, 0x20/*XXX lower area only*/,
			  0, /* no flags */
			  &sc->sc_ioh)) {
		printf("vripattach: can't map ICU register.\n");
		return;
	}
	
	/*
	 *  Disable all Level 1 interrupts.
	 */
	sc->sc_intrmask = 0;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, MSYSINT1_REG_W, 0x0000);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, MSYSINT2_REG_W, 0x0000);
	/*
	 *  Level 1 interrupts are redirected to HwInt0
	 */
	vr_intr_establish(VR_INTR0, vrip_intr, self);
	the_vrip_sc = sc;
	/*
	 *  Attach each devices
	 */
	/* GIU CMU interface interface is used by other system device. so attach first */
	sc->sc_pri = 2;
	config_search(vrip_search, self, vrip_print);
	/* Other system devices. */
	sc->sc_pri = 1;
	config_search(vrip_search, self, vrip_print);
}

int
vrip_print(aux, hoge)
	void *aux;
	const char *hoge;
{
	struct vrip_attach_args *va = (struct vrip_attach_args*)aux;

	if (va->va_addr)
		printf(" addr 0x%x", va->va_addr);
	if (va->va_size > 1)
		printf("-0x%x", va->va_addr + va->va_size - 1);
	if (va->va_intr != VRIPCF_INTR_DEFAULT)
		printf(" intr %d", va->va_intr);
	return (UNCONF);
}

int
vrip_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vrip_softc *sc = (struct vrip_softc *)parent;
	struct vrip_attach_args va;

	va.va_vc = sc;
	va.va_iot = sc->sc_iot;
	va.va_addr = cf->cf_loc[VRIPCF_ADDR];
	va.va_size = cf->cf_loc[VRIPCF_SIZE];
	va.va_intr = cf->cf_loc[VRIPCF_INTR];
	va.va_addr2 = cf->cf_loc[VRIPCF_ADDR2];
	va.va_size2 = cf->cf_loc[VRIPCF_SIZE2];
	va.va_gc = sc->sc_gc;
	va.va_gf = sc->sc_gf;
	va.va_cc = sc->sc_cc;
	va.va_cf = sc->sc_cf;
	if (((*cf->cf_attach->ca_match)(parent, cf, &va) == sc->sc_pri))
		config_attach(parent, cf, &va, vrip_print);

	return 0;
}

void *
vrip_intr_establish(vc, intr, level, ih_fun, ih_arg)
	vrip_chipset_tag_t vc;
	int intr;
	int level; /* XXX not yet */
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	struct intrhand *ih;

	if (!LEGAL_LEVEL1(intr))
		return 0;
	ih = &intrhand[intr];
	if (ih->ih_fun) /* Can't share level 1 interrupt */
		return 0;
	ih->ih_l1line = intr;
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
    
	/* Mask level 2 interrupt mask register. (disable interrupt) */
	vrip_intr_setmask2(vc, ih, ~0, 0);
	/* Unmask  Level 1 interrupt mask register (enable interrupt) */
	vrip_intr_setmask1(vc, ih, 1);

	return (void *)ih;
}

void
vrip_intr_disestablish(vc, arg)
	vrip_chipset_tag_t vc;
	void *arg;
{
	struct intrhand *ih = arg;
	ih->ih_fun = NULL;
	ih->ih_arg = NULL;
	/* Mask level 2 interrupt mask register(if any). (disable interrupt) */
	vrip_intr_setmask2(vc, ih, ~0, 0);
	/* Mask  Level 1 interrupt mask register (disable interrupt) */
	vrip_intr_setmask1(vc, ih, 0);
}

void
vrip_intr_suspend()
{
	bus_space_tag_t iot = the_vrip_sc->sc_iot;
	bus_space_handle_t ioh = the_vrip_sc->sc_ioh;

	bus_space_write_2 (iot, ioh, MSYSINT1_REG_W, (1<<VRIP_INTR_POWER));
	bus_space_write_2 (iot, ioh, MSYSINT2_REG_W, 0);
}

void
vrip_intr_resume()
{
	u_int32_t reg = the_vrip_sc->sc_intrmask;
	bus_space_tag_t iot = the_vrip_sc->sc_iot;
	bus_space_handle_t ioh = the_vrip_sc->sc_ioh;

	bus_space_write_2 (iot, ioh, MSYSINT1_REG_W, reg & 0xffff);
	bus_space_write_2 (iot, ioh, MSYSINT2_REG_W, (reg >> 16) & 0xffff);
}

/* Set level 1 interrupt mask. */
void
vrip_intr_setmask1(vc, arg, enable)
	vrip_chipset_tag_t vc;
	void *arg;
	int enable;
{
	struct vrip_softc *sc = (void*)vc;
	struct intrhand *ih = arg;
	int level1 = ih->ih_l1line;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t reg = sc->sc_intrmask;

	reg = (bus_space_read_2 (iot, ioh, MSYSINT1_REG_W)&0xffff) |
		((bus_space_read_2 (iot, ioh, MSYSINT2_REG_W)<< 16)&0xffff0000);
	if (enable)
		reg |= (1 << level1);
	else {
		reg &= ~(1 << level1);	
	}
	sc->sc_intrmask = reg;
	bus_space_write_2 (iot, ioh, MSYSINT1_REG_W, reg & 0xffff);
	bus_space_write_2 (iot, ioh, MSYSINT2_REG_W, (reg >> 16) & 0xffff);
/*    bitdisp32(reg);    */
    
	return;
}

static void
vrip_dump_level2mask (vc, arg)
	vrip_chipset_tag_t vc;
	void *arg;
{
	struct vrip_softc *sc = (void*)vc;
	struct intrhand *ih = arg;
	u_int32_t reg;
    
	if (ih->ih_mlreg) {
		printf ("level1[%d] level2 mask:", ih->ih_l1line);
		reg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ih->ih_mlreg);
		if (ih->ih_mhreg) { /* GIU [16:31] case only */
			reg |= (bus_space_read_2(sc->sc_iot, sc->sc_ioh, ih->ih_mhreg) << 16);
			bitdisp32(reg);
		} else
			bitdisp16(reg);
	}
}

/* Get level 2 interrupt status */
void
vrip_intr_get_status2(vc, arg, mask)
	vrip_chipset_tag_t vc;
	void *arg;
	u_int32_t *mask; /* Level 2 mask */
{
	struct vrip_softc *sc = (void*)vc;
	struct intrhand *ih = arg;
	u_int32_t reg;
	reg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, 
			       ih->ih_lreg);
	reg |= ((bus_space_read_2(sc->sc_iot, sc->sc_ioh, 
				  ih->ih_hreg) << 16)&0xffff0000);
/*    bitdisp32(reg);*/
	*mask = reg;
}

/* Set level 2 interrupt mask. */
void
vrip_intr_setmask2(vc, arg, mask, onoff)
	vrip_chipset_tag_t vc;
	void *arg;
	u_int32_t mask; /* Level 2 mask */
{
	struct vrip_softc *sc = (void*)vc;
	struct intrhand *ih = arg;
	u_int16_t reg;
#if 1
	printf("vrip_intr_setmask2:\n");
	vrip_dump_level2mask (vc, arg);
#endif
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else
	if (ih->ih_mlreg) {
		reg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ih->ih_mlreg);
		if (onoff)
			reg |= (mask&0xffff);
		else
			reg &= ~(mask&0xffff);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, ih->ih_mlreg, reg);
		if (ih->ih_mhreg != -1) { /* GIU [16:31] case only */
			reg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, ih->ih_mhreg);
			if (onoff)
				reg |= ((mask >> 16) & 0xffff);
			else
				reg &= ~((mask >> 16) & 0xffff);		
			bus_space_write_2(sc->sc_iot, sc->sc_ioh,
					  ih->ih_mhreg, reg);
		}
	}
#endif /* WINCE_DEFAULT_SETTING */
#if 0
	vrip_dump_level2mask (vc, arg);
#endif
	return;
}

int
vrip_intr(arg, pc, statusReg)
	void *arg;
	u_int32_t pc;
	u_int32_t statusReg;
{
	struct vrip_softc *sc = (struct vrip_softc*)arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;
	u_int32_t reg, mask;
	/*
	 *  Read level1 interrupt status.
	 */
	reg = (bus_space_read_2 (iot, ioh, SYSINT1_REG_W)&0xffff) |
		((bus_space_read_2 (iot, ioh, SYSINT2_REG_W)<< 16)&0xffff0000);
	mask = (bus_space_read_2 (iot, ioh, MSYSINT1_REG_W)&0xffff) |
		((bus_space_read_2 (iot, ioh, MSYSINT2_REG_W)<< 16)&0xffff0000);
	reg &= mask;

	/*
	 *  Dispatch each handler.
	 */
	for (i = 0; i < 32; i++) {
		register struct intrhand *ih = &intrhand[i];
		if (ih->ih_fun && (reg & (1 << i))) {
			ih->ih_fun(ih->ih_arg);
		}
	}
	return 1;
}

void
vrip_cmu_function_register(vc, func, arg)
	vrip_chipset_tag_t vc;
	vrcmu_function_tag_t func;
	vrcmu_chipset_tag_t arg;
{
	struct vrip_softc *sc = (void*)vc;
	sc->sc_cf = func;
	sc->sc_cc = arg;
}

void
vrip_giu_function_register(vc, func, arg)
	vrip_chipset_tag_t vc;
	vrgiu_function_tag_t func;
	vrgiu_chipset_tag_t arg;
{
	struct vrip_softc *sc = (void*)vc;
	sc->sc_gf = func;
	sc->sc_gc = arg;
}

