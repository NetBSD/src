/*	$NetBSD: vrkiu.c,v 1.1.1.1.2.2 2001/03/12 13:28:49 bouyer Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi All rights reserved.
 * Copyright (c) 1999,2000 TAKEMRUA, Shin All rights reserved.
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
#include <dev/hpc/hpckbdvar.h>

#include "opt_wsdisplay_compat.h"
#include "opt_pckbd_layout.h"
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbc/wskbdmap_mfii.h>
#ifdef WSDISPLAY_COMPAT_RAWKBD
#include <dev/hpc/pckbd_encode.h>
#endif

#define VRKIUDEBUG
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
	int kc_enabled;
	struct vrkiu_softc* kc_sc;	/* back link */
	struct hpckbd_ic_if	kc_if;
	struct hpckbd_if	*kc_hpckbd;
};

struct vrkiu_softc {
	struct device sc_dev;
	struct vrkiu_chip *sc_chip;
	struct vrkiu_chip sc_chip_body;
	void *sc_handler;
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
static void vrkiu_scan __P((struct vrkiu_chip *));
static int countbits(int);
static void eliminate_phantom_keys(struct vrkiu_chip*, unsigned short *);
static int vrkiu_poll __P((void*));
static int vrkiu_input_establish __P((void*, struct hpckbd_if*));

/*
 * global/static data
 */
struct cfattach vrkiu_ca = {
	sizeof(struct vrkiu_softc), vrkiumatch, vrkiuattach
};

struct vrkiu_chip *vrkiu_consdata = NULL;

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
	chip->kc_enabled = 0;

	chip->kc_if.hii_ctx		= chip;
	chip->kc_if.hii_establish	= vrkiu_input_establish;
	chip->kc_if.hii_poll		= vrkiu_poll;

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
	struct hpckbd_attach_args haa;
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

	/* attach hpckbd */
	haa.haa_ic = &sc->sc_chip->kc_if;
	config_found(self, &haa, hpckbd_print);
}

int
vrkiu_intr(arg)
	void *arg;
{
        struct vrkiu_softc *sc = arg;

	/* When key scan finisshed, this entry is called. */
#if 0
	DPRINTF(("vrkiu_intr: intr=%x scan=%x\n",
		 vrkiu_read(sc->sc_chip, KIUINT) & 7,
		 vrkiu_read(sc->sc_chip, KIUSCANS) & KIUSCANS_SSTAT_MASK));
#endif

	/*
	 * First, we must clear the interrupt register because
	 * vrkiu_scan() may takes long time if a bitmap screen
	 * scrolls up and it makes us to miss some key release
	 * event.
	 */
	vrkiu_write(sc->sc_chip, KIUINT, 0x7); /* Clear all interrupt */

#if 1
	/* just return if kiu is scanning keyboard. */
	if ((vrkiu_read(sc->sc_chip, KIUSCANS) & KIUSCANS_SSTAT_MASK) ==
	       KIUSCANS_SSTAT_SCANNING)
		return (0);
#endif
	vrkiu_scan(sc->sc_chip);

	return (0);
}

static int
countbits(d)
	int d;
{
	int i, n;

	for (i = 0, n = 0; i < NBBY; i++)
		if (d & (1 << i))
			n++;
	return n;
}

static void
eliminate_phantom_keys(chip, scandata)
	struct vrkiu_chip* chip;
	unsigned short *scandata;
{
	unsigned char inkey[KIU_NSCANLINE], *prevkey, *reskey;
	int i, j;
#ifdef VRKIUDEBUG
	int modified = 0;
	static int prevmod = 0;
#endif

	reskey = (unsigned char *)scandata;
	for (i = 0; i < KIU_NSCANLINE; i++)
		inkey[i] = reskey[i];
	prevkey = (unsigned char *)chip->kc_scandata;

	for (i = 0; i < KIU_NSCANLINE; i++) {
	    if (countbits(inkey[i]) > 1) {
		for (j = 0; j < KIU_NSCANLINE; j++) {
		    if (i != j && (inkey[i] & inkey[j])) {
#ifdef VRKIUDEBUG
			    modified = 1;
			    if (!prevmod) {
				DPRINTF(("vrkiu_scan: %x:%02x->%02x",
					 i, inkey[i], prevkey[i]));
				DPRINTF(("  %x:%02x->%02x\n",
					 j, inkey[j], prevkey[j]));
			    }
#endif
			    reskey[i] = prevkey[i];
			    reskey[j] = prevkey[j];
		    }
		}
	    }
	}
#ifdef VRKIUDEBUG
	prevmod = modified;
#endif
}

static void
vrkiu_scan(chip)
	struct vrkiu_chip* chip;
{
	int i, j, modified, mask;
	unsigned short scandata[KIU_NSCANLINE/2];

	if (!chip->kc_enabled)
		return;

	for (i = 0; i < KIU_NSCANLINE / 2; i++) {
		scandata[i] = vrkiu_read(chip, KIUDATP + i * 2);
	}
	eliminate_phantom_keys(chip, scandata);

	for (i = 0; i < KIU_NSCANLINE / 2; i++) {
		modified = scandata[i] ^ chip->kc_scandata[i];
		chip->kc_scandata[i] = scandata[i];
		mask = 1;
		for (j = 0; j < 16; j++, mask <<= 1) {
			/*
			 * Simultaneous keypresses are resolved by registering
			 * the one with the lowest bit index first.
			 */
			if (modified & mask) {
				int key = i * 16 + j;
				DPRINTF(("vrkiu_scan: %s(%d,%d)\n",
					 (scandata[i] & mask) ? "down" : "up",
					 i, j));
				hpckbd_input(chip->kc_hpckbd,
					     (scandata[i] & mask), key);
			}
		}
	}
}

/* called from bicons.c */
int
vrkiu_getc()
{
	static int flag = 1;

	/*
	 * XXX, currently
	 */
	if (flag) {
		flag = 0;
		printf("%s(%d): vrkiu_getc() is not implemented\n",
		       __FILE__, __LINE__);
	}
	return 0;
}

/*
 * hpckbd interface routines
 */
int
vrkiu_input_establish(ic, kbdif)
	void *ic;
	struct hpckbd_if *kbdif;
{
	struct vrkiu_chip *kc = ic;

	/* save hpckbd interface */
	kc->kc_hpckbd = kbdif;
	
	kc->kc_enabled = 1;

	return 0;
}

int
vrkiu_poll(ic)
	void *ic;
{
	struct vrkiu_chip *kc = ic;

#if 1
	/* wait until kiu completes keyboard scan. */
	while ((vrkiu_read(kc, KIUSCANS) & KIUSCANS_SSTAT_MASK) ==
	       KIUSCANS_SSTAT_SCANNING)
		/* wait until kiu completes keyboard scan */;
#endif

	vrkiu_scan(kc);

	return 0;
}

/*
 * console support routine
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

	hpckbd_cnattach(&vrkiu_consdata_body.kc_if);

	return (0);
}
