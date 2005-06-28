/* $NetBSD: j720ssp.c,v 1.24 2005/06/28 18:30:00 drochner Exp $ */

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: j720ssp.c,v 1.24 2005/06/28 18:30:00 drochner Exp $");

#include "apm.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/kthread.h>
#include <sys/lock.h>

#include <machine/bus.h>
#include <machine/config_hook.h>
#include <machine/bootinfo.h>
#include <machine/apmvar.h>

#include <hpcarm/dev/sed1356var.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/hpc/hpctpanelvar.h>

extern const struct wscons_keydesc j720kbd_keydesctab[];

struct j720ssp_softc {
        struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_gpioh;
	bus_space_handle_t sc_ssph;

	struct device *sc_wskbddev;
	struct device *sc_wsmousedev;
	struct tpcalib_softc sc_tpcalib;

	void *sc_kbdsi;
	void *sc_tpsi;
	int sc_enabled;

	struct proc *sc_ssp_kthread;
	int sc_ssp_status;
	struct simplelock sc_ssp_status_lock;
};
/* Values for struct softc's sc_ssp_status */
#define J720_SSP_STATUS_NONE	0
#define J720_SSP_STATUS_TP	1
#define J720_SSP_STATUS_KBD	2

void j720ssp_create_kthread __P((void *));
void j720ssp_kthread __P((void *));
int  j720kbd_intr __P((void *));
int  j720tp_intr __P((void *));
void j720kbd_poll __P((void *));
int  j720tp_poll __P((void *));

int  j720lcdparam __P((void *, int, long, void *));
static void j720kbd_read __P((struct j720ssp_softc *, char *));
static int  j720ssp_readwrite __P((struct j720ssp_softc *, int,
	int, int *, int));

int  j720sspprobe __P((struct device *, struct cfdata *, void *));
void j720sspattach __P((struct device *, struct device *, void *));

int  j720kbd_enable __P((void *, int));
void j720kbd_set_leds __P((void *, int));
int  j720kbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

int  j720lcdpower(void *, int, long, void *);
int  hpcarm_apm_getpower __P((struct apm_power_info *, void *));

CFATTACH_DECL(j720ssp, sizeof(struct j720ssp_softc),
    j720sspprobe, j720sspattach, NULL, NULL);

const struct wskbd_accessops j720kbd_accessops = {
	j720kbd_enable,
	j720kbd_set_leds,
	j720kbd_ioctl,
};

void j720kbd_cngetc __P((void *, u_int *, int *));
void j720kbd_cnpollc __P((void *, int));
void j720kbd_cnbell __P((void *, u_int, u_int, u_int));

const struct wskbd_consops j720kbd_consops = {
	j720kbd_cngetc,
	j720kbd_cnpollc,
	j720kbd_cnbell,
};

const struct wskbd_mapdata j720kbd_keymapdata = {
	j720kbd_keydesctab,
#ifdef J720KBD_LAYOUT
	J720KBD_LAYOUT,
#else
	KB_US,
#endif
};

static int  j720tp_enable __P((void *));
static int  j720tp_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static void j720tp_disable __P((void *));

const struct wsmouse_accessops j720tp_accessops = {
	j720tp_enable,
	j720tp_ioctl,
	j720tp_disable,
};

static int j720ssp_powerstate = 1;

static struct j720ssp_softc j720kbdcons_sc;
static int j720kbdcons_initstate = 0;

#define DEBUG
#ifdef DEBUG
int j720sspwaitcnt;
int j720sspwaittime;
extern int gettick(void);
#endif

#define BIT_INVERT(x)	do {					\
	(x) = ((((x) & 0xf0) >> 4) | (((x) & 0x0f) << 4));	\
	(x) = ((((x) & 0xcc) >> 2) | (((x) & 0x33) << 2));	\
	(x) = ((((x) & 0xaa) >> 1) | (((x) & 0x55) << 1));	\
	} while(0)


int
j720sspprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

void
j720sspattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct j720ssp_softc *sc = (void *)self;
	struct sa11x0_softc *psc = (void *)parent;
	struct sa11x0_attach_args *sa = aux;
	struct wskbddev_attach_args kbd_args;
	struct wsmousedev_attach_args mouse_args;
#if NAPM > 0
	struct apm_attach_args apm_args;
#endif

	printf("\n");

	sc->sc_iot = psc->sc_iot;
	sc->sc_gpioh = psc->sc_gpioh;
	if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ssph)) {
		printf("%s: unable to map SSP registers\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_ssp_status = J720_SSP_STATUS_NONE;
	simple_lock_init(&sc->sc_ssp_status_lock);
	kthread_create(j720ssp_create_kthread, sc);

	sc->sc_enabled = 0;

	kbd_args.console = 0;

	kbd_args.keymap = &j720kbd_keymapdata;

	kbd_args.accessops = &j720kbd_accessops;
	kbd_args.accesscookie = sc;

	/* Do console initialization */
	if (! (bootinfo->bi_cnuse & BI_CNUSE_SERIAL)) {
		j720kbdcons_sc = *sc;
		kbd_args.console = 1;

		wskbd_cnattach(&j720kbd_consops, NULL, &j720kbd_keymapdata);
		j720kbdcons_initstate = 1;
	}

	/*
	 * Attach the wskbd, saving a handle to it.
	 * XXX XXX XXX
	 */
	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &kbd_args,
					  wskbddevprint);

#ifdef DEBUG
	/* Zero the stat counters */
	j720sspwaitcnt = 0;
	j720sspwaittime = 0;
#endif

	if (j720kbdcons_initstate == 1)
		j720kbd_enable(sc, 1);

	mouse_args.accessops = &j720tp_accessops;
	mouse_args.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &mouse_args, 
	    wsmousedevprint);
	tpcalib_init(&sc->sc_tpcalib);

	/* XXX fill in "default" calibrate param */
	{
		static const struct wsmouse_calibcoords j720_default_calib = {
			0, 0, 639, 239,
			4,
			{ { 988,  80,   0,   0 },
			  {  88,  84, 639,   0 },
			  { 988, 927,   0, 239 },
			  {  88, 940, 639, 239 } } };
		tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		    __UNCONST(&j720_default_calib), 0, 0);
	}

	j720tp_disable(sc);

	/* Setup touchpad interrupt */
	sa11x0_intr_establish(0, 9, 1, IPL_BIO, j720tp_intr, sc);

	/* LCD control is on the same bus */
	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE, j720lcdparam, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE, j720lcdparam, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS_MAX,
		    CONFIG_HOOK_SHARE, j720lcdparam, sc);

	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST,
		    CONFIG_HOOK_SHARE, j720lcdparam, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST,
		    CONFIG_HOOK_SHARE, j720lcdparam, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST_MAX,
		    CONFIG_HOOK_SHARE, j720lcdparam, sc);

#if NAPM > 0
	/* attach APM emulation */
	apm_args.aaa_magic = APM_ATTACH_ARGS_MAGIC; /* magic number */
	(void)config_found_ia(self, "j720sspapm", &apm_args, NULL);
#endif

	return;
}

void
j720ssp_create_kthread(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;

	if (kthread_create1(j720ssp_kthread, sc, 
	    &sc->sc_ssp_kthread, "j720ssp")) 
		panic("j720ssp_create_kthread");

	return;
}

void
j720ssp_kthread(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;
	int ssp_status;

	while (1) {
		simple_lock(&sc->sc_ssp_status_lock);
		ssp_status = sc->sc_ssp_status;
		sc->sc_ssp_status &= ~J720_SSP_STATUS_KBD;
		simple_unlock(&sc->sc_ssp_status_lock);

		if (ssp_status & J720_SSP_STATUS_KBD)
			j720kbd_poll(sc);

		if (ssp_status & J720_SSP_STATUS_TP) {
			if (j720tp_poll(sc) == 0) {
				simple_lock(&sc->sc_ssp_status_lock);
				sc->sc_ssp_status &= ~J720_SSP_STATUS_TP;
				simple_unlock(&sc->sc_ssp_status_lock);
			}
			tsleep(&sc->sc_ssp_kthread, PRIBIO, "j720ssp", hz / 25);
		} else
			tsleep(&sc->sc_ssp_kthread, PRIBIO, "j720ssp", 0);
	}

	/* NOTREACHED */
}

int
j720kbd_enable(v, on)
	void *v;
	int on;
{
	struct j720ssp_softc *sc = v;

	if (! sc->sc_enabled) {
		sc->sc_enabled = 1;

		sa11x0_intr_establish(0, 0, 1, IPL_BIO, j720kbd_intr, sc);
	}
	/* XXX */
	return (0);
}

void
j720kbd_set_leds(v, on)
	void *v;
	int on;
{
	/* XXX */
	return;
}

int
j720kbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HPC_KBD;
		return 0;
	}

	return (EPASSTHROUGH);
}

int
j720kbd_intr(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;

	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_EDR, 1);

	simple_lock(&sc->sc_ssp_status_lock);
	sc->sc_ssp_status |= J720_SSP_STATUS_KBD;
	simple_unlock(&sc->sc_ssp_status_lock);

	wakeup(&sc->sc_ssp_kthread);

	return (1);
}

int
j720tp_intr(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;

	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_EDR, 1 << 9);

	simple_lock(&sc->sc_ssp_status_lock);
	sc->sc_ssp_status |= J720_SSP_STATUS_TP;
	simple_unlock(&sc->sc_ssp_status_lock);

	j720tp_disable(sc);
	wakeup(&sc->sc_ssp_kthread);

	return (1);
}

void
j720kbd_poll(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;
	int s, type, value;
	char buf[9], *p;

	j720kbd_read(sc, buf);

	for(p = buf; *p; p++) {
		type = *p & 0x80 ? WSCONS_EVENT_KEY_UP :
		    WSCONS_EVENT_KEY_DOWN;
		value = *p & 0x7f;
		s = spltty();
		wskbd_input(sc->sc_wskbddev, type, value);
		splx(s);
		if (type == WSCONS_EVENT_KEY_DOWN &&
		    value == 0x7f) {
			j720ssp_powerstate = ! j720ssp_powerstate;
			config_hook_call(CONFIG_HOOK_POWERCONTROL,
					 CONFIG_HOOK_POWERCONTROL_LCDLIGHT,
					 (void *)j720ssp_powerstate);
		}
	}

	return;
}

void
j720kbd_read(sc, buf)
	struct j720ssp_softc *sc;
	char *buf;
{
	int data, count;
#ifdef DEBUG
	u_int32_t oscr;

	oscr = gettick();
#endif
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PCR, 0x2000000);

	/* send scan keycode command */
	if (j720ssp_readwrite(sc, 1, 0x900, &data, 100) < 0 ||
	    data != 0x88)
		goto out;

	/* read numbers of scancode available */
	if (j720ssp_readwrite(sc, 0, 0x8800, &data, 100) < 0)
		goto out;
	BIT_INVERT(data);
	count = data;

	for(; count; count--) {
		if (j720ssp_readwrite(sc, 0, 0x8800, &data, 100) < 0)
			goto out;
		BIT_INVERT(data);
		*buf++ = data;
	}
	*buf = 0;
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

#ifdef DEBUG
	oscr = (u_int32_t)gettick() - oscr;
	j720sspwaitcnt++;
	j720sspwaittime += oscr;
#endif

	return;

out:
	*buf = 0;
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x387);

	printf("j720kbd_read: error %x\n", data);
	return;
}

int
j720tp_poll(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;
	int buf[8], data, i, x, y;

	/* 
	 * If touch panel is not touched anymore, 
	 * stop polling and re-enable interrupt 
	 */
	if (bus_space_read_4(sc->sc_iot, 
	    sc->sc_gpioh, SAGPIO_PLR) & (1 << 9)) {
		wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0);
		j720tp_enable(sc);
		return 0;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PCR, 0x2000000);

	/* send read touchpanel command */
	if (j720ssp_readwrite(sc, 1, 0x500, &data, 100) < 0 ||
	    data != 0x88)
		goto out;

	for(i = 0; i < 8; i++) {
		if (j720ssp_readwrite(sc, 0, 0x8800, &data, 100) < 0)
			goto out;
		BIT_INVERT(data);
		buf[i] = data;
	}

	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

	buf[6] <<= 8;
	buf[7] <<= 8;
	for(i = 0; i < 3; i++) {
		buf[i] |= buf[6] & 0x300;
		buf[6] >>= 2;
		buf[i + 3] |= buf[7] & 0x300;
		buf[7] >>= 2;
	}
#if 0
	printf("j720tp_poll: %d %d %d  %d %d %d\n", buf[0], buf[1], buf[2],
	    buf[3], buf[4], buf[5]);
#endif

	/* XXX buf[1], buf[2], ... should also be used */
	tpcalib_trans(&sc->sc_tpcalib, buf[1], buf[4], &x, &y);
	wsmouse_input(sc->sc_wsmousedev, 1, x, y, 0,
	    WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);

	return 1;

out:
	*buf = 0;
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x387);
	printf("j720tp_poll: error %x\n", data);

	return 0;
}

static int
j720tp_enable(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;
	int er, s;

	s = splhigh();
	er = bus_space_read_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_FER);
	er |= 1 << 9;
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_FER, er);
	splx(s);

	return (0);
}
	
static void
j720tp_disable(arg)
	void *arg;
{
	struct j720ssp_softc *sc = arg;
	int er, s;

	s = splhigh();
	er = bus_space_read_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_FER);
	er &= ~(1 << 9);
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_FER, er);
	splx(s);
}

static int
j720tp_ioctl(arg, cmd, data, flag, p)
	void *arg;
	u_long cmd;
	caddr_t data; 
	int flag; 
	struct proc *p;
{
	struct j720ssp_softc *sc = arg;

	return hpc_tpanel_ioctl(&sc->sc_tpcalib, cmd, data, flag, p);
}

int
j720lcdparam(ctx, type, id, msg)
	void *ctx; 
	int type; 
	long id; 
	void *msg;
{
	struct j720ssp_softc *sc = ctx;
	int i, s;
	u_int32_t data[2], len;

	switch (type) {
	case CONFIG_HOOK_GET:
		switch (id) {
		case CONFIG_HOOK_BRIGHTNESS_MAX:
		case CONFIG_HOOK_CONTRAST_MAX:
			*(int *)msg = 255;
			return 1;
		case CONFIG_HOOK_BRIGHTNESS:
			data[0] = 0x6b00;
			data[1] = 0x8800;
			len = 2;
			break;
		case CONFIG_HOOK_CONTRAST:
			data[0] = 0x2b00;
			data[1] = 0x8800;
			len = 2;
			break;
		default:
			return 0;
		}
		break;

	case CONFIG_HOOK_SET:
		switch (id) {
		case CONFIG_HOOK_BRIGHTNESS:
			if (*(int *)msg >= 0) {
				data[0] = 0xcb00;
				data[1] = *(int *)msg;
				BIT_INVERT(data[1]);
				data[1] <<= 8;
				len = 2;
			} else {
				/* XXX hack */
				data[0] = 0xfb00;
				len = 1;
			}
			break;
		case CONFIG_HOOK_CONTRAST:
			data[0] = 0x8b00;
			data[1] = *(int *)msg;
			BIT_INVERT(data[1]);
			data[1] <<= 8;
			len = 2;
			break;
		default:
			return 0;
		}

	default:
		return 0;
	}

	s = splbio();
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PCR, 0x2000000);

	for (i = 0; i < len; i++) {
		if (j720ssp_readwrite(sc, 1, data[i], &data[i], 500) < 0)
			goto out;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);
	splx(s);

	if (type == CONFIG_HOOK_SET)
		return 1;

	BIT_INVERT(data[1]);
	*(int *)msg = data[1];

	return 1;

out:
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x387);
	splx(s);
	return 0;
}

static int
j720ssp_readwrite(sc, drainfifo, in, out, wait)
	struct j720ssp_softc *sc;
	int drainfifo;
	int in;
	int *out;
	int wait;
{
	int timo;

	timo = 100000;
	while(bus_space_read_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PLR) & 0x400)
		if (--timo == 0) {
			printf("timo0\n");
			return -1;
		}
	if (drainfifo) {
		while(bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_SR) &
		      SR_RNE)
			bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_DR);
#if 1
		delay(wait);
#endif
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_DR, in);

	delay(wait);
	timo = 100000;
	while(! (bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_SR) & SR_RNE))
		if (--timo == 0) {
			printf("timo1\n");
			return -1;
		}

	*out = bus_space_read_4(sc->sc_iot, sc->sc_ssph, SASSP_DR);

	return 0;
}

#if 0
int
j720kbd_cnattach(void)
{
	/* XXX defer initialization till j720sspattach */

	return (0);
}
#endif

/* ARGSUSED */
void
j720kbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	char buf[9];

	if (j720kbdcons_initstate < 1)
		return;

	for (;;) {
		j720kbd_read(&j720kbdcons_sc, buf);

		if (buf[0] != 0) {
			/* XXX we are discarding buffer contents */
			*type = buf[0] & 0x80 ? WSCONS_EVENT_KEY_UP :
			    WSCONS_EVENT_KEY_DOWN;
			*data = buf[0] & 0x7f;
			return;
		}
	}
}

void
j720kbd_cnpollc(v, on)
	void *v;
	int on;
{
#if 0
	/* XXX */
	struct j720kbd_internal *t = v;

	pckbc_set_poll(t->t_kbctag, t->t_kbcslot, on);
#endif
}

void
j720kbd_cnbell(v, pitch, period, volume)
	void *v; 
	u_int pitch; 
	u_int period;
	u_int volume;
{
}

int
j720lcdpower(ctx, type, id, msg)
	void *ctx;
	int type; 
	long id; 
	void *msg;
{
	struct sed1356_softc *sc = ctx;
	struct sa11x0_softc *psc = sc->sc_parent;
	int val;
	u_int32_t reg;

	if (type != CONFIG_HOOK_POWERCONTROL ||
	    id != CONFIG_HOOK_POWERCONTROL_LCDLIGHT)
		return 0;

	sed1356_init_brightness(sc, 0);
	sed1356_init_contrast(sc, 0);

	if (msg) {
		bus_space_write_1(sc->sc_iot, sc->sc_regh, 0x1f0, 0);

		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg |= 0x1;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
		delay(50000);

		val = sc->sc_contrast;
		config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST, &val);
		delay(100000);

		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg |= 0x4;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);

		val = sc->sc_brightness;
		config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);

		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg |= 0x2;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
	} else {
		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg &= ~0x2;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
		reg &= ~0x4;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
		delay(100000);

		val = -2;
		config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);

		bus_space_write_1(sc->sc_iot, sc->sc_regh, 0x1f0, 1);

		delay(100000);
		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg &= ~0x1;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
	}
	return 1;
}

int
hpcarm_apm_getpower(api, parent)
	struct apm_power_info *api;
	void *parent;
{
	int data, pmdata[3], i;
	struct j720ssp_softc *sc = (struct j720ssp_softc *)parent;

	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PCR, 0x2000000);

	if (j720ssp_readwrite(sc, 1, 0x300, &data, 100) < 0 || data != 0x88)
		goto out;

	for (i = 0; i < 3; i++) {
		if (j720ssp_readwrite(sc, 0, 0x8800, &pmdata[i], 500) < 0)
			goto out;
		BIT_INVERT(pmdata[i]);
	}

	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

	bzero(api, sizeof(struct apm_power_info));
	api->ac_state = APM_AC_UNKNOWN;

	/*
	 * pmdata[0] is the main battery level
	 * pmdata[1] is the backup battery level
	 * pmdata[2] tells which battery is present
	 */
	switch(pmdata[2]) {
	case 14: /* backup battery present */
	case 2:  /* backup battery absent */
		api->battery_state = APM_BATT_CHARGING;
		api->minutes_left = (pmdata[0] * 840) / 170;
		api->battery_life = (pmdata[0] * 100) / 170;
		api->nbattery = 1;
		break;
	case 15: /* backup battery present */
	case 3:  /* backup battery absent */
		api->battery_state = APM_BATT_ABSENT;
		api->battery_life = 0;
		api->nbattery = 0;
		break;
	default:
		api->battery_state = APM_BATT_UNKNOWN;
		break;
	}

	return 0;

out:
	bus_space_write_4(sc->sc_iot, sc->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(sc->sc_iot, sc->sc_ssph, SASSP_CR0, 0x387);

	printf("hpcarm_apm_getpower: error %x\n", data);
	return EIO;
}
