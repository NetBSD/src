/* $NetBSD: si.c,v 1.2.2.2 2025/12/12 18:38:56 martin Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: si.c,v 1.2.2.2 2025/12/12 18:38:56 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bitops.h>
#include <sys/mutex.h>
#include <sys/tty.h>
#include <uvm/uvm_extern.h>

#include <machine/wii.h>
#include <machine/pio.h>

#include <dev/hid/hidev.h>

#include "locators.h"
#include "mainbus.h"
#include "si.h"
#include "gcpad_rdesc.h"

#define SI_NUM_CHAN		4

#define SICOUTBUF(n)		((n) * 0xc + 0x00)
#define SICINBUFH(n)		((n) * 0xc + 0x04)
#define SICINBUFL(n)		((n) * 0xc + 0x08)
#define SIPOLL			0x30
#define  SIPOLL_X		__BITS(25, 16)
#define  SIPOLL_Y		__BITS(15, 8)
#define  SIPOLL_EN(n)		(__BIT(7 - n))
#define SICOMCSR		0x34
#define  SICOMCSR_TCINT		__BIT(31)
#define  SICOMCSR_TCINTMSK	__BIT(30)
#define  SICOMCSR_RDSTINT	__BIT(28)
#define  SICOMCSR_RDSTINTMSK	__BIT(27)
#define  SICOMCSR_OUTLNGTH	__BITS(22, 16)
#define  SICOMCSR_INLNGTH	__BITS(14, 8)
#define  SICOMCSR_TSTART	__BIT(0)
#define SISR			0x38
#define  SISR_OFF(n)		((3 - (n)) * 8)
#define  SISR_WR(n)		__BIT(SISR_OFF(n) + 7)
#define  SISR_RDST(n)		__BIT(SISR_OFF(n) + 5)
#define  SISR_WRST(n)		__BIT(SISR_OFF(n) + 4)
#define  SISR_NOREP(n)		__BIT(SISR_OFF(n) + 3)
#define  SISR_COLL(n)		__BIT(SISR_OFF(n) + 2)
#define  SISR_OVRUN(n)		__BIT(SISR_OFF(n) + 1)
#define  SISR_UNRUN(n)		__BIT(SISR_OFF(n) + 0)
#define  SISR_ERROR_MASK(n)	(SISR_NOREP(n) | SISR_COLL(n) | \
				 SISR_OVRUN(n) | SISR_UNRUN(n))
#define  SISR_ERROR_ACK_ALL	(SISR_ERROR_MASK(0) | SISR_ERROR_MASK(1) | \
				 SISR_ERROR_MASK(2) | SISR_ERROR_MASK(3))
#define SIEXILK			0x3c
#define SIIOBUF			0x80

#define GCPAD_REPORT_SIZE	9
#define GCPAD_START(_buf)	ISSET((_buf)[0], 0x10)
#define GCPAD_Y(_buf)		ISSET((_buf)[0], 0x08)
#define GCPAD_X(_buf)		ISSET((_buf)[0], 0x04)
#define GCPAD_B(_buf)		ISSET((_buf)[0], 0x02)
#define GCPAD_A(_buf)		ISSET((_buf)[0], 0x01)
#define GCPAD_LCLICK(_buf)	ISSET((_buf)[1], 0x40)
#define GCPAD_RCLICK(_buf)	ISSET((_buf)[1], 0x20)
#define GCPAD_Z(_buf)		ISSET((_buf)[1], 0x10)
#define GCPAD_UP(_buf)		ISSET((_buf)[1], 0x08)
#define GCPAD_DOWN(_buf)	ISSET((_buf)[1], 0x04)
#define GCPAD_RIGHT(_buf)	ISSET((_buf)[1], 0x02)
#define GCPAD_LEFT(_buf)	ISSET((_buf)[1], 0x01)

struct si_softc;

struct si_channel {
	struct si_softc		*ch_sc;
	device_t		ch_dev;
	unsigned		ch_index;
	struct hidev_tag	ch_hidev;
	kmutex_t		ch_lock;
	kcondvar_t		ch_cv;
	uint8_t			ch_state;
#define SI_STATE_OPEN		__BIT(0)
#define SI_STATE_STOPPED	__BIT(1)
	void			(*ch_intr)(void *, void *, u_int);
	void			*ch_intrarg;
	uint8_t			ch_buf[GCPAD_REPORT_SIZE];
	void			*ch_desc;
	int			ch_descsize;
	void			*ch_si;
};

struct si_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct si_channel	sc_chan[SI_NUM_CHAN];
};

#define RD4(sc, reg)							\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	si_match(device_t, cfdata_t, void *);
static void	si_attach(device_t, device_t, void *);

static int	si_intr(void *);
static void	si_softintr(void *);

static int	si_rescan(device_t, const char *, const int *);
static int	si_print(void *, const char *);

static void	si_get_report_desc(void *, void **, int *);                   
static int	si_open(void *, void (*)(void *, void *, unsigned), void *);   
static void	si_stop(void *);                                              
static void	si_close(void *);                                             
static usbd_status si_set_report(void *, int, void *, int);               
static usbd_status si_get_report(void *, int, void *, int);               
static usbd_status si_write(void *, void *, int);                         

CFATTACH_DECL_NEW(si, sizeof(struct si_softc),
	si_match, si_attach, NULL, NULL);

static int
si_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return strcmp(maa->maa_name, "si") == 0;
}

static void
si_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const maa = aux;
	struct si_softc * const sc = device_private(self);
	unsigned chan;
	void *ih;

	KASSERT(device_unit(self) == 0);

	aprint_naive("\n");
	aprint_normal(": Serial Interface\n");

	sc->sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	if (bus_space_map(sc->sc_bst, maa->maa_addr, SI_SIZE, 0,
	    &sc->sc_bsh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	for (chan = 0; chan < SI_NUM_CHAN; chan++) {
		struct si_channel *ch;
		struct hidev_tag *t;

		ch = &sc->sc_chan[chan];
		ch->ch_sc = sc;
		ch->ch_index = chan;
		mutex_init(&ch->ch_lock, MUTEX_DEFAULT, IPL_VM);
		cv_init(&ch->ch_cv, "sich");
		ch->ch_si = softint_establish(SOFTINT_SERIAL,
		    si_softintr, ch);
		KASSERT(ch->ch_si != NULL);

		t = &ch->ch_hidev;
		t->_cookie = &sc->sc_chan[chan];
		t->_get_report_desc = si_get_report_desc;
		t->_open = si_open;
		t->_stop = si_stop;
		t->_close = si_close;
		t->_set_report = si_set_report;
		t->_get_report = si_get_report;
		t->_write = si_write;
	}

	WR4(sc, SIPOLL,
	    __SHIFTIN(7, SIPOLL_X) |
	    __SHIFTIN(1, SIPOLL_Y));
	WR4(sc, SICOMCSR, SICOMCSR_RDSTINT | SICOMCSR_RDSTINTMSK);

	ih = intr_establish_xname(maa->maa_irq, IST_LEVEL, IPL_VM, si_intr, sc,
	    device_xname(self));
	KASSERT(ih != NULL);

	si_rescan(self, NULL, NULL);
}

static int
si_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct si_softc * const sc = device_private(self);
	struct si_attach_args saa;
	unsigned chan;

	for (chan = 0; chan < SI_NUM_CHAN; chan++) {
		struct si_channel *ch = &sc->sc_chan[chan];

		if (ch->ch_dev == NULL) {
			saa.saa_hidev = &ch->ch_hidev;
			saa.saa_index = ch->ch_index;

			ch->ch_dev = config_found(self, &saa, si_print,
			    CFARGS(.submatch = config_stdsubmatch,
				   .locators = locs));
		}
	}

	return 0;
}

static int
si_print(void *aux, const char *pnp)
{
	struct si_attach_args *saa = aux;

	if (pnp != NULL) {
		aprint_normal("uhid at %s", pnp);
	}

	/*
	 * The Wii Operations Manual for RVL-001 refers to the controller
	 * ports as "Nintendo GameCube Controller Sockets".
	 */
	aprint_normal(" socket %d", saa->saa_index + 1);

	return UNCONF;
}

static void
si_make_report(struct si_softc *sc, unsigned chan, void *report, bool with_rid)
{
	uint32_t inbuf[2];
	uint8_t *iptr = (uint8_t *)inbuf;
	uint8_t *optr = report;
	unsigned off = 0;

	inbuf[0] = RD4(sc, SICINBUFH(chan));
	inbuf[1] = RD4(sc, SICINBUFL(chan));

	if (with_rid) {
		optr[off++] = chan + 1;
	}

	optr[off] = 0;
	optr[off] |= GCPAD_X(iptr)	? 0x01 : 0;
	optr[off] |= GCPAD_A(iptr)	? 0x02 : 0;
	optr[off] |= GCPAD_B(iptr)	? 0x04 : 0;
	optr[off] |= GCPAD_Y(iptr)	? 0x08 : 0;
	optr[off] |= GCPAD_LCLICK(iptr)	? 0x10 : 0;
	optr[off] |= GCPAD_RCLICK(iptr)	? 0x20 : 0;
	optr[off] |= GCPAD_Z(iptr)	? 0x80 : 0;
	off++;

	optr[off] = 0;
	optr[off] |= GCPAD_START(iptr)	? 0x02 : 0;
	optr[off] |= GCPAD_UP(iptr)	? 0x10 : 0;
	optr[off] |= GCPAD_RIGHT(iptr)	? 0x20 : 0;
	optr[off] |= GCPAD_DOWN(iptr)	? 0x40 : 0;
	optr[off] |= GCPAD_LEFT(iptr)	? 0x80 : 0;
	off++;

	memcpy(&optr[off], &iptr[2], 6);
	off += 6;

	optr[off++] = 0;
}

static void
si_softintr(void *priv)
{
	struct si_channel *ch = priv;

	if (ISSET(ch->ch_state, SI_STATE_OPEN)) {
		ch->ch_intr(ch->ch_intrarg, ch->ch_buf, sizeof(ch->ch_buf));
	}
}

static int
si_intr(void *priv)
{
	struct si_softc *sc = priv;
	unsigned chan;
	uint32_t comcsr, sr;
	int ret = 0;

	comcsr = RD4(sc, SICOMCSR);
	sr = RD4(sc, SISR);

	if (ISSET(comcsr, SICOMCSR_TCINT)) {
		WR4(sc, SICOMCSR, comcsr | SICOMCSR_TCINT);
	}

	if (ISSET(comcsr, SICOMCSR_RDSTINT)) {
		for (chan = 0; chan < SI_NUM_CHAN; chan++) {
			struct si_channel *ch = &sc->sc_chan[chan];

			if (ISSET(sr, SISR_RDST(chan))) {
				/* Reading INBUF[HL] de-asserts RDSTINT. */
				si_make_report(sc, chan, ch->ch_buf, false);

				if (ISSET(ch->ch_state, SI_STATE_OPEN)) {
					softint_schedule(ch->ch_si);
				}
			}

			ret = 1;
		}
	}

	WR4(sc, SISR, sr & SISR_ERROR_ACK_ALL);

	return ret;
}

static void
si_get_report_desc(void *cookie, void **desc, int *size)
{
	*desc = gcpad_report_descr;
	*size = sizeof(gcpad_report_descr);
}

static int
si_open(void *cookie, void (*intr)(void *, void *, u_int), void *arg)
{
	struct si_channel *ch = cookie;
	struct si_softc *sc = ch->ch_sc;
	int error;

	mutex_enter(&ch->ch_lock);

	if (ISSET(ch->ch_state, SI_STATE_OPEN)) {
		error = EBUSY;
		goto unlock;
	}

	ch->ch_intr = intr;
	ch->ch_intrarg = arg;
	ch->ch_state |= SI_STATE_OPEN;

	(void)RD4(sc, SICINBUFH(ch->ch_index));
	(void)RD4(sc, SICINBUFL(ch->ch_index));

	/* Init controller */
	WR4(sc, SICOUTBUF(ch->ch_index), 0x00400300);

	/* Enable polling */
	WR4(sc, SIPOLL, RD4(sc, SIPOLL) | SIPOLL_EN(ch->ch_index));

	WR4(sc, SISR, SISR_WR(ch->ch_index));
	WR4(sc, SICOMCSR, RD4(sc, SICOMCSR) | SICOMCSR_TSTART);

	error = 0;

unlock:
	mutex_exit(&ch->ch_lock);

	return error;
}

static void
si_stop(void *cookie)
{
	struct si_channel *ch = cookie;

	mutex_enter(&ch->ch_lock);

	ch->ch_state |= SI_STATE_STOPPED;

	cv_broadcast(&ch->ch_cv);
	mutex_exit(&ch->ch_lock);
}

static void
si_close(void *cookie)
{
	struct si_channel *ch = cookie;
	struct si_softc *sc = ch->ch_sc;

	mutex_enter(&ch->ch_lock);

	/* Diable polling */
	WR4(sc, SIPOLL, RD4(sc, SIPOLL) & ~SIPOLL_EN(ch->ch_index));

	ch->ch_state &= ~(SI_STATE_OPEN | SI_STATE_STOPPED);
	ch->ch_intr = NULL;
	ch->ch_intrarg = NULL;

	cv_broadcast(&ch->ch_cv);
	mutex_exit(&ch->ch_lock);
}

static usbd_status
si_set_report(void *cookie, int type, void *data, int len)
{
        return USBD_INVAL;
}

static usbd_status
si_get_report(void *cookie, int type, void *data, int len)
{
	struct si_channel *ch = cookie;
	struct si_softc *sc = ch->ch_sc;
	uint32_t *inbuf = data;

	if (len != GCPAD_REPORT_SIZE + 1) {
		return USBD_IOERROR;
	}

	mutex_enter(&ch->ch_lock);
	si_make_report(sc, ch->ch_index, inbuf, true);
	mutex_exit(&ch->ch_lock);

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
si_write(void *cookie, void *data, int len)
{
        return USBD_INVAL;
}
