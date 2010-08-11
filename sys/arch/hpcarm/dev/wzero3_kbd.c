/*	$NetBSD: wzero3_kbd.c,v 1.5.4.2 2010/08/11 22:52:05 yamt Exp $	*/

/*
 * Copyright (c) 2008, 2009, 2010 NONAKA Kimihiro <nonaka@netbsd.org>
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_kbd.c,v 1.5.4.2 2010/08/11 22:52:05 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/callout.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/hpc/hpckbdvar.h>

#include <arch/hpcarm/dev/wzero3_reg.h>

#ifdef DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

#define	CSR_READ1(r)	bus_space_read_1(sc->sc_iot, sc->sc_ioh, (r))
#define	CSR_WRITE1(r,v)	bus_space_write_1(sc->sc_iot, sc->sc_ioh, (r), (v))
#define	CSR_READ2(r)	bus_space_read_2(sc->sc_iot, sc->sc_ioh, (r))
#define	CSR_WRITE2(r,v)	bus_space_write_2(sc->sc_iot, sc->sc_ioh, (r), (v))
#define	CSR_READ4(r)	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (r))
#define	CSR_WRITE4(r,v)	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (r), (v))

/* register */
#define	KBDCOL_L	(0x00)	/* Write */
#define	KBDCOL_U	(0x04)	/* Write */
#define	KBDCHARGE	(0x08)	/* Write */
#define	KBDDATA		(0x08)	/* Read */
#define	REGMAPSIZE	0x0c

#define	KEYWAIT		20	/* us */

#define	WS003SH_NCOLUMN	12
#define	WS003SH_NROW	7

struct wzero3kbd_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	int sc_ncolumn;
	int sc_nrow;
	uint8_t *sc_okeystat;
	uint8_t *sc_keystat;

	void *sc_key_ih;
	void *sc_power_ih;
	void *sc_reset_ih;

	int sc_key_pin;
	int sc_power_pin;
	int sc_reset_pin;

	struct hpckbd_ic_if sc_if;
	struct hpckbd_if *sc_hpckbd;

	struct sysmon_pswitch sc_smpsw; /* for reset key */

	int sc_enabled;

	/* polling stuff */
	struct callout sc_keyscan_ch;
	int sc_interval;
#define	KEY_INTERVAL	50	/* ms */

#if defined(KEYTEST) || defined(KEYTEST2) || defined(KEYTEST3) || defined(KEYTEST4) || defined(KEYTEST5)
	void *sc_test_ih;
	int sc_test_pin;
	int sc_nouse_pin;
	int sc_nouse_pin2;
	int sc_nouse_pin3;
	int sc_bit;
#endif
};

static int wzero3kbd_match(device_t, cfdata_t, void *);
static void wzero3kbd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3kbd, sizeof(struct wzero3kbd_softc),
    wzero3kbd_match, wzero3kbd_attach, NULL, NULL);

static int wzero3kbd_intr(void *arg);
#if defined(KEYTEST)
static int wzero3kbd_intr2(void *arg);
#endif
#if defined(KEYTEST3)
static int wzero3kbd_intr3(void *arg);
#endif
static void wzero3kbd_tick(void *arg);
static int wzero3kbd_power_intr(void *arg);
static int wzero3kbd_reset_intr(void *arg);
static int wzero3kbd_input_establish(void *arg, struct hpckbd_if *kbdif);
static void wzero3kbd_sysmon_reset_event(void *arg);
static int wzero3kbd_poll(void *arg);
static int wzero3kbd_poll1(void *arg);

/*
 * WS003SH/WS004SH/WS007SH keyscan map
	col#0	col#1	col#2	col#3	col#4	col#5	col#6	col#7	col#8	col#9	col#10	col#11
row#0:	CTRL	1	3	5	6	7	9	0	BS	(none)	ROTATE	CAMERA
row#1:	(none)	2	4	r	y	8	i	o	p	(none)	VOL-	VOL+
row#2:	TAB	q	e	t	g	u	j	k	(none)	(none)	(none)	(none)
row#3:	(none)	w	s	f	v	h	m	l	(none)	(none)	SHIFT	(none)
row#4:	CALL	a	d	c	b	n	.	(none)	ENTER	(none)	WIN	(none)
row#5:	MAIL	z	x	-	SPACE	/	(none)	UP	(none)	(none)	LSOFT	FN
row#6:	IE	MOJI	(none)	OK	ACTION	,	LEFT	DOWN	RIGHT	(none)	RSOFT	(none)
*/

/*
 * WS011SH keyscan map
	col#0	col#1	col#2	col#3	col#4	col#5	col#6	col#7	col#8	col#9	col#10	col#11
row#0	Ctrl	(none)	(none)	(none)	(none)	(none)	(none)	(none)	Del	(none)	ROTATE	(none)
row#1	(none)	(none)	(none)	R	Y	(none)	I	O	P	(none)	(none)	(none)
row#2	Tab	Q	E	T	G	U	J	K	(none)	(none)	(none)	(none)
row#3	(none)	W	S	F	V	H	M	L	(none)	(none)	Shift	(none)
row#4	(none)	A	D	C	B	N	.	(none)	Enter	(none)	(none)	(none)
row#5	(none)	Z	X	-	Space	/	(none)	UP	(none)	(none)	(none)	Fn
row#6	(none)	MOJI	HAN/ZEN	OK	(none)	,	LEFT	DOWN	RIGHT	(none)	(none)	(none)
*/

/*
 * WS020SH keyscan map
	col#0	col#1	col#2	col#3	col#4	col#5	col#6	col#7	col#8	col#9	col#10	col#11
row#0	Ctrl	(none)	(none)	(none)	(none)	(none)	(none)	(none)	Del	(none)	ROTATE	(none)
row#1	(none)	(none)	(none)	R	Y	(none)	I	O	P	(none)	MEDIA	(none)
row#2	Tab	Q	E	T	G	U	J	K	(none)	(none)	(none)	(none)
row#3	(none)	W	S	F	V	H	M	L	(none)	(none)	LShift	(none)
row#4	(none)	A	D	C	B	N	.	(none)	Enter	(none)	RShift	(none)
row#5	(none)	Z	X	-	Space	/	(none)	UP	(none)	DOWN	(none)	Fn
row#6	(none)	MOJI	HAN/ZEN	OK	(none)	,	LEFT	(none)	RIGHT	(none)	(none)	(none)
*/

static const struct wzero3kbd_model {
	platid_mask_t *platid;
	int key_pin;
	int power_pin;
	int reset_pin;
	int ncolumn;
	int nrow;
} wzero3kbd_table[] = {
	/* WS003SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS003SH,
		-1,	/* XXX */
		GPIO_WS003SH_POWER_BUTTON,
		-1,	/* None */
		WS003SH_NCOLUMN,
		WS003SH_NROW,
	},
	/* WS004SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS004SH,
		-1,	/* XXX */
		GPIO_WS003SH_POWER_BUTTON,
		-1,	/* None */
		WS003SH_NCOLUMN,
		WS003SH_NROW,
	},
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
		-1,	/* XXX */
		GPIO_WS007SH_POWER_BUTTON,
		GPIO_WS007SH_RESET_BUTTON,
		WS003SH_NCOLUMN,
		WS003SH_NROW,
	},
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
		-1,	/* XXX */
		GPIO_WS011SH_POWER_BUTTON,
		GPIO_WS011SH_RESET_BUTTON,
		WS003SH_NCOLUMN,
		WS003SH_NROW,
	},
	/* WS020SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS020SH,
		-1,	/* XXX */
		GPIO_WS020SH_POWER_BUTTON,
		GPIO_WS020SH_RESET_BUTTON,
		WS003SH_NCOLUMN,
		WS003SH_NROW,
	},

	{ NULL, -1, -1, -1, 0, 0, }
};

static const struct wzero3kbd_model *
wzero3kbd_lookup(void)
{
	const struct wzero3kbd_model *model;

	for (model = wzero3kbd_table; model->platid != NULL; model++) {
		if (platid_match(&platid, model->platid)) {
			return model;
		}
	}
	return NULL;
}

static int
wzero3kbd_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (strcmp(cf->cf_name, "wzero3kbd") != 0)
		return 0;
	if (wzero3kbd_lookup() == NULL)
		return 0;
	return 1;
}

static void
wzero3kbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct wzero3kbd_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = (struct pxaip_attach_args *)aux;
	struct hpckbd_attach_args haa;
	const struct wzero3kbd_model *model;

	sc->sc_dev = self;

	model = wzero3kbd_lookup();
	if (model == NULL) {
		aprint_error(": unknown model\n");
		return;
	}

	aprint_normal(": keyboard\n");
	aprint_naive("\n");

	sc->sc_key_pin = model->key_pin;
	sc->sc_power_pin = model->power_pin;
	sc->sc_reset_pin = model->reset_pin;
	sc->sc_ncolumn = model->ncolumn;
	sc->sc_nrow = model->nrow;

	sc->sc_iot = pxa->pxa_iot;
	if (bus_space_map(sc->sc_iot, PXA2X0_CS2_START, REGMAPSIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "couldn't map registers.\n");
		return;
	}

	sc->sc_okeystat = malloc(sc->sc_nrow * sc->sc_ncolumn, M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	sc->sc_keystat = malloc(sc->sc_nrow * sc->sc_ncolumn, M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (sc->sc_okeystat == NULL || sc->sc_keystat == NULL) {
		aprint_error_dev(self, "couldn't alloc memory.\n");
		if (sc->sc_okeystat)
			free(sc->sc_okeystat, M_DEVBUF);
		if (sc->sc_keystat)
			free(sc->sc_keystat, M_DEVBUF);
		return;
	}

	sc->sc_if.hii_ctx = sc;
	sc->sc_if.hii_establish = wzero3kbd_input_establish;
	sc->sc_if.hii_poll = wzero3kbd_poll;

	/* Attach console if not using serial. */
	if (!(bootinfo->bi_cnuse & BI_CNUSE_SERIAL))
		hpckbd_cnattach(&sc->sc_if);

	/* Install interrupt handler. */
	if (sc->sc_key_pin >= 0) {
		pxa2x0_gpio_set_function(sc->sc_key_pin, GPIO_IN);
		sc->sc_key_ih = pxa2x0_gpio_intr_establish(sc->sc_key_pin,
		    IST_EDGE_BOTH, IPL_TTY, wzero3kbd_intr, sc);
		if (sc->sc_key_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't establish key interrupt\n");
		}
	} else {
		sc->sc_interval = KEY_INTERVAL / (1000 / hz);
		if (sc->sc_interval < 1)
			sc->sc_interval = 1;
		callout_init(&sc->sc_keyscan_ch, 0);
		callout_reset(&sc->sc_keyscan_ch, sc->sc_interval,
		    wzero3kbd_tick, sc);
	}

	/* power key */
	if (sc->sc_power_pin >= 0) {
		pxa2x0_gpio_set_function(sc->sc_power_pin, GPIO_IN);
		sc->sc_power_ih = pxa2x0_gpio_intr_establish(
		    sc->sc_power_pin, IST_EDGE_BOTH, IPL_TTY,
		    wzero3kbd_power_intr, sc);
		if (sc->sc_power_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't establish power key interrupt\n");
		}
	}

	/* reset button */
	if (sc->sc_reset_pin >= 0) {
		pxa2x0_gpio_set_function(sc->sc_reset_pin, GPIO_IN);
		sc->sc_reset_ih = pxa2x0_gpio_intr_establish(
		    sc->sc_reset_pin, IST_EDGE_BOTH, IPL_TTY,
		    wzero3kbd_reset_intr, sc);
		if (sc->sc_reset_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't establish reset key interrupt\n");
		}

		sc->sc_smpsw.smpsw_name = device_xname(self);
		sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_RESET;
		if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to register reset event handler\n");
		}
	}

	/* Attach hpckbd. */
	haa.haa_ic = &sc->sc_if;
	config_found(self, &haa, hpckbd_print);

#if defined(KEYTEST) || defined(KEYTEST2) || defined(KEYTEST3) || defined(KEYTEST4) || defined(KEYTEST5)
	sc->sc_test_ih = NULL;
	sc->sc_test_pin = -1;
	sc->sc_nouse_pin = -1;
	sc->sc_nouse_pin2 = -1;
	sc->sc_nouse_pin3 = -1;
	sc->sc_bit = 0x01;
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS003SH)
	 || platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS004SH)) {
		sc->sc_nouse_pin = GPIO_WS003SH_SD_DETECT;  /* SD_DETECT */
		sc->sc_nouse_pin2 = 86;  /* Vsync? */
		sc->sc_nouse_pin3 = 89;  /* RESET? */
	}
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS007SH)) {
		sc->sc_nouse_pin = GPIO_WS007SH_SD_DETECT;  /* SD_DETECT */
		sc->sc_nouse_pin2 = 77;  /* Vsync? */
	}
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS011SH)) {
		sc->sc_nouse_pin = GPIO_WS011SH_SD_DETECT;  /* SD_DETECT */
		sc->sc_nouse_pin2 = 77;  /* Vsync? */
	}
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS020SH)) {
		sc->sc_nouse_pin = GPIO_WS020SH_SD_DETECT;  /* SD_DETECT */
		sc->sc_nouse_pin2 = 77;  /* Vsync? */
	}

#ifdef KEYTEST
	for (sc->sc_test_pin = 2; sc->sc_test_pin < PXA270_GPIO_NPINS; sc->sc_test_pin++) {
		if (sc->sc_test_pin != sc->sc_nouse_pin
		 && sc->sc_test_pin != sc->sc_nouse_pin2
		 && sc->sc_test_pin != sc->sc_nouse_pin3
		 && sc->sc_test_pin != sc->sc_key_pin
		 && sc->sc_test_pin != sc->sc_power_pin
		 && sc->sc_test_pin != sc->sc_reset_pin
		 && GPIO_IS_GPIO_IN(pxa2x0_gpio_get_function(sc->sc_test_pin)))
			break;
	}
	if (sc->sc_test_pin < PXA270_GPIO_NPINS) {
		printf("GPIO_IN: GPIO pin #%d\n", sc->sc_test_pin);
		sc->sc_test_ih = pxa2x0_gpio_intr_establish(sc->sc_test_pin,
		    IST_EDGE_BOTH, IPL_TTY, wzero3kbd_intr2, sc);
	} else {
		sc->sc_test_pin = -1;
	}
#endif

#ifdef KEYTEST3
	{
		int i;
		printf("pin: ");
		for (i = 0; i < PXA270_GPIO_NPINS; i++) {
			if (i == sc->sc_nouse_pin
			 || i == sc->sc_nouse_pin2
			 || i == sc->sc_nouse_pin3
			 || i == sc->sc_key_pin
			 || i == sc->sc_power_pin
			 || i == sc->sc_reset_pin)
				continue;

			printf("%d, ", i);
			if (GPIO_IS_GPIO_IN(pxa2x0_gpio_get_function(i))) {
				pxa2x0_gpio_intr_establish(i, IST_EDGE_BOTH,
				    IPL_TTY, wzero3kbd_intr3, (void *)(long)i);
			}
		}
	}
#endif

#ifdef KEYTEST4
	for (sc->sc_test_pin = 2; sc->sc_test_pin < PXA270_GPIO_NPINS; sc->sc_test_pin++) {
		if (sc->sc_test_pin != sc->sc_nouse_pin
		 && sc->sc_test_pin != sc->sc_nouse_pin2
		 && sc->sc_test_pin != sc->sc_nouse_pin3
		 && sc->sc_test_pin != sc->sc_key_pin
		 && sc->sc_test_pin != sc->sc_power_pin
		 && sc->sc_test_pin != sc->sc_reset_pin
		 && GPIO_IS_GPIO_OUT(pxa2x0_gpio_get_function(sc->sc_test_pin)))
			break;
	}
	if (sc->sc_test_pin < PXA270_GPIO_NPINS) {
		printf("GPIO_OUT: GPIO pin #%d\n", sc->sc_test_pin);
	} else {
		sc->sc_test_pin = -1;
	}
#endif
#ifdef KEYTEST5
	sc->sc_test_pin = 0x00;
	sc->sc_bit = 0x01;
#endif
#endif
}

static int
wzero3kbd_intr(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

#if defined(KEYTEST) || defined(KEYTEST2) || defined(KEYTEST3) || defined(KEYTEST4) || defined(KEYTEST5)
	printf("wzero3kbd_intr: GPIO pin #%d = %s\n", sc->sc_key_pin,
	    pxa2x0_gpio_get_bit(sc->sc_key_pin) ? "on" : "off");
#endif

#if defined(KEYTEST4)
	if (sc->sc_test_pin >= 0) {
		if (pxa2x0_gpio_get_bit(sc->sc_test_pin)) {
			printf("GPIO_OUT: GPIO pin #%d: L\n",sc->sc_test_pin);
			pxa2x0_gpio_clear_bit(sc->sc_test_pin);
		} else {
			printf("GPIO_OUT: GPIO pin #%d: H\n", sc->sc_test_pin);
			pxa2x0_gpio_set_bit(sc->sc_test_pin);
		}
	}
#endif
#if defined(KEYTEST5)
	printf("CPLD(%#x): value=%#x, mask=%#x\n",
	    sc->sc_test_pin, CSR_READ4(sc->sc_test_pin), sc->sc_bit);
	if (CSR_READ4(sc->sc_test_pin) & sc->sc_bit) {
		printf("CPLD_OUT: CPLD: L\n");
		CSR_WRITE4(sc->sc_test_pin,
		    CSR_READ4(sc->sc_test_pin) & ~sc->sc_bit);
	} else {
		printf("CPLD_OUT: CPLD: H\n");
		CSR_WRITE4(sc->sc_test_pin,
		    CSR_READ4(sc->sc_test_pin) | sc->sc_bit);
	}
#endif

	(void) wzero3kbd_poll1(sc);

	pxa2x0_gpio_clear_intr(sc->sc_key_pin);

	return 1;
}

#if defined(KEYTEST)
static int
wzero3kbd_intr2(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

	printf("wzero3kbd_intr2: GPIO_IN: GPIO pin #%d = %s\n", sc->sc_test_pin,
	    pxa2x0_gpio_get_bit(sc->sc_test_pin) ? "on" : "off");

	return 1;
}
#endif

#if defined(KEYTEST3)
static int
wzero3kbd_intr3(void *arg)
{
	int pin = (int)arg;

	printf("wzero3kbd_intr3: GPIO pin #%d = %s\n", pin,
	    pxa2x0_gpio_get_bit(pin) ? "on" : "off");

	return 1;
}
#endif


static void
wzero3kbd_tick(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

	(void) wzero3kbd_poll1(sc);

	callout_schedule(&sc->sc_keyscan_ch, sc->sc_interval);
}

static int
wzero3kbd_power_intr(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

#if defined(KEYTEST) || defined(KEYTEST2) || defined(KEYTEST3) || defined(KEYTEST4)
	printf("wzero3kbd_power_intr: status = %s\n",
	    pxa2x0_gpio_get_bit(sc->sc_power_pin) ? "on" : "off");
#endif

#if defined(KEYTEST)
	if (pxa2x0_gpio_get_bit(sc->sc_power_pin)) {
		if (sc->sc_test_pin >= 0) {
			int orig_pin = sc->sc_test_pin;
			pxa2x0_gpio_intr_disestablish(sc->sc_test_ih);
			sc->sc_test_ih = NULL;

			for (;;) {
				if (++sc->sc_test_pin >= PXA270_GPIO_NPINS)
					sc->sc_test_pin = 2;
				if (sc->sc_test_pin == orig_pin)
					break;
				if (sc->sc_test_pin != sc->sc_nouse_pin
				 && sc->sc_test_pin != sc->sc_nouse_pin2
				 && sc->sc_test_pin != sc->sc_nouse_pin3
				 && sc->sc_test_pin != sc->sc_key_pin
				 && sc->sc_test_pin != sc->sc_power_pin
				 && sc->sc_test_pin != sc->sc_reset_pin
				 && GPIO_IS_GPIO_IN(pxa2x0_gpio_get_function(sc->sc_test_pin)))
					break;
			}
			if (sc->sc_test_pin != orig_pin) {
				printf("GPIO_IN: GPIO pin #%d\n",
				    sc->sc_test_pin);
				sc->sc_test_ih =
				    pxa2x0_gpio_intr_establish(sc->sc_test_pin,
				    IST_EDGE_BOTH, IPL_TTY, wzero3kbd_intr2,sc);
			} else {
				sc->sc_test_pin = -1;
			}
		}
	}
#endif

#if defined(KEYTEST2)
	if (pxa2x0_gpio_get_bit(sc->sc_power_pin)) {
		sc->sc_enabled ^= 2;
		if (sc->sc_enabled & 2) {
			printf("print col/row\n");
		} else {
			printf("keyscan\n");
		}
	}
#endif
#if defined(KEYTEST4)
	if (pxa2x0_gpio_get_bit(sc->sc_power_pin)) {
		if (sc->sc_test_pin >= 0) {
			int orig_pin = sc->sc_test_pin;
			for (;;) {
				if (++sc->sc_test_pin >= PXA270_GPIO_NPINS)
					sc->sc_test_pin = 2;
				if (sc->sc_test_pin == orig_pin)
					break;
				if (sc->sc_test_pin != sc->sc_nouse_pin
				 && sc->sc_test_pin != sc->sc_nouse_pin2
				 && sc->sc_test_pin != sc->sc_nouse_pin3
				 && sc->sc_test_pin != sc->sc_key_pin
				 && sc->sc_test_pin != sc->sc_power_pin
				 && sc->sc_test_pin != sc->sc_reset_pin
				 && GPIO_IS_GPIO_OUT(pxa2x0_gpio_get_function(sc->sc_test_pin)))
				break;
			}
			if (sc->sc_test_pin != orig_pin) {
				printf("GPIO_OUT: GPIO pin #%d\n", sc->sc_test_pin);
			} else {
				sc->sc_test_pin = -1;
			}
		}
	}
#endif
#if defined(KEYTEST5)
	if (pxa2x0_gpio_get_bit(sc->sc_power_pin)) {
		sc->sc_bit <<= 1;
		if (sc->sc_bit & ~0xff) {
			sc->sc_bit = 0x01;
			sc->sc_test_pin += 0x4;
			if (sc->sc_test_pin >= 0x20) {
				sc->sc_test_pin = 0x00;
			}
		}
		printf("CPLD(%#x), mask=%#x\n", sc->sc_test_pin, sc->sc_bit);
	}
#endif

	pxa2x0_gpio_clear_intr(sc->sc_power_pin);

	return 1;
}

static int
wzero3kbd_reset_intr(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

	sysmon_task_queue_sched(0, wzero3kbd_sysmon_reset_event, sc);

	pxa2x0_gpio_clear_intr(sc->sc_reset_pin);

	return 1;
}

static int
wzero3kbd_input_establish(void *arg, struct hpckbd_if *kbdif)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

	/* Save hpckbd interface. */
	sc->sc_hpckbd = kbdif;
	sc->sc_enabled = 1;

	return 0;
}

static void
wzero3kbd_sysmon_reset_event(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

static int
wzero3kbd_poll(void *arg)
{
	int keydown;

	keydown = wzero3kbd_poll1(arg);

	return keydown;
}

static int
wzero3kbd_poll1(void *arg)
{
	struct wzero3kbd_softc *sc = (struct wzero3kbd_softc *)arg;
	int row, col, data;
	int keycol;
	int keydown;
	int i;
	int s;

	if (!sc->sc_enabled) {
		DPRINTF(("wzero3kbd_poll: disabled\n"));
		return 0;
	}

	s = spltty();

	for (col = 0; col < sc->sc_ncolumn; col++) {
		/* deselect column# and charge */
		CSR_WRITE1(KBDCOL_L, 0);
		CSR_WRITE1(KBDCOL_U, 0);
		CSR_WRITE1(KBDCHARGE, 1);
		delay(KEYWAIT);
		CSR_WRITE1(KBDCHARGE, 0);

		/* select scan column# */
		keycol = 1 << col;
		CSR_WRITE1(KBDCOL_L, keycol & 0xff);
		CSR_WRITE1(KBDCOL_U, keycol >> 8);
		delay(KEYWAIT);
		CSR_WRITE1(KBDCHARGE, 0);

		/* read key data */
		data = CSR_READ1(KBDDATA);
		for (row = 0; row < sc->sc_nrow; row++) {
#ifdef KEYTEST2
			if (!(sc->sc_enabled & 2)) {
#endif
			sc->sc_keystat[row + col * sc->sc_nrow] =
			    (data >> row) & 1;
#ifdef KEYTEST2
			} else if (data & (1 << row)) {
				printf("col = %d, row = %d, idx = %d, data = 0x%02x\n", col, row, row + col * sc->sc_nrow, data);
			}
#endif
		}
	}

	/* deselect column# and charge */
	CSR_WRITE1(KBDCOL_L, 0);
	CSR_WRITE1(KBDCOL_U, 0);
	CSR_WRITE1(KBDCHARGE, 1);
	delay(KEYWAIT);
	CSR_WRITE1(KBDCHARGE, 0);

	/* send key scan code */
	keydown = 0;
	for (i = 0; i < sc->sc_nrow * sc->sc_ncolumn; i++) {
		if (sc->sc_keystat[i] == sc->sc_okeystat[i])
			continue;

		keydown |= sc->sc_keystat[i];
		hpckbd_input(sc->sc_hpckbd, sc->sc_keystat[i], i);
		sc->sc_okeystat[i] = sc->sc_keystat[i];
	}

	splx(s);

	return keydown;
}
