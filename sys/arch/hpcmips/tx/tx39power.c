/*	$NetBSD: tx39power.c,v 1.6 2000/05/22 17:17:44 uch Exp $ */

/*-
 * Copyright (c) 1999, 2000 UCHIYAMA Yasushi.  All rights reserved.
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

#include "opt_tx39_debug.h"
#include "opt_tx39powerdebug.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39powerreg.h>
#include <hpcmips/tx/tx39spireg.h>

#include <hpcmips/dev/video_subr.h>

#undef POWERBUTTON_IS_DEBUGGER

#define ISSET(x, v)	((x) & (v))
#define ISSETPRINT(r, m) __is_set_print(r, TX39_POWERCTRL_##m, #m)

int	tx39power_match __P((struct device*, struct cfdata*, void*));
void	tx39power_attach __P((struct device*, struct device*, void*));

int	tx39power_intr __P((void*));
int	tx39power_ok_intr __P((void*));
int	tx39power_button_intr __P((void*));

struct tx39power_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
};

struct cfattach tx39power_ca = {
	sizeof(struct tx39power_softc), tx39power_match, tx39power_attach
};

int
tx39power_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group of txsim */
}

void
tx39power_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39power_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	txreg_t reg;
	
	tc = sc->sc_tc = ta->ta_tc;
	tx_conf_register_power(tc, self);

	printf("\n");
#ifdef TX39POWERDEBUG
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
#endif /* TX39POWERDEBUG */
#ifdef DISABLE_SPI_AND_DOZE /* XXX test XXX */
	/* 
	 *	Disable SPI module 
	 */
	reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	if (ISSET(reg, TX39_SPICTRL_ENSPI)) {
		reg &= ~TX39_SPICTRL_ENSPI;
	}
	printf("SPI module disabled\n");

	/* 
	 *	Disable Stop timer (Doze CPU mode)
	 */
	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);
	printf("STPTIMER disabled.\n");
	reg &= ~TX39_POWERCTRL_ENSTPTIMER;
	tx_conf_write(tc, TX39_POWERCTRL_REG, reg);
#endif /* DISABLE_SPI_AND_DOZE */

	/*
	 * enable stop timer
	 */
	reg = tx_conf_read(tc, TX39_POWERCTRL_REG);

	reg &= ~(TX39_POWERCTRL_STPTIMERVAL_MASK <<
		 TX39_POWERCTRL_STPTIMERVAL_SHIFT);
	reg = TX39_POWERCTRL_STPTIMERVAL_SET(reg, 1);

	reg |= TX39_POWERCTRL_ENSTPTIMER;
	tx_conf_write(tc, TX39_POWERCTRL_REG, reg);

	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSPWRINT),
			    IST_EDGE, IPL_CLOCK, 
			    tx39power_intr, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGPWRINT),
			    IST_EDGE, IPL_CLOCK,			    
			    tx39power_intr, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSPWROKINT),
			    IST_EDGE, IPL_CLOCK, 
			    tx39power_ok_intr, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGPWROKINT),
			    IST_EDGE, IPL_CLOCK,			    
			    tx39power_ok_intr, sc);
#if 0
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSONBUTNINT),
			    IST_EDGE, IPL_CLOCK,
			    tx39power_button_intr, sc);
#endif
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGONBUTNINT),
			    IST_EDGE, IPL_CLOCK,			    
			    tx39power_button_intr, sc);

}

int
tx39power_button_intr(arg)
	void *arg;
{
	struct tx39power_softc *sc = arg;
	
	if (sc->sc_tc->tc_videot)
		video_calibration_pattern(sc->sc_tc->tc_videot); /* debug */

#if defined DDB && defined POWERBUTTON_IS_DEBUGGER
	cpu_Debugger();
#endif	
	return 0;
}

int
tx39power_intr(arg)
	void *arg;
{
	printf("power\n");
	return 0;
}

int
tx39power_ok_intr(arg)
	void *arg;
{
	printf("power NG\n");
	return 0;
}
