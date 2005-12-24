/*	$NetBSD: tx39power.c,v 1.15 2005/12/24 23:24:00 perry Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: tx39power.c,v 1.15 2005/12/24 23:24:00 perry Exp $");

#include "opt_tx39power_debug.h"
#define TX39POWERDEBUG 

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/config_hook.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39powerreg.h>

#ifdef	TX39POWER_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	tx39power_debug
#endif
#include <machine/debug.h>

#ifdef TX39POWER_DEBUG
#define DUMP_REGS(x)		__tx39power_dump(x)
#else
#define DUMP_REGS(x)		((void)0)
#endif

#define ISSET(x, v)		((x) & (v))
#define ISSETPRINT(r, m)	dbg_bitmask_print(r, TX39_POWERCTRL_##m, #m)

int	tx39power_match(struct device *, struct cfdata *, void *);
void	tx39power_attach(struct device *, struct device *, void *);

struct tx39power_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;

	/* save interrupt status for resume */
	txreg_t sc_icu_state[TX39_INTRSET_MAX + 1];
};

CFATTACH_DECL(tx39power, sizeof(struct tx39power_softc),
    tx39power_match, tx39power_attach, NULL, NULL);

void tx39power_suspend_cpu(void); /* automatic hardware resume */

static int tx39power_intr_p(void *);
static int tx39power_intr_n(void *);
static int tx39power_ok_intr_p(void *);
static int tx39power_ok_intr_n(void *);
static int tx39power_button_intr_p(void *);
static int tx39power_button_intr_n(void *);
#ifdef TX39POWER_DEBUG
static void __tx39power_dump(struct tx39power_softc *);
#endif

int
tx39power_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (ATTACH_FIRST);
}

void
tx39power_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct tx39power_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	txreg_t reg;
	
	tc = sc->sc_tc = ta->ta_tc;
	tx_conf_register_power(tc, self);

	printf("\n");
	DUMP_REGS(sc);

	/* power button setting */
	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);
	reg |= TX39_POWERCTRL_DBNCONBUTN;
	tx_conf_write(tc, TX39_POWERCTRL_REG, reg);
	
	/* enable stop timer */
	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);
	reg &= ~(TX39_POWERCTRL_STPTIMERVAL_MASK <<
	    TX39_POWERCTRL_STPTIMERVAL_SHIFT);
	reg = TX39_POWERCTRL_STPTIMERVAL_SET(reg,
	    TX39_POWERCTRL_STPTIMERVAL_MAX);
	reg |= TX39_POWERCTRL_ENSTPTIMER;
	tx_conf_write(tc, TX39_POWERCTRL_REG, reg);

	/* install power event handler */
	/* low priority */
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSPWRINT),
	    IST_EDGE, IPL_CLOCK, 
	    tx39power_intr_p, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGPWRINT),
	    IST_EDGE, IPL_CLOCK,			    
	    tx39power_intr_n, sc);
	/* high priority */
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSPWROKINT),
	    IST_EDGE, IPL_CLOCK, 
	    tx39power_ok_intr_p, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGPWROKINT),
	    IST_EDGE, IPL_CLOCK,			    
	    tx39power_ok_intr_n, sc);
	/* user driven event */
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSONBUTNINT),
	    IST_EDGE, IPL_CLOCK,
	    tx39power_button_intr_p, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGONBUTNINT),
	    IST_EDGE, IPL_CLOCK,			    
	    tx39power_button_intr_n, sc);
}

void
tx39power_suspend_cpu() /* I assume already splhigh */
{
	tx_chipset_tag_t tc = tx_conf_get_tag();
	struct tx39power_softc *sc = tc->tc_powert;
	txreg_t reg, *iregs = sc->sc_icu_state;

	printf ("%s: CPU sleep\n", sc->sc_dev.dv_xname);
	__asm volatile(".set noreorder");
	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);
	reg |= TX39_POWERCTRL_STOPCPU;
	/* save interrupt state */
	iregs[0] = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	iregs[1] = tx_conf_read(tc, TX39_INTRENABLE1_REG);
	iregs[2] = tx_conf_read(tc, TX39_INTRENABLE2_REG);
	iregs[3] = tx_conf_read(tc, TX39_INTRENABLE3_REG);
	iregs[4] = tx_conf_read(tc, TX39_INTRENABLE4_REG);
	iregs[5] = tx_conf_read(tc, TX39_INTRENABLE5_REG);
#ifdef TX392X
	iregs[7] = tx_conf_read(tc, TX39_INTRENABLE7_REG);
	iregs[8] = tx_conf_read(tc, TX39_INTRENABLE8_REG);
#endif
	/* disable all interrupt (don't disable GLOBALEN) */
	tx_conf_write(tc, TX39_INTRENABLE6_REG, TX39_INTRENABLE6_GLOBALEN);
	tx_conf_write(tc, TX39_INTRENABLE1_REG, 0);
	tx_conf_write(tc, TX39_INTRENABLE2_REG, 0);
	tx_conf_write(tc, TX39_INTRENABLE3_REG, 0);
	tx_conf_write(tc, TX39_INTRENABLE4_REG, 0);
	tx_conf_write(tc, TX39_INTRENABLE5_REG, 0);
#ifdef TX392X
	tx_conf_write(tc, TX39_INTRENABLE7_REG, 0);
	tx_conf_write(tc, TX39_INTRENABLE8_REG, 0);
#endif
	/* enable power button interrupt only */
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_NEGONBUTNINT);
	tx_conf_write(tc, TX39_INTRENABLE5_REG, TX39_INTRSTATUS5_NEGONBUTNINT);
	__asm volatile("sync");

	/* stop CPU clock */
	tx_conf_write(tc, TX39_POWERCTRL_REG, reg);
	__asm volatile("sync");
	/* wait until power button pressed */
	/* clear interrupt */
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_NEGONBUTNINT);
#ifdef TX392X
	/* Clear WARMSTART bit to reset vector(0xbfc00000) work correctly */
	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);
	reg &= ~TX39_POWERCTRL_WARMSTART;
	tx_conf_write(tc, TX39_POWERCTRL_REG, reg);
#endif

	/* restore interrupt state */
	tx_conf_write(tc, TX39_INTRENABLE6_REG, iregs[0]);
	tx_conf_write(tc, TX39_INTRENABLE1_REG, iregs[1]);
	tx_conf_write(tc, TX39_INTRENABLE2_REG, iregs[2]);
	tx_conf_write(tc, TX39_INTRENABLE3_REG, iregs[3]);
	tx_conf_write(tc, TX39_INTRENABLE4_REG, iregs[4]);
	tx_conf_write(tc, TX39_INTRENABLE5_REG, iregs[5]);
#ifdef TX392X
	tx_conf_write(tc, TX39_INTRENABLE7_REG, iregs[7]);
	tx_conf_write(tc, TX39_INTRENABLE8_REG, iregs[8]);
#endif
	__asm volatile(".set reorder");

	printf ("%s: CPU wakeup\n", sc->sc_dev.dv_xname);
}

static int
tx39power_button_intr_p(void *arg)
{
	config_hook_call(CONFIG_HOOK_BUTTONEVENT,
	    CONFIG_HOOK_BUTTONEVENT_POWER,
	    (void *)1 /* on */);

	return (0);
}

static int
tx39power_button_intr_n(void *arg)
{
	config_hook_call(CONFIG_HOOK_BUTTONEVENT,
	    CONFIG_HOOK_BUTTONEVENT_POWER,
	    (void *)0 /* off */);
	DUMP_REGS(arg);

	return (0);
}

int
tx39power_intr_p(void *arg)
{
	/* low priority event */
	printf("power_p\n");
	DUMP_REGS(arg);

	return (0);
}

static int
tx39power_intr_n(void *arg)
{
	/* low priority event */
	printf("power_n\n");
	DUMP_REGS(arg);

	return (0);
}

static int
tx39power_ok_intr_p(void *arg)
{
	/* high priority event */
	printf("power NG\n");
	DUMP_REGS(arg);
	config_hook_call(CONFIG_HOOK_PMEVENT,
	    CONFIG_HOOK_PMEVENT_SUSPENDREQ, NULL);

	return (0);
}

static int
tx39power_ok_intr_n(void *arg)
{
	/* high priority event */
	printf("power OK\n");
	DUMP_REGS(arg);

	return (0);
}

#ifdef TX39POWER_DEBUG
static void
__tx39power_dump (struct tx39power_softc *sc)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);
	ISSETPRINT(reg, ONBUTN);
	ISSETPRINT(reg, PWRINT);
	ISSETPRINT(reg, PWROK);
#ifdef TX392X
	ISSETPRINT(reg, PWROKNMI);
#endif /* TX392X */
	ISSETPRINT(reg, SLOWBUS);
#ifdef TX391X
	ISSETPRINT(reg, DIVMOD);
#endif /* TX391X */
	ISSETPRINT(reg, ENSTPTIMER);
	ISSETPRINT(reg, ENFORCESHUTDWN);
	ISSETPRINT(reg, FORCESHUTDWN); 
	ISSETPRINT(reg, FORCESHUTDWNOCC);
	ISSETPRINT(reg, SELC2MS);
#ifdef TX392X
	ISSETPRINT(reg, WARMSTART);
#endif /* TX392X */
	ISSETPRINT(reg, BPDBVCC3);        
	ISSETPRINT(reg, STOPCPU);       
	ISSETPRINT(reg, DBNCONBUTN);
	ISSETPRINT(reg, COLDSTART);     
	ISSETPRINT(reg, PWRCS);      
	ISSETPRINT(reg, VCCON);          
#ifdef TX391X
	printf("VIDRF=%d ", TX39_POWERCTRL_VIDRF(reg));
#endif /* TX391X */
	printf("STPTIMERVAL=%d ", TX39_POWERCTRL_STPTIMERVAL(reg));
	printf("\n");
}
#endif /* TX39POWER_DEBUG */
