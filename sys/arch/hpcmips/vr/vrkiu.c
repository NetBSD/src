/*	$NetBSD: vrkiu.c,v 1.3 1999/11/03 13:55:41 enami Exp $	*/

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

const struct wskbd_mapdata vrkiu_keymapdata = {
	pckbd_keydesctab,
	KB_US,
};

struct vrkiu_chip *vrkiu_consdata = NULL;

/* XXX: This tranlation table may depend on each machine.
        Should I build it in? */
static char keytrans[] = {
	-1,  28,  25,  52,  21,  48,  44,  57,  /* - enter p . y b z space */
	-1,  53,  24,  51,  20,  47,  30,  -1,  /* - / o , t v a - */
	-1,  -1,  23,  50,  19,  46,  17,  -1,  /* - - i m r c w - */
	13,  -1,  22,  -1,  18,  45,  16,   2,  /* = - u - e x q 1 */
	-1,  -1,  11,  38,  40,  34,  15,  59,  /* - - 0 l ' g tab f1 */
	-1,  39,  10,  49,   6,  33,   3,  37,  /* - ; 9 n 5 f 2 k */
	-1,  27,   9,  36,   5,  32,   7,  -1,  /* - ] 8 j 4 d 6 - */
	12,  26,   8,  35,   4,  41,  31,  -1,  /* - [ 7 h 3 ` s - */
	58,  -1,  -1,  -1,  14,  -1,  66,  61,  /* caps - - - bs - f8 f3 */
	-1,  56,  -1,  -1,  43,  -1,  65,  62,  /* - alt - - \ - f7 f4 */
	-1,  -1,  29,  -1,  68,  -1,  64,  60,  /* - - ctrl - f10 - f6 f2 */
	-1,  -1,  -1,  42,  -1,  67,  63,   1,  /* - - - shift - f9 f5 esc */
};
/* XXX: fill the field of funct. keys, ex. arrow, fnc, nfer... */

#define	SCROLL		0x0001	/* stop output */
#define	NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	SHIFT		0x0008	/* keyboard shift */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	ASCII		0x0020	/* ascii code for this key */
#define	ALT		0x0080	/* alternate shift -- alternate chars */
#define	FUNC		0x0100	/* function key */
#define	KP		0x0200	/* Keypad keys */
#define	NONE		0x0400	/* no function */

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

	detect_key(sc->sc_chip);

	vrkiu_write(sc->sc_chip, KIUINT, 0x7); /* Clear all interrupt */

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
				if (keytrans[key] < 0) {
	                                printf("vrkiu: Unkown scan code 0x%02x\n", key);
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
