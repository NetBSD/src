/* $NetBSD: si.c,v 1.2 2026/06/11 19:46:18 andvar Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: si.c,v 1.2 2026/06/11 19:46:18 andvar Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/bitops.h>
#include <sys/mutex.h>
#include <sys/tty.h>
#include <sys/queue.h>
#include <uvm/uvm_extern.h>

#include <machine/wii.h>
#include <machine/pio.h>

#include <dev/hid/hidev.h>

#include "locators.h"
#include "mainbus.h"
#include "si.h"
#include "gcpad_rdesc.h"

#define RD4(sc, reg)							\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define SIIOBUF_CLEAR(sc)						\
	bus_space_set_region_4((sc)->sc_bst, (sc)->sc_bsh, SIIOBUF, 0,	\
	    (SIIOBUF_SIZE / 4))
#define SIIOBUF_WR(sc, buf, cnt)					\
	bus_space_write_region_4((sc)->sc_bst, (sc)->sc_bsh, SIIOBUF, buf, cnt)
#define SIIOBUF_RD(sc, buf, cnt)					\
	bus_space_read_region_1((sc)->sc_bst, (sc)->sc_bsh, SIIOBUF, buf, cnt)


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
static void	si_enable_polling(device_t);
static usbd_status si_set_report(void *, int, void *, int);               
static usbd_status si_get_report(void *, int, void *, int);               
static usbd_status si_write(void *, void *, int);                         

static void			tcint_handler(struct work *, void *);
static void			txn_softintr(void *);

static kmutex_t			sicomcsr_qlock;
static void			*txn_si;
static struct work		sicomcsr_work;
static struct workqueue		*sicomcsr_wqp;
static int			txn_len;

TAILQ_HEAD(, sicomcsr_txn) txn_head = TAILQ_HEAD_INITIALIZER(txn_head);

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
	int err;
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
	mutex_init(&sicomcsr_qlock, MUTEX_DEFAULT, IPL_VM);

	err = workqueue_create(&sicomcsr_wqp, "si_tcint", tcint_handler,
	    sc, PRI_NONE, IPL_VM, 0);
	if (err != 0) {
		aprint_normal("si: failed to create workqueue\n");
		sicomcsr_wqp = NULL;
	}

	txn_len = 0;
	txn_si = softint_establish(SOFTINT_SERIAL, txn_softintr, sc);
	KASSERT(txn_si != NULL);

	for (chan = 0; chan < SI_NUM_CHAN; chan++) {
		struct si_channel *ch;
		struct hidev_tag *t;

		ch = &sc->sc_chan[chan];
		ch->ch_sc = sc;
		ch->ch_index = chan;
		ch->ch_gcport_dev = NULL;
		ch->ch_uhid_dev = NULL;
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

		/* Init controller */
		WR4(sc, SICOUTBUF(ch->ch_index), 0x00400300);
	}

	ih = intr_establish_xname(maa->maa_irq, IST_LEVEL, IPL_VM, si_intr, sc,
	    device_xname(self));
	KASSERT(ih != NULL);

	config_interrupts(self, si_enable_polling);
	si_rescan(self, NULL, NULL);
}

static void
si_enable_polling(device_t self)
{
	struct si_softc *sc = device_private(self);
	struct sicomcsr_txn txn;

	/* write to all SICOUTBUF double buffers */
	WR4(sc, SISR, SISR_SICNOUTBUF);

	WR4(sc, SIPOLL,
	    SIPOLL_EN(0) |
	    SIPOLL_EN(1) |
	    SIPOLL_EN(2) |
	    SIPOLL_EN(3) |
	    __SHIFTIN(7, SIPOLL_X) |
	    __SHIFTIN(1, SIPOLL_Y));

	txn_init(&txn);
	txn.comcsr = (
	    RD4(sc, SICOMCSR) |
	    SICOMCSR_RDSTINT |
	    SICOMCSR_RDSTINTMSK |
	    SICOMCSR_TSTART
	);
	txn_enqueue(&txn);
	if (txn_await(&txn) != 0) {
		txn_dequeue(&txn);
		aprint_normal("si: failed to setup polling\n");
	}
	txn_destroy(&txn);
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
		aprint_normal("gcport at %s", pnp);
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
	struct si_channel *ch;
	unsigned chan;
	uint32_t comcsr, sr;
	uint32_t inbuf[2];
	unsigned err, uhid;

	int ret = 0;

	comcsr = RD4(sc, SICOMCSR);
	sr = RD4(sc, SISR);

	if (ISSET(comcsr, SICOMCSR_TCINT)) {
		WR4(sc, SICOMCSR, (comcsr | SICOMCSR_TCINT) & ~SICOMCSR_TSTART);
		workqueue_enqueue(sicomcsr_wqp, &sicomcsr_work, NULL);
		ret = 1;
	}


	if (!ISSET(comcsr, SICOMCSR_RDSTINT)) {
		goto si_intr_done;
	}

	for (chan = 0; chan < SI_NUM_CHAN; chan++) {
		ch = &sc->sc_chan[chan];
		uhid = ch->ch_uhid_dev != NULL;

		if (ISSET(sr, SISR_RDST(chan))) {
			/* clears the sisr by reading inbuf */
			inbuf[0] = RD4(sc, SICINBUFH(chan));
			inbuf[1] = RD4(sc, SICINBUFL(chan));
			err = GCPAD_ERR(inbuf);

			if (uhid) {
				si_make_report(sc, chan, ch->ch_buf, false);
				if (ISSET(ch->ch_state, SI_STATE_OPEN)) {
					softint_schedule(ch->ch_si);
				}
			}

			/*
			 * attach event: non-uhid has no errors
			 * detach event: hid has errors
			 */
			if ((err == uhid) && ch->ch_wqp != NULL) {
				workqueue_enqueue(ch->ch_wqp, &ch->ch_work,
				    NULL);
			}
		}
		ret = 1;
	}

si_intr_done:
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
	int error;

	mutex_enter(&ch->ch_lock);

	if (ISSET(ch->ch_state, SI_STATE_OPEN)) {
		error = EBUSY;
		goto unlock;
	}

	ch->ch_intr = intr;
	ch->ch_intrarg = arg;
	ch->ch_state |= SI_STATE_OPEN;

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

	/* Disable polling */
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


static void
txn_softintr(void *arg)
{
	uint32_t cnt;
	struct si_softc *sc;
	struct si_packet *pk;
	struct sicomcsr_txn *txn;

	sc = arg;

	mutex_enter(&sicomcsr_qlock);
	txn = TAILQ_FIRST(&txn_head);
	pk = txn->pk;
	if(pk != NULL) {
		SIIOBUF_CLEAR(sc);

		/* outsize is number of bytes. we need number of words */
		cnt = ((pk->outsize+3)/4);
		SIIOBUF_WR(sc, pk->out, cnt);

	}

	WR4(sc, SISR, SISR_ERROR_MASK(pk->chan));
	WR4(sc, SICOMCSR,
	    txn->comcsr |
	    SICOMCSR_TCINTMSK |
	    SICOMCSR_RDSTINTMSK |
	    SICOMCSR_TSTART);
	mutex_exit(&sicomcsr_qlock);
}

static void
tcint_handler(struct work *, void *arg)
{
	struct si_softc *sc;
	struct si_packet *pk;
	struct sicomcsr_txn *txn;
	uint32_t comcsr, sisr, sisr_mask, shift_amt, status;
	unsigned chan;

	sc = arg;

	mutex_enter(&sicomcsr_qlock);
	if (TAILQ_EMPTY(&txn_head)) {
		txn_len = 0;
		goto done;
	}

	txn = TAILQ_FIRST(&txn_head);
	mutex_enter(&txn->lock);
	if (txn->status == TXN_READY) {
		txn->status = TXN_DEQUEUED;
		TAILQ_REMOVE(&txn_head, txn, txn_q);
		txn_len--;
		pk = txn->pk;
		if (pk != NULL) {
			chan = pk->chan;
			SIIOBUF_RD(sc, (void *)pk->in, pk->insize);
			comcsr = RD4(sc, SICOMCSR);
			if (ISSET(comcsr, SICOMCSR_COMERR)) {
				sisr = RD4(sc, SISR);
				shift_amt = 8 * (SI_NUM_CHAN - 1 - chan);
				sisr_mask = sisr & SISR_ERROR_MASK(chan);
				status = (sisr_mask >> shift_amt) & 0x3F;
				pk->status = status;
			}

		}
		cv_signal(&txn->cv);
	}
	mutex_exit(&txn->lock);
done:
	if (!TAILQ_EMPTY(&txn_head)) {
		softint_schedule(txn_si);
	}
	mutex_exit(&sicomcsr_qlock);
}

void
txn_init(struct sicomcsr_txn *txn)
{
	cv_init(&txn->cv, "sicomcsr_txn");
	mutex_init(&txn->lock, MUTEX_DEFAULT, IPL_VM);
	txn->status = TXN_READY;
	txn->pk = NULL;
}

int
txn_enqueue(struct sicomcsr_txn *txn)
{
	struct si_packet *pk;

	pk = txn->pk;
	if (pk != NULL) {
		if (pk->chan > 3) {
			return EINVAL;
		}

		if (pk->outsize > SIIOBUF_SIZE || pk->insize > SIIOBUF_SIZE) {
			return EINVAL;
		}

		if (pk->out == NULL || pk->in == NULL) {
			return EINVAL;
		}
	}

	mutex_enter(&sicomcsr_qlock);
	if (txn_len > TXN_MAX) {
		return EBUSY;
	}

	TAILQ_INSERT_TAIL(&txn_head, txn, txn_q);
	txn_len++;

	if (txn_len == 1) {
		softint_schedule(txn_si);
	}
	mutex_exit(&sicomcsr_qlock);

	return 0;
}

void
txn_dequeue(struct sicomcsr_txn *txn)
{
	mutex_enter(&sicomcsr_qlock);
	mutex_enter(&txn->lock);
	if (txn->status != TXN_DEQUEUED) {
		txn->status = TXN_DEQUEUED;
		txn_len--;
		TAILQ_REMOVE(&txn_head, txn, txn_q);
	}
	mutex_exit(&txn->lock);
	mutex_exit(&sicomcsr_qlock);
}

int
txn_await(struct sicomcsr_txn *txn)
{
	struct bintime bt;
	struct timeval tv;
	int err;

	err = 0;
	tv.tv_sec = 0;
	tv.tv_usec = TXN_USEC;
	timeval2bintime(&tv, &bt);

	mutex_enter(&txn->lock);
	while (txn->status == TXN_READY) {
		err = cv_timedwaitbt(&txn->cv, &txn->lock, &bt,
		    DEFAULT_TIMEOUT_EPSILON);
		if (err) {
			KASSERT(err == EWOULDBLOCK);
			break;
		}
	}
	mutex_exit(&txn->lock);
	return err;
}

void
txn_destroy(struct sicomcsr_txn *txn)
{
	cv_destroy(&txn->cv);
	mutex_destroy(&txn->lock);
}
