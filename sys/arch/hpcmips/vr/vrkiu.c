/*	$NetBSD: vrkiu.c,v 1.11 2000/01/17 12:22:37 shin Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi All rights reserved.
 * Copyright (c) 1999 TAKEMRUA, Shin All rights reserved.
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

#define VRKIUDEBUG

#include <sys/param.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrkiuvar.h>
#include <hpcmips/vr/vrkiureg.h>
#include <hpcmips/vr/icureg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbc/wskbdmap_mfii.h>

#include "opt_pckbd_layout.h"

#ifdef VRKIUDEBUG
int vrkiu_debug = 0;
#define DPRINTF(arg) if (vrkiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

/*
 * structure and data types
 */
struct vrkiu_chip {
	bus_space_tag_t kc_iot;
	bus_space_handle_t kc_ioh;
	unsigned short kc_scandata[KIU_NSCANLINE/2];
	int kc_polling;
	u_int kc_type;
	int kc_data;

	int kc_sft:1, kc_alt:1, kc_ctrl:1;

	struct vrkiu_softc* kc_sc;	/* back link */
};

struct vrkiu_softc {
	struct device sc_dev;
	struct vrkiu_chip *sc_chip;
	struct vrkiu_chip sc_chip_body;
	int sc_enabled;
	struct device *sc_wskbddev;

	void *sc_handler;
#define NKEYBUF 32
	unsigned char keybuf[NKEYBUF];
	int keybufhead, keybuftail;
};

/*
 * function prototypes
 */
static int vrkiumatch __P((struct device *, struct cfdata *, void *));
static void vrkiuattach __P((struct device *, struct device *, void *));

int vrkiu_intr __P((void *));

static int vrkiu_init(struct vrkiu_chip*, bus_space_tag_t, bus_space_handle_t);
static void vrkiu_write __P((struct vrkiu_chip *, int, unsigned short));
static unsigned short vrkiu_read __P((struct vrkiu_chip *, int));
static int vrkiu_is_console(bus_space_tag_t, bus_space_handle_t);
static void detect_key __P((struct vrkiu_chip *));

static struct vrkiu_softc *the_vrkiu = NULL; /* XXX: kludge!! */

/* wskbd accessopts */
int vrkiu_enable __P((void *, int));
void vrkiu_set_leds __P((void *, int));
int vrkiu_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

/* consopts */
void vrkiu_cngetc __P((void*, u_int*, int*));
void vrkiu_cnpollc __P((void *, int));

/*
 * global/static data
 */
struct cfattach vrkiu_ca = {
	sizeof(struct vrkiu_softc), vrkiumatch, vrkiuattach
};

const struct wskbd_accessops vrkiu_accessops = {
	vrkiu_enable,
	vrkiu_set_leds,
	vrkiu_ioctl,
};

const struct wskbd_consops vrkiu_consops = {
	vrkiu_cngetc,
	vrkiu_cnpollc,
};

struct wskbd_mapdata vrkiu_keymapdata = {
	pckbd_keydesctab,
#ifdef PCKBD_LAYOUT
	PCKBD_LAYOUT,
#else
	KB_US,
#endif
};

struct vrkiu_chip *vrkiu_consdata = NULL;

#define UNK	-1	/* unknown */
#define IGN	-2	/* ignore */

static char default_keytrans[] = {
/*00*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*08*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*10*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*18*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*20*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*28*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*30*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*38*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*40*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*48*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*50*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
/*58*/ UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,  UNK,	/* - - - - - - - - */
};

/* NEC MobileGearII MCR series (Japan) */
static char mcr_jp_keytrans[] = {
/*00*/  77,  28,  25,  52,  21,  48,  44,  57, /* right ent p . y b z space */
/*08*/  80,  53,  24,  51,  20,  47,  30, 123, /* down / o , t v a nfer */
/*10*/  75, 115,  23,  50,  19,  46,  17, 221, /* left \ i m r c w menu */
/*18*/  13, IGN,  22, IGN,  18,  45,  16,   2, /* ^ - u - e x q 1 */
/*20*/  81,  41,  11,  38,  40,  34,  15,  59, /* pgdn h/z 0 l : g tab f1 */
/*28*/ 121,  39,  10,  49,   6,  33,   3,  37, /* xfer ; 9 n 5 f 2 k */
/*30*/  72,  27,   9,  36,   5,  32,   7, IGN, /* up [ 8 j 4 d 6 - */
/*38*/  12,  26,   8,  35,   4,  43,  31, IGN, /* - @ 7 h 3 ] s - */
/*40*/  58, IGN, IGN, IGN,  14, IGN,  66,  61, /* caps - - - bs - f8 f3 */
/*48*/ IGN,  56, IGN, IGN, 125, 112,  65,  62, /* - alt - - | k/h f7 f4 */
/*50*/ IGN, IGN,  29, IGN,  68,  73,  64,  60, /* - - ctrl - f10 pgup f6 f2 */
/*58*/ IGN, IGN, IGN,  42,  14,  67,  63,   1, /* - - - shift del f9 f5 esc */
};

/* IBM WorkPad z50 */
static char z50_keytrans[] = {
/*00*/  59,  61,  63,  65,  67, IGN, IGN,  87,	/* f1 f3 f5 f7 f9 - - f11 */
/*08*/  60,  62,  64,  66,  68, IGN, IGN,  88,	/* f2 f4 f6 f8 f10 - - f12 */
/*10*/  40,  26,  12,  11,  25,  39,  72,  53,	/* ' [ - 0 p ; up / */
/*18*/ IGN, IGN, IGN,  10,  24,  38,  52, IGN,	/* - - - 9 o l . - */
/*20*/  75,  27,  13,   9,  23,  37,  51, IGN,	/* left ] = 8 i k , - */
/*28*/  35,  21,   7,   8,  22,  36,  50,  49,	/* h y 6 7 u j m n */
/*30*/ IGN,  14,  69,  14, IGN,  43,  28,  57,	/* - bs num del - \ ent sp */
/*38*/  34,  20,   6,   5,  19,  33,  47,  48,	/* g t 5 4 r f v b */
/*40*/ IGN, IGN, IGN,   4,  18,  32,  46,  77,	/* - - - 3 e d c right */
/*48*/ IGN, IGN, IGN,   3,  17,  31,  45,  80,	/* - - - 2 w s x down */
/*50*/   1,  29,  41,   2,  16,  30,  44, IGN,	/* esc tab ~ 1 q a z - */
/*58*/ 221,  42,  29,  29,  56,  56,  54, IGN,	/* menu Ls Lc Rc La Ra Rs - */
};

/* Sharp Tripad PV6000 */
static char tripad_keytrans[] = {
/*00*/  42,  15,  41,  16,   1,   2, 104, 221,	/* lsh tab ` q esc 1 WIN - */
/*08*/  58,  44,  45,  30,  31,  17,  18,   3,	/* ctrl z x a s w e 2 */
/*10*/  56,  57,  46,  47,  32,  33,  19,   4,	/* lalt sp c v d f r 3 */
/*18*/  48,  49,  34,  35,  20,  21,   5,   6,	/* b n g h t y 4 5 */
/*20*/  50,  51,  36,  37,  22,  23,   7,   8,	/* m , j k u i 6 7 */
/*28*/ 105,  29,  38,  24,  25,   9,  10,  11,	/* Fn caps l o p 8 9 0 */
/*30*/  26,  27, 102,  52,  53,  39,  12,  13,	/* [ ] dar , / ; \- = */
/*38*/  54, 103, 100, 102,  39,  28,  43,  14,	/* rsh - - uar - ; ent \ del */
/*40*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,	/* - - - - - - - - */
/*48*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,	/* - - - - - - - - */
/*50*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,	/* - - - - - - - - */
/*58*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,	/* - - - - - - - - */
};

/* NEC Mobile Gear MCCS series */
static char mccs_keytrans[] = {
/*00*/  58,  28, 102,  25,  52,  21,  48,  44,  /* caps cr rar p . y b z */
/*08*/  56,  27, 103,  24,  51,  20,  47,  30,  /* alt [ dar o , t v a */
/*10*/  41,  26, 101,  23,  50,  19,  46,  17,  /* zen @ lar i m r c w */
/*18*/  29,  39, 100,  22,  49,  18,  45,  16,  /* lctrl ; uar u n e x q */
/*20*/  42,  14, 115,  11,  38,   7,  34,  15,  /* lshft bs \ 0 l 6 g tab */
/*28*/ 123, 125,  53,  10,  37,   6,  33,   3,  /* nconv | / 9 k 5 f 2 */
/*30*/ 121,  13,  43,   9,  36,   5,  32,   2,  /* conv = ] 8 j 4 d 1 */
/*38*/ 112,  12,  40,   8,  35,   4,  31,   1,  /* hira - ' 7 h 3 s esc */
/*40*/ IGN,  57, IGN, IGN, IGN, IGN, IGN, IGN,  /* - sp - - - - - - */
/*48*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,  /* - - - - - - - - */
/*50*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,  /* - - - - - - - - */
/*58*/ IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,  /* - - - - - - - - */
};

static char mobilepro_keytrans[] = {
/*00*/  57,  27,  43,  53,  75,  80,  28,  38,  /* space ] \ / - - enter l */
/*08*/ IGN,  26,  40,  39,  77,  72,  52,  24,  /* - [ ' ; - - . o */
/*10*/ IGN, IGN, IGN, 221,  47,  46,  45,  44,  /* - - - Windows v c x z */
/*18*/ IGN,  13,  12,  41,  33,  32,  31,  30,  /* - = \- ` f d s a */
/*20*/   9,   8,   7,   6,  19,  18,  17,  16,  /* 8 7 6 5 r e w q */
/*28*/  51,  50,  49,  48, IGN, IGN,  11,  10,  /* , m n b - - 0 9 */
/*30*/  37,  36,  35,  34,   5,   4,   3,   2,  /* k j h g 4 3 2 1 */
/*38*/  23,  22,  21,  20, IGN,  58,  14,   1,  /* i u y t - caps del esc */
/*40*/ 184, IGN, IGN, IGN,  14,  25,  15, IGN,  /* alt_R - - - BS p TAB Fn */
/*48*/ IGN,  56, IGN, IGN,  88,  87,  68,  67,  /* - alt_L - - f12 f11 f10 f9*/
/*50*/ IGN, IGN,  29, IGN,  66,  65,  64,  63,  /* - - ctrl - f8 f7 f6 f5 */
/*58*/ IGN, IGN, IGN,  42,  62,  61,  60,  59,  /* - - - shift f4 f3 f2 f1 */
};

/* NEC MobilePro 750c by "Castor Fu" <castor@geocast.com> */
static char mobilepro750c_keytrans[] = {
/*00*/  77,  43,  25,  52,  21,  48,  44,  57, /* right \ p . y b z space */
/*08*/  80,  53,  24,  51,  20,  47,  30, IGN, /* down / o , t v a  - */
/*10*/  75,  28,  23,  50,  19,  46,  17, 221, /* left enter i m r c w Win */
/*18*/  69,  27,  22,  49,  18,  45,  16,  58, /* num ] u n e x q caps */
/*20*/  81,  IGN, 11,  38,   7,  34,  15,   1, /* pgdn - 0 l : g tab esc */
/*28*/ IGN,  39,  10,  37,   6,  33,   3,  41, /* - ; 9 k 5 f 2 ` */
/*30*/  72,  26,   9,  36,   5,  32,   2,  40, /* up [ 8 j 4 d 1 ' */
/*38*/  12,  26,   8,  35,   4,  31,  83, IGN, /* - @ 7 h 3 s del - */
/*40*/  42, IGN, IGN, IGN,  14,  88,  66,  62, /* shift - - - bs f12 f8 f4 */
/*48*/ IGN,  56, IGN, IGN, 125,  87,  65,  61, /* - alt - - | f11 f7 f3 */
/*50*/ IGN, IGN,  29, IGN,  68,  68,  64,  60, /* - - ctrl - f10 f10 f6 f2 */
/*58*/ IGN, IGN, IGN,  42,  13,  67,  63,  59, /* - - - shift del f9 f5 f1 */
};

/* FUJITSU INTERTOP CX300 */
static char intertop_keytrans[] = {
  57,  60,   2,  15,  28,  58,  75,  41,
 112,  59,   3,  16, IGN,  30,  56,   1,
 210,  17,   4,  31,  83,  43,  80,  45,
  44,  18,   5,  32,  68, 125,  77,  46,
 115,  19,  39,  33,  67,  26,  13,  47,
  53,  20,   6,  34,  66,  25,  12,  48,
  52,  21,   7,  35,  65,  38,  11,  49,
 IGN,  22,   8,  36,  63,  24,  14,  50,
 IGN,  61,   9,  62, IGN,  23,  37,  51,
  69,  40,  10,  27,  64, IGN,  72, IGN,
 IGN, IGN, IGN, IGN,  42, IGN, IGN,  54,
  29, 221, 123, 121, 184, IGN, IGN, IGN,
};
/*
space   a2      1       tab     enter   caps    left    zenkaku
hiraga  a1      2       q       -       a       fnc     esc
ins     w       3       s       del     ]       down    x
z       e       4       d       a10     \       right   c
backsla r       ;       f       a9      @       ^       v
/       t       5       g       a8      p       -       b
.       y       6       h       a7      l       0       n
-       u       7       j       a5      o       bs      m
-       a3      8       a4      -       i       k       ,
num     :       9       [       a6      -       up      -
-       -       -       -       shift_L -       -       shift_R
ctrl    win     muhenka henkan  alt     -       -       -
*/

static char *keytrans = default_keytrans;

/*
 * utilities
 */
static inline void
vrkiu_write(chip, port, val)
	struct vrkiu_chip *chip;
	int port;
	unsigned short val;
{
	bus_space_write_2(chip->kc_iot, chip->kc_ioh, port, val);
}

static inline unsigned short
vrkiu_read(chip, port)
	struct vrkiu_chip *chip;
	int port;
{
	return bus_space_read_2(chip->kc_iot, chip->kc_ioh, port);
}

static inline int
vrkiu_is_console(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	if (vrkiu_consdata &&
	    vrkiu_consdata->kc_iot == iot &&
	    vrkiu_consdata->kc_ioh == ioh) {
		return 1;
	} else {
		return 0;
	}
}

static void
vrkiu_initkeymap(void)
{
	if (platid_match(&platid, &platid_mask_MACH_NEC_MCR_520A)) {
		keytrans = mobilepro_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_US;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_NEC_MCR_500A)) {
		keytrans = mobilepro750c_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_US;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_NEC_MCR_700A)) {
		keytrans = mobilepro_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_US;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_NEC_MCR)) {
		keytrans = mcr_jp_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_JP;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_IBM_WORKPAD_Z50)) {
		keytrans = z50_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_US;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_SHARP_TRIPAD)) {
		keytrans = tripad_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_JP;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_NEC_MCCS)) {
		keytrans = mccs_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_JP;
#endif
	} else if (platid_match(&platid, &platid_mask_MACH_FUJITSU_INTERTOP)) {
		keytrans = intertop_keytrans;
#if !defined(PCKBD_LAYOUT)
		vrkiu_keymapdata.layout = KB_JP;
#endif
	}
}

/*
 * initialize device
 */
static int
vrkiu_init(chip, iot, ioh)
	struct vrkiu_chip* chip;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	memset(chip, 0, sizeof(struct vrkiu_chip));
	chip->kc_iot = iot;
	chip->kc_ioh = ioh;
	chip->kc_polling = 0;

	/* set KIU */
	vrkiu_write(chip, KIURST, 1);   /* reset */
	vrkiu_write(chip, KIUSCANLINE, 0); /* 96keys */
	vrkiu_write(chip, KIUWKS, 0x18a4); /* XXX: scan timing! */
	vrkiu_write(chip, KIUWKI, 450);
	vrkiu_write(chip, KIUSCANREP, 0x8023);
				/* KEYEN | STPREP = 2 | ATSTP | ATSCAN */
	vrkiu_initkeymap();
	return 0;
}

/*
 * probe
 */
static int
vrkiumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

/*
 * attach
 */
static void
vrkiuattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrkiu_softc *sc = (struct vrkiu_softc *)self;
	struct vrip_attach_args *va = aux;
	struct wskbddev_attach_args wa;
	int isconsole;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	isconsole = vrkiu_is_console(iot, ioh);
	if (isconsole) {
		sc->sc_chip = vrkiu_consdata;
	} else {
		sc->sc_chip = &sc->sc_chip_body;
		vrkiu_init(sc->sc_chip, iot, ioh);
	}
	sc->sc_chip->kc_sc = sc;

	if (!(sc->sc_handler = 
	      vrip_intr_establish(va->va_vc, va->va_intr, IPL_TTY,
				  vrkiu_intr, sc))) {
		printf (": can't map interrupt line.\n");
		return;
	}
	/* Level2 register setting */
	vrip_intr_setmask2(va->va_vc, sc->sc_handler, KIUINT_KDATRDY, 1);

	printf("\n");

	wa.console = isconsole;
	wa.keymap = &vrkiu_keymapdata;
	wa.accessops = &vrkiu_accessops;
	wa.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &wa, wskbddevprint);
}

int
vrkiu_intr(arg)
	void *arg;
{
        struct vrkiu_softc *sc = arg;

	/* When key scan finisshed, this entry is called. */
	DPRINTF(("%s(%d): vrkiu_intr: %d\n",
		 __FILE__, __LINE__,
		 vrkiu_read(sc->sc_chip, KIUINT) & 7));

	/*
	 * First, we must clear the interrupt register because
	 * detect_key() may takes long time if a bitmap screen
	 * scrolls up and it makes us to miss some key release
	 * event.
	 */
	vrkiu_write(sc->sc_chip, KIUINT, 0x7); /* Clear all interrupt */
	detect_key(sc->sc_chip);

	return 0;
}

static void
detect_key(chip)
	struct vrkiu_chip* chip;
{
	int i, j, modified, mask;
	unsigned short scandata[KIU_NSCANLINE/2];

	for (i = 0; i < KIU_NSCANLINE / 2; i++) {
		scandata[i] = vrkiu_read(chip, KIUDATP + i * 2);
	}

	DPRINTF(("%s(%d): detect_key():", __FILE__, __LINE__));

	if (chip->kc_polling) {
		chip->kc_type = WSCONS_EVENT_ALL_KEYS_UP;
	}

	for (i = 0; i < KIU_NSCANLINE / 2; i++) {
		modified = scandata[i] ^ chip->kc_scandata[i];
		mask = 1;
		for (j = 0; j < 16; j++, mask <<= 1) {
			/* XXX: The order of keys can be a problem.
			   If CTRL and normal key are pushed simultaneously,
			   normal key can be entered in queue first. 
			   Same problem would occur in key break. */
			if (modified & mask) {
				int key, type;
				key = i * 16 + j;
				if (keytrans[key] == UNK) {
	                                printf("vrkiu: Unknown scan code 0x%02x\n", key);
	                                continue;
				} else if (keytrans[key] == IGN) {
					continue;
				}
				type = (scandata[i] & mask) ? 
					WSCONS_EVENT_KEY_DOWN :
					WSCONS_EVENT_KEY_UP;
				DPRINTF(("(%d,%d)=%s%d ", i, j,
					 (scandata[i] & mask) ? "v" : "^",
					 keytrans[key]));
				if (chip->kc_polling) {
					chip->kc_type = type;
					chip->kc_data = keytrans[key];
				} else {
					wskbd_input(chip->kc_sc->sc_wskbddev,
						    type,
						    keytrans[key]);
				}
			}
		}
		chip->kc_scandata[i] = scandata[i];
	}
	DPRINTF(("\n"));
}

/* called from biconsdev.c */
int
vrkiu_getc()
{
	int ret;

	if (the_vrkiu == NULL) {
		return 0;	/* XXX */
	}

	while (the_vrkiu->keybuftail == the_vrkiu->keybufhead) {
		detect_key(vrkiu_consdata);
	}
	ret = the_vrkiu->keybuf[the_vrkiu->keybuftail++];
	if (the_vrkiu->keybuftail >= NKEYBUF) {
		the_vrkiu->keybuftail = 0;
	}
	return ret;
}

int
vrkiu_enable(scx, on)
	void *scx;
	int on;
{
	struct vrkiu_softc *sc = scx;

	if (on) {
		if (sc->sc_enabled)
			return (EBUSY);
		sc->sc_enabled = 1;
	} else {
		if (sc->sc_chip == vrkiu_consdata)
			return (EBUSY);
		sc->sc_enabled = 0;
	}

	return (0);
}

void
vrkiu_set_leds(scx, leds)
	void *scx;
	int leds;
{
	/*struct pckbd_softc *sc = scx;
	 */

	DPRINTF(("%s(%d): vrkiu_set_leds() not implemented\n",
		 __FILE__, __LINE__));
}

int
vrkiu_ioctl(scx, cmd, data, flag, p)
	void *scx;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	/*struct vrkiu_softc *sc = scx;
	 */

	switch (cmd) {
	case WSKBDIO_GTYPE:
		/*
		 * XXX, fix me !
		 */
		*(int *)data = WSKBD_TYPE_PC_XT;
		return 0;
	case WSKBDIO_SETLEDS:
		DPRINTF(("%s(%d): no LED\n", __FILE__, __LINE__));
		return 0;
	case WSKBDIO_GETLEDS:
		DPRINTF(("%s(%d): no LED\n", __FILE__, __LINE__));
		*(int *)data = 0;
		return (0);
	}
	return -1;
}

/*
 * console support routines
 */
int
vrkiu_cnattach(iot, iobase)
	bus_space_tag_t iot;
	int iobase;
{
	static struct vrkiu_chip vrkiu_consdata_body;
	bus_space_handle_t ioh;

	if (vrkiu_consdata) {
		panic("vrkiu is already attached as the console");
	}
	if (bus_space_map(iot, iobase, 1, 0, &ioh)) {
		printf("%s(%d): can't map bus space\n", __FILE__, __LINE__);
		return -1;
	}

	if (vrkiu_init(&vrkiu_consdata_body, iot, ioh) != 0) {
		DPRINTF(("%s(%d): vrkiu_init() failed\n", __FILE__, __LINE__));
		return -1;
	}
	vrkiu_consdata = &vrkiu_consdata_body;

	wskbd_cnattach(&vrkiu_consops, vrkiu_consdata, &vrkiu_keymapdata);

	return (0);
}

void
vrkiu_cngetc(chipx, type, data)
	void *chipx;
	u_int *type;
	int *data;
{
	struct vrkiu_chip* chip = chipx;
	int s;

	if (!chip->kc_polling) {
		printf("%s(%d): kiu is not polled\n", __FILE__, __LINE__);
		while (1);
	}

	s = splimp();
	if (chip->kc_type == WSCONS_EVENT_ALL_KEYS_UP) {
		detect_key(chip);
	}
	*type = chip->kc_type;
	*data = chip->kc_data;
	chip->kc_type = WSCONS_EVENT_ALL_KEYS_UP;
	splx(s);
}

void
vrkiu_cnpollc(chipx, on)
	void *chipx;
        int on;
{
	struct vrkiu_chip* chip = chipx;
	int s = splimp();

	chip->kc_polling = on;

	splx(s);

	DPRINTF(("%s(%d): vrkiu polling %s\n",
		 __FILE__, __LINE__, on ? "ON" : "OFF"));
}
