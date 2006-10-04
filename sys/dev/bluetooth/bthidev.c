/*	$NetBSD: bthidev.c,v 1.5 2006/10/04 19:23:59 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bthidev.c,v 1.5 2006/10/04 19:23:59 plunky Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <prop/proplib.h>

#include <netbt/bluetooth.h>
#include <netbt/l2cap.h>

#include <dev/usb/hid.h>
#include <dev/bluetooth/btdev.h>
#include <dev/bluetooth/bthid.h>
#include <dev/bluetooth/bthidev.h>

#include "locators.h"

/*****************************************************************************
 *
 *	Bluetooth HID device
 */

#define MAX_DESCRIPTOR_LEN	1024		/* sanity check */

/* bthidev softc */
struct bthidev_softc {
	struct btdev		sc_btdev;
	uint16_t		sc_state;
	uint16_t		sc_flags;

	bdaddr_t		sc_laddr;	/* local address */
	bdaddr_t		sc_raddr;	/* remote address */

	uint16_t		sc_ctlpsm;	/* control PSM */
	struct l2cap_channel	*sc_ctl;	/* control channel */
	struct l2cap_channel	*sc_ctl_l;	/* control listen */

	uint16_t		sc_intpsm;	/* interrupt PSM */
	struct l2cap_channel	*sc_int;	/* interrupt channel */
	struct l2cap_channel	*sc_int_l;	/* interrupt listen */

	LIST_HEAD(,bthidev)	sc_list;	/* child list */

	struct callout		sc_reconnect;
	int			sc_attempts;	/* connection attempts */
};

/* sc_flags */
#define BTHID_RECONNECT		(1 << 0)	/* reconnect on link loss */
#define BTHID_CONNECTING	(1 << 1)	/* we are connecting */

/* device state */
#define BTHID_CLOSED		0
#define BTHID_WAIT_CTL		1
#define BTHID_WAIT_INT		2
#define BTHID_OPEN		3
#define BTHID_DETACHING		4

#define	BTHID_RETRY_INTERVAL	5	/* seconds between connection attempts */

/* bthidev internals */
static void bthidev_timeout(void *);
static int  bthidev_listen(struct bthidev_softc *);
static int  bthidev_connect(struct bthidev_softc *);
static int  bthidev_output(struct bthidev *, uint8_t *, int);
static void bthidev_null(struct bthidev *, uint8_t *, int);

/* autoconf(9) glue */
static int  bthidev_match(struct device *, struct cfdata *, void *);
static void bthidev_attach(struct device *, struct device *, void *);
static int  bthidev_detach(struct device *, int);
static int  bthidev_print(void *, const char *);

CFATTACH_DECL(bthidev, sizeof(struct bthidev_softc),
    bthidev_match, bthidev_attach, bthidev_detach, NULL);

/* bluetooth(9) protocol methods for L2CAP */
static void  bthidev_connecting(void *);
static void  bthidev_ctl_connected(void *);
static void  bthidev_int_connected(void *);
static void  bthidev_ctl_disconnected(void *, int);
static void  bthidev_int_disconnected(void *, int);
static void *bthidev_ctl_newconn(void *, struct sockaddr_bt *, struct sockaddr_bt *);
static void *bthidev_int_newconn(void *, struct sockaddr_bt *, struct sockaddr_bt *);
static void  bthidev_complete(void *, int);
static void  bthidev_input(void *, struct mbuf *);

static const struct btproto bthidev_ctl_proto = {
	bthidev_connecting,
	bthidev_ctl_connected,
	bthidev_ctl_disconnected,
	bthidev_ctl_newconn,
	bthidev_complete,
	bthidev_input,
};

static const struct btproto bthidev_int_proto = {
	bthidev_connecting,
	bthidev_int_connected,
	bthidev_int_disconnected,
	bthidev_int_newconn,
	bthidev_complete,
	bthidev_input,
};

/*****************************************************************************
 *
 *	bthidev autoconf(9) routines
 */

static int
bthidev_match(struct device *self, struct cfdata *cfdata, void *aux)
{
	prop_dictionary_t dict = aux;
	prop_object_t obj;

	obj = prop_dictionary_get(dict, BTDEVservice);
	if (prop_string_equals_cstring(obj, "HID"))
		return 1;

	return 0;
}

static void
bthidev_attach(struct device *parent, struct device *self, void *aux)
{
	struct bthidev_softc *sc = (struct bthidev_softc *)self;
	prop_dictionary_t dict = aux;
	prop_object_t obj;
	struct bthidev_attach_args bha;
	struct bthidev *dev;
	struct hid_data *d;
	struct hid_item h;
	const void *desc;
	int locs[BTHIDBUSCF_NLOCS];
	int maxid, rep, s, dlen;

	/*
	 * Init softc
	 */
	LIST_INIT(&sc->sc_list);
	callout_init(&sc->sc_reconnect);
	callout_setfunc(&sc->sc_reconnect, bthidev_timeout, sc);
	sc->sc_state = BTHID_CLOSED;
	sc->sc_flags = BTHID_CONNECTING;
	sc->sc_ctlpsm = L2CAP_PSM_HID_CNTL;
	sc->sc_intpsm = L2CAP_PSM_HID_INTR;

	/*
	 * extract config from proplist
	 */
	obj = prop_dictionary_get(dict, BTDEVladdr);
	bdaddr_copy(&sc->sc_laddr, prop_data_data_nocopy(obj));

	obj = prop_dictionary_get(dict, BTDEVraddr);
	bdaddr_copy(&sc->sc_raddr, prop_data_data_nocopy(obj));

	obj = prop_dictionary_get(dict, BTHIDEVcontrolpsm);
	if (prop_object_type(obj) == PROP_TYPE_NUMBER) {
		sc->sc_ctlpsm = prop_number_integer_value(obj);
		if (L2CAP_PSM_INVALID(sc->sc_ctlpsm)) {
			aprint_error(" invalid %s\n", BTHIDEVcontrolpsm);
			return;
		}
	}

	obj = prop_dictionary_get(dict, BTHIDEVinterruptpsm);
	if (prop_object_type(obj) == PROP_TYPE_NUMBER) {
		sc->sc_intpsm = prop_number_integer_value(obj);
		if (L2CAP_PSM_INVALID(sc->sc_intpsm)) {
			aprint_error(" invalid %s\n", BTHIDEVinterruptpsm);
			return;
		}
	}

	obj = prop_dictionary_get(dict, BTHIDEVdescriptor);
	if (prop_object_type(obj) == PROP_TYPE_DATA) {
		dlen = prop_data_size(obj);
		desc = prop_data_data_nocopy(obj);
	} else {
		aprint_error(" no %s\n", BTHIDEVdescriptor);
		return;
	}

	obj = prop_dictionary_get(dict, BTHIDEVreconnect);
	if (prop_object_type(obj) == PROP_TYPE_BOOL
	    && !prop_bool_true(obj))
		sc->sc_flags |= BTHID_RECONNECT;

	/*
	 * Parse the descriptor and attach child devices, one per report.
	 */
	maxid = -1;
	h.report_ID = 0;
	d = hid_start_parse(desc, dlen, hid_none);
	while (hid_get_item(d, &h)) {
		if (h.report_ID > maxid)
			maxid = h.report_ID;
	}
	hid_end_parse(d);

	if (maxid < 0) {
		aprint_error(" no reports found\n");
		return;
	}

	aprint_normal("\n");

	for (rep = 0 ; rep <= maxid ; rep++) {
		if (hid_report_size(desc, dlen, hid_feature, rep) == 0
		    && hid_report_size(desc, dlen, hid_input, rep) == 0
		    && hid_report_size(desc, dlen, hid_output, rep) == 0)
			continue;

		bha.ba_desc = desc;
		bha.ba_dlen = dlen;
		bha.ba_input = bthidev_null;
		bha.ba_feature = bthidev_null;
		bha.ba_output = bthidev_output;
		bha.ba_id = rep;

		locs[BTHIDBUSCF_REPORTID] = rep;

		dev = (struct bthidev *)config_found_sm_loc((struct device *)sc, "bthidbus",
					locs, &bha, bthidev_print, config_stdsubmatch);
		if (dev != NULL) {
			dev->sc_parent = (struct device *)sc;
			dev->sc_id = rep;
			dev->sc_input = bha.ba_input;
			dev->sc_feature = bha.ba_feature;
			LIST_INSERT_HEAD(&sc->sc_list, dev, sc_next);
		}
	}

	/*
	 * start bluetooth connections
	 */
	s = splsoftnet();
	if ((sc->sc_flags & BTHID_RECONNECT) == 0)
		bthidev_listen(sc);

	if (sc->sc_flags & BTHID_CONNECTING)
		bthidev_connect(sc);
	splx(s);
}

static int
bthidev_detach(struct device *self, int flags)
{
	struct bthidev_softc *sc = (struct bthidev_softc *)self;
	struct bthidev *dev;
	int s;

	s = splsoftnet();
	sc->sc_flags = 0;	/* disable reconnecting */

	/* release interrupt listen */
	if (sc->sc_int_l != NULL) {
		l2cap_detach(&sc->sc_int_l);
		sc->sc_int_l = NULL;
	}

	/* release control listen */
	if (sc->sc_ctl_l != NULL) {
		l2cap_detach(&sc->sc_ctl_l);
		sc->sc_ctl_l = NULL;
	}

	/* close interrupt channel */
	if (sc->sc_int != NULL) {
		l2cap_disconnect(sc->sc_int, 0);
		l2cap_detach(&sc->sc_int);
		sc->sc_int = NULL;
	}

	/* close control channel */
	if (sc->sc_ctl != NULL) {
		l2cap_disconnect(sc->sc_ctl, 0);
		l2cap_detach(&sc->sc_ctl);
		sc->sc_ctl = NULL;
	}

	/* remove callout */
	sc->sc_state = BTHID_DETACHING;
	callout_stop(&sc->sc_reconnect);
	if (callout_invoking(&sc->sc_reconnect))
		tsleep(sc, PWAIT, "bthidetach", 0);

	splx(s);

	/* detach children */
	while ((dev = LIST_FIRST(&sc->sc_list)) != NULL) {
		LIST_REMOVE(dev, sc_next);
		config_detach((struct device *)dev, flags);
	}

	return 0;
}

/*
 * bthidev config print
 */
static int
bthidev_print(void *aux, const char *pnp)
{
	struct bthidev_attach_args *ba = aux;

	if (pnp != NULL)
		aprint_normal("%s:", pnp);

	if (ba->ba_id > 0)
		aprint_normal(" reportid %d", ba->ba_id);

	return UNCONF;
}

/*****************************************************************************
 *
 *	bluetooth(4) HID attach/detach routines
 */

/*
 * callouts are scheduled after connections have been lost, in order
 * to clean up and reconnect.
 */
static void
bthidev_timeout(void *arg)
{
	struct bthidev_softc *sc = arg;
	int s;

	s = splsoftnet();
	callout_ack(&sc->sc_reconnect);

	switch (sc->sc_state) {
	case BTHID_CLOSED:
		if (sc->sc_int != NULL) {
			l2cap_disconnect(sc->sc_int, 0);
			break;
		}

		if (sc->sc_ctl != NULL) {
			l2cap_disconnect(sc->sc_ctl, 0);
			break;
		}

		if (sc->sc_flags & BTHID_RECONNECT) {
			sc->sc_flags |= BTHID_CONNECTING;
			bthidev_connect(sc);
			break;
		}

		break;

	case BTHID_WAIT_CTL:
		break;

	case BTHID_WAIT_INT:
		break;

	case BTHID_OPEN:
		break;

	case BTHID_DETACHING:
		wakeup(sc);
		break;

	default:
		break;
	}
	splx(s);
}

/*
 * listen for our device
 */
static int
bthidev_listen(struct bthidev_softc *sc)
{
	struct sockaddr_bt sa;
	int err;

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, &sc->sc_laddr);

	/*
	 * Listen on control PSM
	 */
	err = l2cap_attach(&sc->sc_ctl_l, &bthidev_ctl_proto, sc);
	if (err)
		return err;

	sa.bt_psm = sc->sc_ctlpsm;
	err = l2cap_bind(sc->sc_ctl_l, &sa);
	if (err)
		return err;

	err = l2cap_listen(sc->sc_ctl_l);
	if (err)
		return err;

	/*
	 * Listen on interrupt PSM
	 */
	err = l2cap_attach(&sc->sc_int_l, &bthidev_int_proto, sc);
	if (err)
		return err;

	sa.bt_psm = sc->sc_intpsm;
	err = l2cap_bind(sc->sc_int_l, &sa);
	if (err)
		return err;

	err = l2cap_listen(sc->sc_int_l);
	if (err)
		return err;

	sc->sc_state = BTHID_WAIT_CTL;
	return 0;
}

/*
 * start connecting to our device
 */
static int
bthidev_connect(struct bthidev_softc *sc)
{
	struct sockaddr_bt sa;
	int err;

	if (sc->sc_attempts++ > 0)
		printf("%s: connect (#%d)\n",
			device_xname((struct device *)sc), sc->sc_attempts);

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;

	err = l2cap_attach(&sc->sc_ctl, &bthidev_ctl_proto, sc);
	if (err) {
		printf("%s: l2cap_attach failed (%d)\n",
			device_xname((struct device *)sc), err);
		return err;
	}

	bdaddr_copy(&sa.bt_bdaddr, &sc->sc_laddr);
	err = l2cap_bind(sc->sc_ctl, &sa);
	if (err) {
		printf("%s: l2cap_bind failed (%d)\n",
			device_xname((struct device *)sc), err);
		return err;
	}

	sa.bt_psm = sc->sc_ctlpsm;
	bdaddr_copy(&sa.bt_bdaddr, &sc->sc_raddr);
	err = l2cap_connect(sc->sc_ctl, &sa);
	if (err) {
		printf("%s: l2cap_connect failed (%d)\n",
			device_xname((struct device *)sc), err);
		return err;
	}

	sc->sc_state = BTHID_WAIT_CTL;
	return 0;
}

/*****************************************************************************
 *
 *	bluetooth(9) callback methods for L2CAP
 *
 *	All these are called from Bluetooth Protocol code, in a soft
 *	interrupt context at IPL_SOFTNET.
 */

static void
bthidev_connecting(void *arg)
{

	/* dont care */
}

static void
bthidev_ctl_connected(void *arg)
{
	struct sockaddr_bt sa;
	struct bthidev_softc *sc = arg;
	int err;

	if (sc->sc_state != BTHID_WAIT_CTL)
		return;

	KASSERT(sc->sc_ctl != NULL);
	KASSERT(sc->sc_int == NULL);

	if (sc->sc_flags & BTHID_CONNECTING) {
		/* initiate connect on interrupt PSM */
		err = l2cap_attach(&sc->sc_int, &bthidev_int_proto, sc);
		if (err)
			goto fail;

		memset(&sa, 0, sizeof(sa));
		sa.bt_len = sizeof(sa);
		sa.bt_family = AF_BLUETOOTH;
		bdaddr_copy(&sa.bt_bdaddr, &sc->sc_laddr);

		err = l2cap_bind(sc->sc_int, &sa);
		if (err)
			goto fail;

		sa.bt_psm = sc->sc_intpsm;
		bdaddr_copy(&sa.bt_bdaddr, &sc->sc_raddr);
		err = l2cap_connect(sc->sc_int, &sa);
		if (err)
			goto fail;
	}

	sc->sc_state = BTHID_WAIT_INT;
	return;

fail:
	l2cap_detach(&sc->sc_ctl);
	sc->sc_ctl = NULL;

	printf("%s: connect failed (%d)\n",
		device_xname((struct device *)sc), err);
}

static void
bthidev_int_connected(void *arg)
{
	struct bthidev_softc *sc = arg;

	if (sc->sc_state != BTHID_WAIT_INT)
		return;

	KASSERT(sc->sc_ctl != NULL);
	KASSERT(sc->sc_int != NULL);

	sc->sc_attempts = 0;
	sc->sc_flags &= ~BTHID_CONNECTING;
	sc->sc_state = BTHID_OPEN;

	printf("%s: connected\n", device_xname((struct device *)sc));
}

/*
 * Disconnected
 *
 * Depending on our state, this could mean several things, but essentially
 * we are lost. If both channels are closed, and we are marked to reconnect,
 * schedule another try otherwise just give up. They will contact us.
 */
static void
bthidev_ctl_disconnected(void *arg, int err)
{
	struct bthidev_softc *sc = arg;

	if (sc->sc_ctl != NULL) {
		l2cap_detach(&sc->sc_ctl);
		sc->sc_ctl = NULL;
	}

	sc->sc_state = BTHID_CLOSED;

	if (sc->sc_int == NULL) {
		printf("%s: disconnected\n",
			device_xname((struct device *)sc));
		sc->sc_flags &= ~BTHID_CONNECTING;

		if (sc->sc_flags & BTHID_RECONNECT)
			callout_schedule(&sc->sc_reconnect,
					BTHID_RETRY_INTERVAL * hz);
		else
			sc->sc_state = BTHID_WAIT_CTL;
	} else {
		/*
		 * The interrupt channel should have been closed first,
		 * but its potentially unsafe to detach that from here.
		 * Give them a second to do the right thing or let the
		 * callout handle it.
		 */
		callout_schedule(&sc->sc_reconnect, hz);
	}
}

static void
bthidev_int_disconnected(void *arg, int err)
{
	struct bthidev_softc *sc = arg;

	if (sc->sc_int != NULL) {
		l2cap_detach(&sc->sc_int);
		sc->sc_int = NULL;
	}

	sc->sc_state = BTHID_CLOSED;

	if (sc->sc_ctl == NULL) {
		printf("%s: disconnected\n",
			device_xname((struct device *)sc));
		sc->sc_flags &= ~BTHID_CONNECTING;

		if (sc->sc_flags & BTHID_RECONNECT)
			callout_schedule(&sc->sc_reconnect,
					BTHID_RETRY_INTERVAL * hz);
		else
			sc->sc_state = BTHID_WAIT_CTL;
	} else {
		/*
		 * The control channel should be closing also, allow
		 * them a chance to do that before we force it.
		 */
		callout_schedule(&sc->sc_reconnect, hz);
	}
}

/*
 * New Connections
 *
 * We give a new L2CAP handle back if this matches the BDADDR we are
 * listening for and we are in the right state. bthidev_connected will
 * be called when the connection is open, so nothing else to do here
 */
static void *
bthidev_ctl_newconn(void *arg, struct sockaddr_bt *laddr, struct sockaddr_bt *raddr)
{
	struct bthidev_softc *sc = arg;

	if (bdaddr_same(&raddr->bt_bdaddr, &sc->sc_raddr) == 0
	    || (sc->sc_flags & BTHID_CONNECTING)
	    || sc->sc_state != BTHID_WAIT_CTL
	    || sc->sc_ctl != NULL
	    || sc->sc_int != NULL)
		return NULL;

	l2cap_attach(&sc->sc_ctl, &bthidev_ctl_proto, sc);
	return sc->sc_ctl;
}

static void *
bthidev_int_newconn(void *arg, struct sockaddr_bt *laddr, struct sockaddr_bt *raddr)
{
	struct bthidev_softc *sc = arg;

	if (bdaddr_same(&raddr->bt_bdaddr, &sc->sc_raddr) == 0
	    || (sc->sc_flags & BTHID_CONNECTING)
	    || sc->sc_state != BTHID_WAIT_INT
	    || sc->sc_ctl == NULL
	    || sc->sc_int != NULL)
		return NULL;

	l2cap_attach(&sc->sc_int, &bthidev_int_proto, sc);
	return sc->sc_int;
}

static void
bthidev_complete(void *arg, int count)
{

	/* dont care */
}

/*
 * Receive reports from the protocol stack.
 */
static void
bthidev_input(void *arg, struct mbuf *m)
{
	struct bthidev_softc *sc = arg;
	struct bthidev *dev;
	uint8_t *data;
	int len;

	if (sc->sc_state != BTHID_OPEN)
		goto release;

	if (m->m_pkthdr.len > m->m_len)
		printf("%s: truncating HID report\n",
			device_xname((struct device *)sc));

	len = m->m_len;
	data = mtod(m, uint8_t *);

	if (BTHID_TYPE(data[0]) == BTHID_DATA) {
		/*
		 * data[0] == type / parameter
		 * data[1] == id
		 * data[2..len] == report
		 */
		if (len < 3)
			goto release;

		LIST_FOREACH(dev, &sc->sc_list, sc_next) {
			if (data[1] == dev->sc_id) {
				switch (BTHID_DATA_PARAM(data[0])) {
				case BTHID_DATA_INPUT:
					(*dev->sc_input)(dev, data + 2, len - 2);
					break;

				case BTHID_DATA_FEATURE:
					(*dev->sc_feature)(dev, data + 2, len - 2);
					break;

				default:
					break;
				}

				goto release;
			}
		}
		printf("%s: report id %d, len = %d ignored\n",
			device_xname((struct device *)sc), data[1], len - 2);

		goto release;
	}

	if (BTHID_TYPE(data[0]) == BTHID_CONTROL) {
		if (len < 1)
			goto release;

		if (BTHID_DATA_PARAM(data[0]) == BTHID_CONTROL_UNPLUG) {
			printf("%s: unplugged\n",
				device_xname((struct device *)sc));

			/* close interrupt channel */
			if (sc->sc_int != NULL) {
				l2cap_disconnect(sc->sc_int, 0);
				l2cap_detach(&sc->sc_int);
				sc->sc_int = NULL;
			}

			/* close control channel */
			if (sc->sc_ctl != NULL) {
				l2cap_disconnect(sc->sc_ctl, 0);
				l2cap_detach(&sc->sc_ctl);
				sc->sc_ctl = NULL;
			}
		}

		goto release;
	}

release:
	m_freem(m);
}

/*****************************************************************************
 *
 *	IO routines
 */

static void
bthidev_null(struct bthidev *dev, uint8_t *report, int len)
{

	/*
	 * empty routine just in case the device
	 * provided no method to handle this report
	 */
}

static int
bthidev_output(struct bthidev *dev, uint8_t *report, int rlen)
{
	struct bthidev_softc *sc = (struct bthidev_softc *)dev->sc_parent;
	struct mbuf *m;
	int s, err;

	if (sc == NULL || sc->sc_state != BTHID_OPEN)
		return ENOTCONN;

	KASSERT(sc->sc_ctl != NULL);
	KASSERT(sc->sc_int != NULL);

	if (rlen == 0 || report == NULL)
		return 0;

	if (rlen > MHLEN - 2) {
		printf("%s: output report too long (%d)!\n",
				device_xname((struct device *)sc), rlen);

		return EMSGSIZE;
	}

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;

	/*
	 * data[0] = type / parameter
	 * data[1] = id
	 * data[2..N] = report
	 */
	mtod(m, uint8_t *)[0] = (uint8_t)((BTHID_DATA << 4) | BTHID_DATA_OUTPUT);
	mtod(m, uint8_t *)[1] = dev->sc_id;
	memcpy(mtod(m, uint8_t *) + 2, report, rlen);
	m->m_pkthdr.len = m->m_len = rlen + 2;

	s = splsoftnet();
	err = l2cap_send(sc->sc_int, m);
	splx(s);

	return err;
}
