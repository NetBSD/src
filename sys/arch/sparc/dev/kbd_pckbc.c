/*	$NetBSD: kbd_pckbc.c,v 1.3 2003/08/07 16:29:35 agc Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
 * All rights reserved.
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
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kbd_pckbc.c,v 1.3 2003/08/07 16:29:35 agc Exp $");

/*
 * Serve JavaStation-1 PS/2 keyboard as a Type5 keyboard with US101A
 * layout.  Since stock Xsun(1) knows this layout, JavaStation-1 gets
 * X for free.  When sparc port is switched to wscons and its X server
 * knows how to talk wskbd, this driver will no longer be necessary,
 * and we will be able to attach the keyboard with the MI pckbc(4) driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/select.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/sun/kbd_reg.h>
#include <dev/sun/kbio.h>

#include <dev/pckbc/pckbdreg.h>

#include <dev/ic/pckbcvar.h>

#include <dev/sun/event_var.h>
#include <dev/sun/kbd_xlate.h>
#include <dev/sun/kbdvar.h>

#define DPRINTF(args) /* printf args */


struct kbd_pckbc_softc {
	struct kbd_softc sc_kbd;

	/* pckbc attachment */
	pckbc_tag_t sc_kbctag;
	pckbc_slot_t sc_kbcslot;

	/*
	 * Middle layer data.
	 */ 
	int sc_isopen;
	int sc_pcleds;

	/* xt scan-codes decoding */
	int sc_lastchar;
	int sc_extended;
	int sc_extended1;
	
};

static int	kbd_pckbc_match(struct device *, struct cfdata *, void *);
static void	kbd_pckbc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(kbd_pckbc, sizeof(struct kbd_pckbc_softc),
    kbd_pckbc_match, kbd_pckbc_attach, NULL, NULL);


/*
 * Middle layer.
 */

/* callbacks for the upper /dev/kbd layer */
static int	kbd_pckbc_open(struct kbd_softc *);
static int	kbd_pckbc_close(struct kbd_softc *);
static int	kbd_pckbc_do_cmd(struct kbd_softc *, int, int);
static int	kbd_pckbc_set_leds(struct kbd_softc *, int, int);

static const struct kbd_ops kbd_ops_pckbc = {
	kbd_pckbc_open,
	kbd_pckbc_close,
	kbd_pckbc_do_cmd,
	kbd_pckbc_set_leds
};


static const u_int8_t	kbd_pckbc_xt_to_sun[];

static int	kbd_pckbc_set_xtscancode(pckbc_tag_t, pckbc_slot_t);
static void	kbd_pckbc_input(void *, int);
static int	kbd_pckbc_decode(struct kbd_pckbc_softc *, int, int *);


/*********************************************************************
 *			  Autoconfiguration
 */

int 
kbd_pckbc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void   *aux;
{
	struct pckbc_attach_args *pa = aux;

	if (pa->pa_slot != PCKBC_KBD_SLOT)
		return (0);

	return (1);
}


void
kbd_pckbc_attach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;

{
	struct kbd_pckbc_softc *sc = (void *)self;
	struct pckbc_attach_args *pa = aux;
	struct kbd_softc *kbd = &sc->sc_kbd;

	/* save our attachment to pckbc */
	sc->sc_kbctag = pa->pa_tag;
	sc->sc_kbcslot = pa->pa_slot;

	/* provide upper layer a link to our middle layer */
	kbd->k_ops = &kbd_ops_pckbc;

	/* pre-fill keyboard type/layout */
	kbd->k_state.kbd_id = KB_SUN4;	/* NB: type5 keyboards actually report type4 */
	kbd->k_state.kbd_layout = 19;	/* US101A */

	if (1) { /* XXX: pckbc_machdep_cnattach should tell us */
		/*
		 * Hookup ourselves as the console input channel
		 */
		extern void kd_attach_input(struct cons_channel *);
		struct cons_channel *cc;

		if ((cc = kbd_cc_alloc(kbd)) == NULL)
			return;

	        kd_attach_input(cc);	/* XXX ???? */
		kbd->k_isconsole = 1;
		printf(": console input");
	}

	printf("\n");

	kbd_pckbc_set_xtscancode(sc->sc_kbctag, sc->sc_kbcslot);

	/* slow down typematic (can't disable, grrr) */
	{
		u_char cmd[2];
		int res;

		cmd[0] = KBC_TYPEMATIC;
		cmd[1] = 0x7f;	/* 1s, 2/s */
		res = pckbc_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				     cmd, 2, 0, NULL, 0);
		if (res) {
			printf("%s: set typematic failed, error %d\n",
			       kbd->k_dev.dv_xname, res);
		}
	}

	/* register our callback with pckbc interrupt handler */
	pckbc_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot,
			       kbd_pckbc_input, sc,
			       kbd->k_dev.dv_xname);
}


static int
kbd_pckbc_set_xtscancode(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
{
	u_char cmd[2];
	int res;

	/*
	 * Some keyboard/8042 combinations do not seem to work if the keyboard
	 * is set to table 1; in fact, it would appear that some keyboards just
	 * ignore the command altogether.  So by default, we use the AT scan
	 * codes and have the 8042 translate them.  Unfortunately, this is
	 * known to not work on some PS/2 machines.  We try desparately to deal
	 * with this by checking the (lack of a) translate bit in the 8042 and
	 * attempting to set the keyboard to XT mode.  If this all fails, well,
	 * tough luck.
	 *
	 * XXX It would perhaps be a better choice to just use AT scan codes
	 * and not bother with this.
	 */
	if (pckbc_xt_translation(kbctag, kbcslot, 1)) {
		/* The 8042 is translating for us; use AT codes. */
		cmd[0] = KBC_SETTABLE;
		cmd[1] = 2;
		res = pckbc_poll_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
		if (res) {
			u_char cmd[1];
#ifdef DEBUG
			printf("pckbd: error setting scanset 2\n");
#endif
			/*
			 * XXX at least one keyboard is reported to lock up
			 * if a "set table" is attempted, thus the "reset".
			 * XXX ignore errors, scanset 2 should be
			 * default anyway.
			 */
			cmd[0] = KBC_RESET;
			(void)pckbc_poll_cmd(kbctag, kbcslot, cmd, 1, 1, 0, 1);
			pckbc_flush(kbctag, kbcslot);
			res = 0;
		}
	} else {
		/* Stupid 8042; set keyboard to XT codes. */
		cmd[0] = KBC_SETTABLE;
		cmd[1] = 1;
		res = pckbc_poll_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
#ifdef DEBUG
		if (res)
			printf("pckbd: error setting scanset 1\n");
#endif
	}
	return (res);
}


/*********************************************************************
 *			/dev/kbd middle layer
 */

/*
 * Initialization to be done at first open.
 * This is called from kbdopen() or kd_cc_open()
 * Called with user context.
 */
static int
kbd_pckbc_open(kbd)
	struct kbd_softc *kbd;
{
	struct kbd_pckbc_softc *sc = (struct kbd_pckbc_softc *)kbd;
	struct kbd_state *ks;
	int error = 0;

	if (kbd == NULL) {
		DPRINTF(("kbd_pckbc_open: kbd == NULL\n"));
		return (ENXIO);
	}

	ks = &kbd->k_state;

	/* tolerate extra calls */
	if (sc->sc_isopen)
		return (0);

	/* open internal device */

	/* reset the keyboard (and enable interrupts?) */

	/*
	 * Initialize the table pointers for this type/layout.
	 * NB: fixed type/layout were preset during attach.
	 */
	kbd_xlate_init(ks);

	if (error == 0)
		sc->sc_isopen = 1;
	return (error);
}


static int
kbd_pckbc_close(kbd)
	struct kbd_softc *kbd;
{
#if 0
	struct kbd_pckbc_softc *k = (struct kbd_pckbc_softc *)kbd;
#endif
	return (0);		/* nothing to do so far */
}


/*
 * Upper layer talks sun keyboard protocol to us.
 */
/* ARGSUSED2 */
static int
kbd_pckbc_do_cmd(kbd, suncmd, isioctl)
	struct kbd_softc *kbd;
	int suncmd;
	int isioctl;
{
	int error = 0;

	switch (suncmd) {

	case KBD_CMD_BELL:	/* FALLTHROUGH */
	case KBD_CMD_NOBELL:	/* FALLTHROUGH */
	case KBD_CMD_CLICK:	/* FALLTHROUGH */
	case KBD_CMD_NOCLICK:
		/* not supported, do nothing */
		DPRINTF(("%s: ignoring KIOCCMD 0x%02x\n",
			 kbd->k_dev.dv_xname, suncmd));
		break;

	default:
		error = EINVAL;
		break;
	}

	return (error);
}

/* ARGSUSED2 */
static int
kbd_pckbc_set_leds(kbd, sunleds, isioctl)
	struct kbd_softc *kbd;
	int sunleds;
	int isioctl;
{
	struct kbd_pckbc_softc *sc = (struct kbd_pckbc_softc *)kbd;
	u_char pcleds;
	u_char cmd[2];
	int res;

	/* re-encode sun leds into pckbd leds */
	/* pckbd: 0 - scroll; 1 - num; 2 - caps */
	pcleds = 0;
	if (sunleds & LED_SCROLL_LOCK)
		pcleds |= 0x01;
	if (sunleds & LED_NUM_LOCK)
		pcleds |= 0x02;
	if (sunleds & LED_CAPS_LOCK)
		pcleds |= 0x04;

	if (pcleds == sc->sc_pcleds)
		return (0);
	sc->sc_pcleds = pcleds;
	DPRINTF(("leds: sun %x, pc %x\n", sunleds, pcleds));

	/* request the change */
	cmd[0] = KBC_MODEIND;
	cmd[1] = pcleds;
	if (isioctl) /* user called KIOCSLED */
	    res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				    cmd, 2, 0, 1, NULL);
	else /* console updates leds - called from interrupt handler */
	    res = pckbc_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				 cmd, 2, 0, NULL, 0);
	return (res);
}


/*********************************************************************
 *			 /dev/kbd lower layer
 */

/*
 * Got a receive interrupt - pckbc wants to give us a byte.
 */
static void
kbd_pckbc_input(vsc, data)
	void *vsc;
	int data;
{
	struct kbd_pckbc_softc *sc = vsc;
	struct kbd_softc *kbd = &sc->sc_kbd;
	int sunkey;

	/* convert to sun make/break code */
	if (!kbd_pckbc_decode(sc, data, &sunkey))
		return;

	kbd_input(kbd, sunkey);
}


/*
 * Plagiarized from pckbd_decode
 */
static int
kbd_pckbc_decode(sc, data, sundata)
    	struct kbd_pckbc_softc *sc;
	int data;
	int *sundata;
{
	int key, up;
	int sunkey;

	/* XXX: DEBUG*/
	DPRINTF(("%s: %02x", sc->sc_kbd.k_dev.dv_xname, data));

	if (data == KBR_EXTENDED0) {
		sc->sc_extended = 1;
		DPRINTF(("\n"));
		return (0);
	} else if (data == KBR_EXTENDED1) {
		sc->sc_extended1 = 2;
		DPRINTF(("\n"));
		return (0);
	}

 	/* map extended keys to (unused) codes 128-254 */
	key = (data & 0x7f) | (sc->sc_extended ? 0x80 : 0);
	sc->sc_extended = 0;

	/*
	 * process BREAK key (EXT1 1D 45  EXT1 9D C5):
	 * map to (unused) code 7F
	 */
	if (sc->sc_extended1 == 2 && (data == 0x1d || data == 0x9d)) {
		sc->sc_extended1 = 1;
		DPRINTF(("\n"));
		return (0);
	} else if (sc->sc_extended1 == 1
		   && (data == 0x45 || data == 0xc5)) {
		sc->sc_extended1 = 0;
		key = 0x7f;
	} else if (sc->sc_extended1 > 0) {
		sc->sc_extended1 = 0;
	}

	if (data & 0x80) {
		sc->sc_lastchar = 0;
		up = 1;
	} else {
		/* always ignore typematic keys */
		if (key == sc->sc_lastchar) {
			DPRINTF(("\n"));
			return (0);
		}
		sc->sc_lastchar = key;
		up = 0;
	}

	
	sunkey = kbd_pckbc_xt_to_sun[key];

	DPRINTF((" -> xt 0x%02x %s -> %d\n",
		 key, (up ? "up  " : "down"),  sunkey));

	if (up)
		sunkey |= KBD_UP;

	*sundata = sunkey;
	return (1);
}

static const u_int8_t kbd_pckbc_xt_to_sun[256] = {
/* 0x00 */   0,	/*             */
/* 0x01 */  29,	/* Esc         */
/* 0x02 */  30,	/* 1           */
/* 0x03 */  31,	/* 2           */
/* 0x04 */  32,	/* 3           */
/* 0x05 */  33,	/* 4           */
/* 0x06 */  34,	/* 5           */
/* 0x07 */  35,	/* 6           */
/* 0x08 */  36,	/* 7           */
/* 0x09 */  37,	/* 8           */
/* 0x0a */  38,	/* 9           */
/* 0x0b */  39,	/* 0           */
/* 0x0c */  40,	/* minus       */
/* 0x0d */  41,	/* equal       */
/* 0x0e */  43,	/* BackSpace   */
/* 0x0f */  53,	/* Tab         */
/* 0x10 */  54,	/* Q           */
/* 0x11 */  55,	/* W           */
/* 0x12 */  56,	/* E           */
/* 0x13 */  57,	/* R           */
/* 0x14 */  58,	/* T           */
/* 0x15 */  59,	/* Y           */
/* 0x16 */  60,	/* U           */
/* 0x17 */  61,	/* I           */
/* 0x18 */  62,	/* O           */
/* 0x19 */  63,	/* P           */
/* 0x1a */  64,	/* [           */
/* 0x1b */  65,	/* ]           */
/* 0x1c */  89,	/* Return      */
/* 0x1d */  76,	/* Ctrl_L      */
/* 0x1e */  77,	/* A           */
/* 0x1f */  78,	/* S           */
/* 0x20 */  79,	/* D           */
/* 0x21 */  80,	/* F           */
/* 0x22 */  81,	/* G           */
/* 0x23 */  82,	/* H           */
/* 0x24 */  83,	/* J           */
/* 0x25 */  84,	/* K           */
/* 0x26 */  85,	/* L           */
/* 0x27 */  86,	/* ;           */
/* 0x28 */  87,	/* apostr.     */
/* 0x29 */  42,	/* grave/tilde */
/* 0x2a */  99,	/* Shift_L     */
/* 0x2b */  88,	/* backslash   */
/* 0x2c */ 100,	/* Z           */
/* 0x2d */ 101,	/* X           */
/* 0x2e */ 102,	/* C           */
/* 0x2f */ 103,	/* V           */
/* 0x30 */ 104,	/* B           */
/* 0x31 */ 105,	/* N           */
/* 0x32 */ 106,	/* M           */
/* 0x33 */ 107,	/* ,           */
/* 0x34 */ 108,	/* .           */
/* 0x35 */ 109,	/* /           */
/* 0x36 */ 110,	/* Shift_R     */
/* 0x37 */  47,	/* R6/KP_Mult  */
/* 0x38 */ 120,	/* Meta_L      */
/* 0x39 */ 121,	/* SpaceBar    */
/* 0x3a */ 119,	/* CapsLock    */
/* 0x3b */   5,	/* F1          */
/* 0x3c */   6,	/* F2          */
/* 0x3d */   8,	/* F3          */
/* 0x3e */  10,	/* F4          */
/* 0x3f */  12,	/* F5          */
/* 0x40 */  14,	/* F6          */
/* 0x41 */  16,	/* F7          */
/* 0x42 */  17,	/* F8          */
/* 0x43 */  18,	/* F9          */
/* 0x44 */   7,	/* F10         */
/* 0x45 */  98,	/* Num_Lock    */
/* 0x46 */   0,	/*             */	/* scroll lock */
/* 0x47 */  68,	/* R7/Home     */
/* 0x48 */  69,	/* R8/Up       */
/* 0x49 */  70,	/* R9/PgUp     */
/* 0x4a */  71,	/* KP_Minus    */
/* 0x4b */  91,	/* R10/Left    */
/* 0x4c */  92,	/* R11/KP_5    */
/* 0x4d */  93,	/* R12/Right   */
/* 0x4e */ 125,	/* KP_Add      */
/* 0x4f */ 112,	/* R13/End     */
/* 0x50 */ 113,	/* R14/Down    */
/* 0x51 */ 114,	/* R15/PgDn    */
/* 0x52 */  94,	/* KP_Insert   */
/* 0x53 */  50,	/* KP_Delete   */
/* 0x54 */   0,	/*             */
/* 0x55 */   0,	/*             */
/* 0x56 */   0,	/*             */
/* 0x57 */   9,	/* F11         */
/* 0x58 */  11,	/* F12         */
/* 0x59 */   0,	/*             */
/* 0x5a */   0,	/*             */
/* 0x5b */   0,	/*             */
/* 0x5c */   0,	/*             */
/* 0x5d */   0,	/*             */
/* 0x5e */   0,	/*             */
/* 0x5f */   0,	/*             */
/* 0x60 */   0,	/*             */
/* 0x61 */   0,	/*             */
/* 0x62 */   0,	/*             */
/* 0x63 */   0,	/*             */
/* 0x64 */   0,	/*             */
/* 0x65 */   0,	/*             */
/* 0x66 */   0,	/*             */
/* 0x67 */   0,	/*             */
/* 0x68 */   0,	/*             */
/* 0x69 */   0,	/*             */
/* 0x6a */   0,	/*             */
/* 0x6b */   0,	/*             */
/* 0x6c */   0,	/*             */
/* 0x6d */   0,	/*             */
/* 0x6e */   0,	/*             */
/* 0x6f */   0,	/*             */
/* 0x70 */   0,	/*             */
/* 0x71 */   0,	/*             */
/* 0x72 */   0,	/*             */
/* 0x73 */   0,	/*             */
/* 0x74 */   0,	/*             */
/* 0x75 */   0,	/*             */
/* 0x76 */   0,	/*             */
/* 0x77 */   0,	/*             */
/* 0x78 */   0,	/*             */
/* 0x79 */   0,	/*             */
/* 0x7a */   0,	/*             */
/* 0x7b */   0,	/*             */
/* 0x7c */   0,	/*             */
/* 0x7d */   0,	/*             */
/* 0x7e */   0,	/*             */
/* 0x7f */  23,	/* R3/Break    */

/* 0x80 */   0,	/*             */
/* 0x81 */   0,	/*             */
/* 0x82 */   0,	/*             */
/* 0x83 */   0,	/*             */
/* 0x84 */   0,	/*             */
/* 0x85 */   0,	/*             */
/* 0x86 */   0,	/*             */
/* 0x87 */   0,	/*             */
/* 0x88 */   0,	/*             */
/* 0x89 */   0,	/*             */
/* 0x8a */   0,	/*             */
/* 0x8b */   0,	/*             */
/* 0x8c */   0,	/*             */
/* 0x8d */   0,	/*             */
/* 0x8e */   0,	/*             */
/* 0x8f */   0,	/*             */
/* 0x90 */   0,	/*             */
/* 0x91 */   0,	/*             */
/* 0x92 */   0,	/*             */
/* 0x93 */   0,	/*             */
/* 0x94 */   0,	/*             */
/* 0x95 */   0,	/*             */
/* 0x96 */   0,	/*             */
/* 0x97 */   0,	/*             */
/* 0x98 */   0,	/*             */
/* 0x99 */   0,	/*             */
/* 0x9a */   0,	/*             */
/* 0x9b */   0,	/*             */
/* 0x9c */  90,	/* KP_Enter    */
/* 0x9d */  76,	/* Ctrl_L      */	/* XXX: Sun kbd has no Ctrl_R */
/* 0x9e */   0,	/*             */
/* 0x9f */   0,	/*             */
/* 0xa0 */   0,	/*             */
/* 0xa1 */   0,	/*             */
/* 0xa2 */   0,	/*             */
/* 0xa3 */   0,	/*             */
/* 0xa4 */   0,	/*             */
/* 0xa5 */   0,	/*             */
/* 0xa6 */   0,	/*             */
/* 0xa7 */   0,	/*             */
/* 0xa8 */   0,	/*             */
/* 0xa9 */   0,	/*             */
/* 0xaa */   0,	/*             */
/* 0xab */   0,	/*             */
/* 0xac */   0,	/*             */
/* 0xad */   0,	/*             */
/* 0xae */   0,	/*             */
/* 0xaf */   0,	/*             */
/* 0xb0 */   0,	/*             */
/* 0xb1 */   0,	/*             */
/* 0xb2 */   0,	/*             */
/* 0xb3 */   0,	/*             */
/* 0xb4 */   0,	/*             */
/* 0xb5 */  46,	/* R5/KP_Div   */
/* 0xb6 */   0,	/*             */
/* 0xb7 */   0,	/*             */
/* 0xb8 */ 122,	/* Meta_R      */
/* 0xb9 */   0,	/*             */
/* 0xba */   0,	/*             */
/* 0xbb */   0,	/*             */
/* 0xbc */   0,	/*             */
/* 0xbd */   0,	/*             */
/* 0xbe */   0,	/*             */
/* 0xbf */   0,	/*             */
/* 0xc0 */   0,	/*             */
/* 0xc1 */   0,	/*             */
/* 0xc2 */   0,	/*             */
/* 0xc3 */   0,	/*             */
/* 0xc4 */   0,	/*             */
/* 0xc5 */   0,	/*             */
/* 0xc6 */   0,	/*             */
/* 0xc7 */  52,	/* - T5_Home   */
/* 0xc8 */  20,	/* T5_Up       */
/* 0xc9 */  96,	/* - T5_PgUp   */
/* 0xca */   0,	/*             */
/* 0xcb */  24,	/* T5_Left     */
/* 0xcc */   0,	/*             */
/* 0xcd */  28,	/* T5_Right    */
/* 0xce */   0,	/*             */
/* 0xcf */  74,	/* - T5_End    */
/* 0xd0 */  27,	/* T5_Down     */
/* 0xd1 */ 123,	/* - T5_PgDn   */
/* 0xd2 */  44,	/* - T5_Insert */
/* 0xd3 */  66,	/* Delete      */
/* 0xd4 */   0,	/*             */
/* 0xd5 */   0,	/*             */
/* 0xd6 */   0,	/*             */
/* 0xd7 */   0,	/*             */
/* 0xd8 */   0,	/*             */
/* 0xd9 */   0,	/*             */
/* 0xda */   0,	/*             */
/* 0xdb */  19,	/* Alt_L       */	/* left flying window */
/* 0xdc */   0,	/*             */	/* right flying window */
/* 0xdd */   0,	/*             */	/* (right) menu key */
/* 0xde */   0,	/*             */
/* 0xdf */   0,	/*             */
/* 0xe0 */   0,	/*             */
/* 0xe1 */   0,	/*             */
/* 0xe2 */   0,	/*             */
/* 0xe3 */   0,	/*             */
/* 0xe4 */   0,	/*             */
/* 0xe5 */   0,	/*             */
/* 0xe6 */   0,	/*             */
/* 0xe7 */   0,	/*             */
/* 0xe8 */   0,	/*             */
/* 0xe9 */   0,	/*             */
/* 0xea */   0,	/*             */
/* 0xeb */   0,	/*             */
/* 0xec */   0,	/*             */
/* 0xed */   0,	/*             */
/* 0xee */   0,	/*             */
/* 0xef */   0,	/*             */
/* 0xf0 */   0,	/*             */
/* 0xf1 */   0,	/*             */
/* 0xf2 */   0,	/*             */
/* 0xf3 */   0,	/*             */
/* 0xf4 */   0,	/*             */
/* 0xf5 */   0,	/*             */
/* 0xf6 */   0,	/*             */
/* 0xf7 */   0,	/*             */
/* 0xf8 */   0,	/*             */
/* 0xf9 */   0,	/*             */
/* 0xfa */   0,	/*             */
/* 0xfb */   0,	/*             */
/* 0xfc */   0,	/*             */
/* 0xfd */   0,	/*             */
/* 0xfe */   0,	/*             */
/* 0xff */   0	/*             */
}; /* kbd_pckbc_xt_to_sun */
