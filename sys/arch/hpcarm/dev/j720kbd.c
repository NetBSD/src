/*	$NetBSD: j720kbd.c,v 1.6.78.1 2021/03/21 21:09:00 thorpej Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Peter Postma.
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

/* Jornada 720 keyboard driver. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720kbd.c,v 1.6.78.1 2021/03/21 21:09:00 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bootinfo.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <dev/hpc/hpckbdvar.h>

#include <hpcarm/dev/j720sspvar.h>

#ifdef DEBUG
#define DPRINTF(arg)	aprint_normal arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

struct j720kbd_chip {
	int			scc_enabled;

	struct j720ssp_softc	*scc_ssp;

	struct hpckbd_ic_if	scc_if;
	struct hpckbd_if	*scc_hpckbd;
};

struct j720kbd_softc {
	device_t		sc_dev;

	struct j720kbd_chip	*sc_chip;
};

static struct j720kbd_chip j720kbd_chip;

static int	j720kbd_match(device_t, cfdata_t, void *);
static void	j720kbd_attach(device_t, device_t, void *);

static void	j720kbd_cnattach(void);
static void	j720kbd_ifsetup(struct j720kbd_chip *);
static int	j720kbd_input_establish(void *, struct hpckbd_if *);
static int	j720kbd_intr(void *);
static int	j720kbd_poll(void *);
static void	j720kbd_read(struct j720kbd_chip *, char *);

CFATTACH_DECL_NEW(j720kbd, sizeof(struct j720kbd_softc),
    j720kbd_match, j720kbd_attach, NULL, NULL);


static int
j720kbd_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_7XX))
		return 0;
	if (strcmp(cf->cf_name, "j720kbd") != 0)
		return 0;

	return 1;
}

static void
j720kbd_attach(device_t parent, device_t self, void *aux)
{
	struct j720kbd_softc *sc = device_private(self);
	struct hpckbd_attach_args haa;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_chip = &j720kbd_chip;
	sc->sc_chip->scc_ssp = device_private(parent);
	sc->sc_chip->scc_enabled = 0;

	/* Attach console if not using serial. */
	if (!(bootinfo->bi_cnuse & BI_CNUSE_SERIAL))
		j720kbd_cnattach();

	j720kbd_ifsetup(sc->sc_chip);

	/* Install interrupt handler. */
	sa11x0_intr_establish(0, 0, 1, IPL_TTY, j720kbd_intr, sc);

	/* Attach hpckbd. */
	haa.haa_ic = &sc->sc_chip->scc_if;
	config_found(self, &haa, hpckbd_print, CFARG_EOL);
}

static void
j720kbd_cnattach(void)
{
	struct j720kbd_chip *scc = &j720kbd_chip;

	/* Initialize interface. */
	j720kbd_ifsetup(scc);

	/* Attach console. */
	hpckbd_cnattach(&scc->scc_if);
}

static void
j720kbd_ifsetup(struct j720kbd_chip *scc)
{

	scc->scc_if.hii_ctx = scc;
	scc->scc_if.hii_establish = j720kbd_input_establish;
	scc->scc_if.hii_poll = j720kbd_poll;
}

static int
j720kbd_input_establish(void *ic, struct hpckbd_if *kbdif)
{
	struct j720kbd_chip *scc = ic;

	/* Save hpckbd interface. */
	scc->scc_hpckbd = kbdif;

	scc->scc_enabled = 1;

	return 0;
}

static int
j720kbd_intr(void *arg)
{
	struct j720kbd_softc *sc = arg;
	struct j720ssp_softc *ssp = sc->sc_chip->scc_ssp;

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_EDR, 1);

	return j720kbd_poll(sc->sc_chip);
}

static int
j720kbd_poll(void *arg)
{
	struct j720kbd_chip *scc = arg;
	int type, key;
	char buf[9], *p;

	if (!scc->scc_enabled) {
		DPRINTF(("j720kbd_poll: !scc_enabled\n"));
		return 0;
	}

	j720kbd_read(scc, buf);

	for (p = buf; *p; p++) {
		type = *p & 0x80 ? 0 : 1;
		key = *p & 0x7f;

		hpckbd_input(scc->scc_hpckbd, type, key);
	}

	return 1;
}

static void
j720kbd_read(struct j720kbd_chip *scc, char *buf)
{
	struct j720ssp_softc *ssp = scc->scc_ssp;
	int data, count;

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PCR, 0x2000000);

	/* Send scan keycode command. */
	if (j720ssp_readwrite(ssp, 1, 0x90, &data, 500) < 0 || data != 0x11) {
		DPRINTF(("j720kbd_read: no dummy received\n"));
		goto out;
	}

	/* Read number of scancodes available. */
	if (j720ssp_readwrite(ssp, 0, 0x11, &data, 500) < 0) {
		DPRINTF(("j720kbd_read: unable to read number of scancodes\n"));
		goto out;
	}
	count = data;

	for (; count; count--) {
		if (j720ssp_readwrite(ssp, 0, 0x11, &data, 100) < 0)
			goto out;
		*buf++ = data;
	}
	*buf = 0;
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	return;

out:
	*buf = 0;
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x387);

	DPRINTF(("j720kbd_read: error %x\n", data));
}
