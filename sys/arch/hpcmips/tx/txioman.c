/*	$NetBSD: txioman.c,v 1.1 2000/01/16 21:47:00 uch Exp $ */

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39iovar.h>

#include <hpcmips/tx/txiomanvar.h>
#include <hpcmips/tx/txsnd.h>

#include "locators.h"

int	txioman_match __P((struct device*, struct cfdata*, void*));
void	txioman_attach __P((struct device*, struct device*, void*));

struct txioman_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
};

struct cfattach txioman_ca = {
	sizeof(struct txioman_softc), txioman_match, txioman_attach
};

void	__txioman_led		__P((txioman_tag_t, int, int));
void	__txioman_backlight	__P((txioman_tag_t, int));
void	__txioman_uart_init	__P((txioman_tag_t));
void	__txioman_uarta_init	__P((txioman_tag_t, void*));

struct txioman_tag txioman_null_tag = {
	NULL,
	__txioman_led,
	__txioman_backlight,
	__txioman_uart_init,
	__txioman_uarta_init,
};

void	__mobilon_backlight	__P((txioman_tag_t, int));
void	__mobilon_uart_init	__P((txioman_tag_t));
void	__mobilon_uarta_init	__P((txioman_tag_t, void*));

struct txioman_tag txioman_mobilon_tag = {
	NULL,
	__txioman_led,
	__mobilon_backlight,
	__mobilon_uart_init,
	__mobilon_uarta_init,
};

void	__compaq_led		__P((txioman_tag_t, int, int));
void	__compaq_uarta_init	__P((txioman_tag_t, void*));

struct txioman_tag txioman_compaq_tag = {
	NULL,
	__compaq_led,
	__txioman_backlight,
	__txioman_uart_init,
	__compaq_uarta_init,
};

const struct txioman_platform_table {
	platid_t	tp_platform;
	char*		tp_name;
	txioman_tag_t	tp_tag;
} txioman_platform_table[] = {
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C}},
	 "Compaq-C",
	 &txioman_compaq_tag},

	{{{PLATID_WILD, PLATID_MACH_SHARP_MOBILON}},
	 "Mobilon HC",
	 &txioman_mobilon_tag},

	{{{0, 0}}, "", &txioman_null_tag}
};

txioman_tag_t	txioman_tag_lookup __P((void));

int	__config_hook_backlight __P((void*, int, long, void*));

int
txioman_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	platid_mask_t mask;

	if (cf->cf_loc[TXIOMANIFCF_PLATFORM] == 
	    TXIOMANIFCF_PLATFORM_DEFAULT) {
		return 1;
	}

	mask = PLATID_DEREF(cf->cf_loc[TXIOMANIFCF_PLATFORM]);
	if (platid_match(&platid, &mask)) {
		return 2;
	}

	return 0;
}

void
txioman_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txioman_attach_args *tia = aux;
	struct txioman_softc *sc = (void*)self;
	txioman_tag_t tag;

	sc->sc_tc = tia->tia_tc;

	tag = txioman_tag_lookup();
	tag->ti_v = sc;

	/*
	 * register myself to tx_chipset.
	 */
	tx_conf_register_ioman(sc->sc_tc, tag);

	/*
	 * register backlight config_hook if any.
	 */
	config_hook(CONFIG_HOOK_BUTTONEVENT,
		    CONFIG_HOOK_BUTTONEVENT_LIGHT, 
		    CONFIG_HOOK_SHARE, /* btnmgr */
		    __config_hook_backlight, sc->sc_tc);

}

txioman_tag_t
txioman_tag_lookup()
{
	const struct txioman_platform_table *tab;
	platid_mask_t mask;
	
	for (tab = txioman_platform_table; 
	     tab->tp_tag != &txioman_null_tag; tab++) {
		
		mask = PLATID_DEREF(&tab->tp_platform);
		
		if (platid_match(&platid, &mask))
			goto out;
	}
out:	
	printf(": %s\n", tab->tp_name);

	return tab->tp_tag;
}

/*
 * default functions.
 */
void	
__txioman_led(ti, type, onoff)
	txioman_tag_t ti;
	int type, onoff;
{
}

void	
__txioman_backlight(ti, onoff)
	txioman_tag_t ti;
	int onoff;
{
}

void	
__txioman_uart_init(ti)
	txioman_tag_t ti;
{
}

void	
__txioman_uarta_init(ti, cookie)
	txioman_tag_t ti;
	void *cookie;
{
}

/*
 * Compaq-C functions.
 */
void	
__compaq_led(ti, type, onoff)
	txioman_tag_t ti;
	int type, onoff;
{
	struct txioman_softc *sc = (void*)ti;

	/* Green LED */
	tx39io_portout(sc->sc_tc, TXPORT(TXMFIO, 3), 
		       onoff ? TXOFF : TXON);
}

extern int	__compaq_uart_dcd	__P((void*));
extern int	__mobilon_uart_dcd	__P((void*));

void	
__compaq_uarta_init(ti, cookie)
	txioman_tag_t ti;
	void *cookie;
{
	struct txioman_softc *sc = (void*)ti;
	tx_chipset_tag_t tc = sc->sc_tc;

	tx_intr_establish(tc, MAKEINTR(3, (1<<30)), IST_EDGE, IPL_TTY,
			  __compaq_uart_dcd, cookie);	
	tx_intr_establish(tc, MAKEINTR(4, (1<<30)), IST_EDGE, IPL_TTY,
			  __compaq_uart_dcd, cookie);	
	tx_intr_establish(tc, MAKEINTR(3, (1<<5)), IST_EDGE, IPL_TTY,
			  __compaq_uart_dcd, cookie);	
	tx_intr_establish(tc, MAKEINTR(4, (1<<5)), IST_EDGE, IPL_TTY,
			  __compaq_uart_dcd, cookie);
}

/*
 * Mobilon HC functions
 */
void	
__mobilon_backlight(ti, onoff)
	txioman_tag_t ti;
	int onoff;
{
	struct txioman_softc *sc = (void*)ti;

	tx39io_portout(sc->sc_tc, TXPORT(TXMFIO, 14), 
		       onoff ? TXON : TXOFF);
}

void	
__mobilon_uart_init(ti)
	txioman_tag_t ti;
{
	struct txioman_softc *sc = (void*)ti;
	
	tx39io_portout(sc->sc_tc, TXPORT(TXIO, 5), TXON);
	tx39io_portout(sc->sc_tc, TXPORT(TXMFIO, 15), TXON);
}

void	
__mobilon_uarta_init(ti, cookie)
	txioman_tag_t ti;
	void *cookie;
{
#if not_required /* ??? this is harmful... */
	struct txioman_softc *sc = (void*)ti;
	tx_chipset_tag_t tc = sc->sc_tc;

	tx_intr_establish(tc, MAKEINTR(5, (1<<4)), IST_EDGE, IPL_TTY,
			  __mobilon_uart_dcd, cookie);	
	tx_intr_establish(tc, MAKEINTR(5, (1<<11)), IST_EDGE, IPL_TTY,
			  __mobilon_uart_dcd, cookie);	
	tx_intr_establish(tc, MAKEINTR(5, (1<<6)), IST_EDGE, IPL_TTY,
			  __mobilon_uart_dcd, cookie);	
	tx_intr_establish(tc, MAKEINTR(5, (1<<13)), IST_EDGE, IPL_TTY,
			  __mobilon_uart_dcd, cookie);
#endif
}

/*
 * config_hook.
 */
int
__config_hook_backlight(arg, type, id, msg)
	void*	arg;
	int	type;
	long	id;
	void*	msg;
{
	static int onoff; /* XXX */
	tx_chipset_tag_t tc = arg;
	
	onoff ^= 1;

	txioman_backlight(tc, onoff);
	txioman_led(tc, 0, onoff); /* test */
	tx_sound_mute(tc, !onoff); /* test */

	return 0;
}
