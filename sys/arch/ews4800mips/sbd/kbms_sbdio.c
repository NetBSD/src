/*	$NetBSD: kbms_sbdio.c,v 1.2.4.1 2007/02/27 16:50:21 yamt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kbms_sbdio.c,v 1.2.4.1 2007/02/27 16:50:21 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <dev/ic/z8530reg.h>

#include <machine/sbdiovar.h>

#include <ews4800mips/dev/ews4800keymap.h>

/* 85C30 keyboard, mouse driver */

struct kbms_reg {
	volatile uint8_t *kbd_csr;
	volatile uint8_t *kbd_data;
	volatile uint8_t *mouse_csr;
	volatile uint8_t *mouse_data;
};

enum { MOUSE_PACKET_LEN = 5 };
struct kbms_softc {
	struct device sc_dv;
	struct device *sc_wskbd;
	struct device *sc_wsmouse;
	struct kbms_reg sc_reg;
	int sc_leds;
	int sc_flags;
	int sc_mouse_sig;
	int sc_mouse_cnt;
	int8_t sc_mouse_buf[MOUSE_PACKET_LEN];
};

int kbms_sbdio_match(struct device *, struct cfdata *, void *);
void kbms_sbdio_attach(struct device *, struct device *, void *);
int kbms_sbdio_intr(void *);

CFATTACH_DECL(kbms_sbdio, sizeof(struct kbms_softc),
    kbms_sbdio_match, kbms_sbdio_attach, NULL, NULL);

int kbd_enable(void *, int);
void kbd_set_leds(void *, int);
int kbd_ioctl(void *, u_long, caddr_t, int, struct lwp *);

int mouse_enable(void *);
void mouse_disable(void *);
int mouse_ioctl(void *, u_long, caddr_t, int, struct lwp *);

bool kbd_init(struct kbms_softc *);
bool kbd_reset(struct kbms_softc *, int);

void mouse_init(struct kbms_softc *);
#ifdef MOUSE_DEBUG
void mouse_debug_print(u_int, int, int);
#endif

int kbd_sbdio_cnattach(uint32_t, uint32_t);
void kbd_cngetc(void *, u_int *, int *);
void kbd_cnpollc(void *, int);

static struct kbms_reg kbms_consreg;

const struct wskbd_consops kbd_consops = {
	kbd_cngetc,
	kbd_cnpollc,
};

const struct wskbd_accessops kbd_accessops = {
	kbd_enable,
	kbd_set_leds,
	kbd_ioctl,
};

struct wskbd_mapdata kbd_keymapdata = {
	ews4800kbd_keydesctab,
	KB_JP,
};

const struct wsmouse_accessops mouse_accessops = {
	mouse_enable,
	mouse_ioctl,
	mouse_disable,
};

#define KBMS_PCLK	(9600 * 512)


int
kbms_sbdio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbdio_attach_args *sa = aux;

	return strcmp(sa->sa_name, "zkbms") ? 0 : 1;
}

void
kbms_sbdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbdio_attach_args *sa = aux;
	struct wsmousedev_attach_args ma;
	struct wskbddev_attach_args ka;
	struct kbms_softc *sc = (void *)self;
	struct kbms_reg *reg = &sc->sc_reg;
	uint8_t *base;

	printf(" at %p irq %d\n", (void *)sa->sa_addr1, sa->sa_irq);

	base = (uint8_t *)MIPS_PHYS_TO_KSEG1(sa->sa_addr1);
	reg->kbd_csr    = base + 0x00;	/* port B */
	reg->kbd_data   = base + 0x04;
	reg->mouse_csr  = base + 0x08;	/* port A */
	reg->mouse_data = base + 0x0c;

	if (reg->kbd_csr  == kbms_consreg.kbd_csr &&
	    reg->kbd_data == kbms_consreg.kbd_data)
		ka.console = true;
	else
		ka.console = false;

	ka.keymap = &kbd_keymapdata;
	ka.accessops = &kbd_accessops;
	ka.accesscookie = self;

	if (kbd_init(sc) == false) {
		printf("keyboard not connected\n");
		return;
	}

	sc->sc_wskbd = config_found(self, &ka, wskbddevprint);

	ma.accessops = &mouse_accessops;
	ma.accesscookie = self;

	if (sa->sa_flags == 0x0001)
		sc->sc_flags = 1;
	sc->sc_mouse_sig = 0x80;
	if (sc->sc_flags == 1)	/* ER/TR?? */
		sc->sc_mouse_sig = 0x88;

	mouse_init(sc);
	sc->sc_wsmouse = config_found(self, &ma, wsmousedevprint);

	intr_establish(sa->sa_irq, kbms_sbdio_intr, self);
}

int
kbms_sbdio_intr(void *arg)
{
	struct kbms_softc *sc = (void *)arg;
	struct kbms_reg *reg = &sc->sc_reg;
	int v;

	if (*reg->kbd_csr & ZSRR0_RX_READY) {
		v = *reg->kbd_data;
		wskbd_input(sc->sc_wskbd,
		    v & 0x80 ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN,
		    v & 0x7f);
	}

	while (*reg->mouse_csr & ZSRR0_RX_READY) {
		int8_t *buf = sc->sc_mouse_buf;
		*reg->mouse_csr = 1;
		if (((v = *reg->mouse_csr) &
		    (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE)) != 0) {
			/* Error occured. re-initialize */
			printf("initialize mouse. error=%02x\n", v);
			mouse_init(sc);
		} else {
			v = *reg->mouse_data;
			if ((sc->sc_mouse_cnt == 0) &&
			    (v & 0xf8) != sc->sc_mouse_sig) {
				printf("missing first packet. reset. %x\n", v);
				mouse_init(sc);
				continue;
			}
			buf[sc->sc_mouse_cnt++] = v;

			if (sc->sc_mouse_cnt == MOUSE_PACKET_LEN) {
				int x, y;
				u_int buttons;
				buttons = ~buf[0] & 0x7;
				if (sc->sc_flags == 0) {
					u_int b1 = (buttons & 0x1) << 2;
					u_int b3 = (buttons & 0x4) >> 2;
					buttons = (buttons & 0x2) | b1 | b3;
				}
				x = buf[1] + buf[3];
				y = buf[2] + buf[4];
#ifdef MOUSE_DEBUG
				mouse_debug_print(buttons, x, y);
#endif
				wsmouse_input(sc->sc_wsmouse,
						buttons,
						x, y, 0, 0,
						WSMOUSE_INPUT_DELTA);
				sc->sc_mouse_cnt = 0;
			}

		}
		*reg->mouse_csr = ZSWR1_REQ_RX | ZSWR1_RIE | ZSWR1_RIE_FIRST;
		(void)*reg->mouse_csr;
	}

	return 0;
}

#define	__REG_WR(r, v)							\
do {									\
	*csr = (r);							\
	delay(1);							\
	*csr = (v);							\
	delay(1);							\
} while (/*CONSTCOND*/ 0)

bool
kbd_init(struct kbms_softc *sc)
{
	struct kbms_reg *reg = &sc->sc_reg;
	volatile uint8_t *csr = reg->kbd_csr;
	int retry = 2;
	int reset_retry = 100;

	do {
		__REG_WR(9, ZSWR9_B_RESET);
		delay(100);
		__REG_WR(9, ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR);
		__REG_WR(1, 0);
		__REG_WR(4, ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_PARENB);
		__REG_WR(12, BPS_TO_TCONST(KBMS_PCLK / 16, 4800));
		__REG_WR(13, 0);
		__REG_WR(5, ZSWR5_TX_8 | ZSWR5_RTS);
		__REG_WR(3, ZSWR3_RX_8);
		__REG_WR(10, 0);
		__REG_WR(11, ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD |
		    ZSWR11_TRXC_OUT_ENA | ZSWR11_TRXC_BAUD);
		__REG_WR(14, ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA);
		__REG_WR(15, 0);
		__REG_WR(5, ZSWR5_TX_8 | ZSWR5_TX_ENABLE);
		__REG_WR(3, ZSWR3_RX_8 | ZSWR3_RX_ENABLE);
		reset_retry *= 2;
	} while (!kbd_reset(sc, reset_retry) && --retry > 0);

	if (retry == 0) {
		printf("keyboard initialize failed.\n");
		return false;
	}

	return true;
}

bool
kbd_reset(struct kbms_softc *sc, int retry)
{
#define	__RETRY_LOOP(x, y)						\
do {									\
	for (i = 0; (x) && (i < retry); i++) {				\
		(void)(y);						\
		delay(10);						\
	}								\
	if (i == retry)							\
		goto error;						\
} while (/*CONSTCOND*/ 0)
	int i;
	struct kbms_reg *reg = &sc->sc_reg;
	volatile uint8_t *csr = reg->kbd_csr;
	volatile uint8_t *data = reg->kbd_data;
	volatile uint8_t dummy;

	__REG_WR(5, ZSWR5_DTR | ZSWR5_TX_8 | ZSWR5_TX_ENABLE);
	delay(100);
	__RETRY_LOOP(*csr & ZSRR0_RX_READY, dummy = *data);
	*csr = 48;
	__REG_WR(5, ZSWR5_TX_8 | ZSWR5_TX_ENABLE | ZSWR5_RTS);
	__RETRY_LOOP((*csr & ZSRR0_RX_READY) == 0, 0);
	*csr = 1;
	__RETRY_LOOP((*csr & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE)) != 0, 0);
	__RETRY_LOOP(*data != 0xa0, 0);
	__RETRY_LOOP((*csr & ZSRR0_RX_READY) == 0, 0);
	*csr = 1;
	__RETRY_LOOP((*csr & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE)) != 0, 0);
	__REG_WR(1, ZSWR1_RIE);

	/* drain buffer */
	(void)*reg->kbd_data;
#undef __RETRY_LOOP
	return true;
 error:
	printf("retry failed.\n");
	return false;
}

void
mouse_init(struct kbms_softc *sc)
{
	struct kbms_reg *reg = &sc->sc_reg;
	volatile uint8_t *csr = reg->mouse_csr;
	volatile uint8_t *data = reg->mouse_data;
	uint8_t d[] = { 0x02, 0x52, 0x53, 0x3b, 0x4d, 0x54, 0x2c, 0x36, 0x0d };
	int i;

	__REG_WR(9, ZSWR9_A_RESET);
	delay(100);
	__REG_WR(9, ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR);
	__REG_WR(1, 0);
	__REG_WR(4, ZSWR4_CLK_X16 | ZSWR4_ONESB);
	if (sc->sc_flags & 0x1) {
		__REG_WR(12, BPS_TO_TCONST(KBMS_PCLK / 16, 4800));
		__REG_WR(13, 0);
	} else {
		__REG_WR(12, BPS_TO_TCONST(KBMS_PCLK / 16, 1200));
		__REG_WR(13, 0);
	}
	__REG_WR(5, ZSWR5_DTR | ZSWR5_TX_8 | ZSWR5_RTS);
	__REG_WR(3, ZSWR3_RX_8);
	__REG_WR(10, 0);
	__REG_WR(11, ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD |
	    ZSWR11_TRXC_OUT_ENA | ZSWR11_TRXC_BAUD);
	__REG_WR(14, ZSWR14_BAUD_FROM_PCLK);
	__REG_WR(15, 0);
	__REG_WR(5, ZSWR5_DTR | ZSWR5_TX_8 | ZSWR5_TX_ENABLE);
	__REG_WR(3, ZSWR3_RX_8 | ZSWR3_RX_ENABLE);

	if (sc->sc_flags & 0x1) {
		for (i = 0; i < sizeof d; i++) {
			while ((*csr & ZSRR0_TX_READY) == 0)
				;
			*data = d[i];
		}
	}

	__REG_WR(1, ZSWR1_RIE);
}

int
kbd_enable(void *arg, int on)
{

	/* always active */
	return 0;
}

void
kbd_set_leds(void *arg, int leds)
{
	struct kbms_softc *sc = arg;
	struct kbms_reg *reg = &sc->sc_reg;

	sc->sc_leds = leds;
	if (leds & WSKBD_LED_CAPS)
		*reg->kbd_data = 0x92;
	else
		*reg->kbd_data = 0x90;
}

int
kbd_ioctl(void *arg, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct kbms_softc *sc = arg;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_EWS4800;
		return 0;
	case WSKBDIO_SETLEDS:
		kbd_set_leds(arg, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_leds;
		return 0;
	case WSKBDIO_BELL:
	case WSKBDIO_COMPLEXBELL:
		return 0;
	}

	return EPASSTHROUGH;
}

int
kbd_sbdio_cnattach(uint32_t csr, uint32_t data)
{
	struct kbms_softc __softc, *sc;
	struct kbms_reg *reg;

	kbms_consreg.kbd_csr  = (void *)csr;
	kbms_consreg.kbd_data = (void *)data;

	/* setup dummy softc for kbd_init() */
	sc = &__softc;
	memset(sc, 0, sizeof(struct kbms_softc));
	reg = &sc->sc_reg;
	reg->kbd_csr  = (void *)csr;
	reg->kbd_data = (void *)data;

	if (kbd_init(sc) == false)
		return false;

	wskbd_cnattach(&kbd_consops, &kbms_consreg, &kbd_keymapdata);
	return true;
}

void
kbd_cngetc(void *arg, u_int *type, int *data)
{
	struct kbms_reg *reg = (void *)arg;
	int v;

	while ((*reg->kbd_csr & ZSRR0_RX_READY) == 0)
		;
	v = *reg->kbd_data;
	*type = v & 0x80 ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	*data = v & 0x7f;
}

void
kbd_cnpollc(void *arg, int on)
{
	static bool __polling = false;
	static int s;

	if (on && !__polling) {
		s = splhigh();  /* Disable interrupt driven I/O */
	} else if (!on && __polling) {
		__polling = false;
	splx(s);        /* Enable interrupt driven I/O */
	}
}

int
mouse_enable(void *arg)
{

	/* always active */
	return 0;
}

void
mouse_disable(void *arg)
{

	/* always active */
}

int
mouse_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct lwp *l)
{

	return EPASSTHROUGH;
}

#ifdef MOUSE_DEBUG
void
mouse_debug_print(u_int buttons, int x, int y)
{
#define	MINMAX(x, min, max)						\
	((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
	static int k, __x, __y;
	int i, j;
	char buf[64];

	__x = MINMAX(__x + x, 0, FB_WIDTH);
	__y = MINMAX(__y + y, 0, FB_HEIGHT);
	*(uint8_t *)(fb.fb_addr + __x + __y * FB_LINEBYTES) = 0xff;

	sprintf(buf, "%8d %8d", x, y);
	for (i = 0; i < 64 && buf[i]; i++)
		fb_drawchar(480 + i * 12, k, buf[i]);

	i += 12;
	for (j = 0x80; j > 0; j >>= 1, i++) {
		fb_drawchar(480 + i * 12, k, buttons & j ? '|' : '.');
	}

	k += 24;
	if (k > 1000)
		k = 0;

}
#endif
