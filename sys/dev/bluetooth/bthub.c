/*	$NetBSD: bthub.c,v 1.1.2.5 2007/11/15 11:44:05 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bthub.c,v 1.1.2.5 2007/11/15 11:44:05 yamt Exp $");

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

#include <dev/bluetooth/btdev.h>

#include "ioconf.h"

/*****************************************************************************
 *
 *	Bluetooth Device Hub
 */

struct bthub_softc {
	device_t		sc_dev;
	LIST_HEAD(,btdev)	sc_list;
};

/* autoconf(9) glue */
static int	bthub_match(device_t, struct cfdata *, void *);
static void	bthub_attach(device_t, device_t, void *);
static int	bthub_detach(device_t, int);

CFATTACH_DECL_NEW(bthub, sizeof(struct bthub_softc),
    bthub_match, bthub_attach, bthub_detach, NULL);

/* control file */
dev_type_ioctl(bthubioctl);

const struct cdevsw bthub_cdevsw = {
	nullopen, nullclose, noread, nowrite, bthubioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER,
};

/* bthub functions */
static int	bthub_print(void *, const char *);
static int	bthub_pioctl(dev_t, unsigned long, prop_dictionary_t, int, struct lwp *);

/*****************************************************************************
 *
 *	bthub autoconf(9) routines
 *
 *	A Hub is attached to each Bluetooth Controller as it is enabled
 */

static int
bthub_match(device_t self, struct cfdata *cfdata, void *arg)
{

	return 1;
}

static void
bthub_attach(device_t parent, device_t self, void *aux)
{
	struct bthub_softc *sc = device_private(self);
	bdaddr_t *addr = aux;
	prop_dictionary_t dict;
	prop_object_t obj;

	LIST_INIT(&sc->sc_list);
	sc->sc_dev = self;

	dict = device_properties(self);
	obj = prop_data_create_data(addr, sizeof(*addr));
	prop_dictionary_set(dict, BTDEVladdr, obj);
	prop_object_release(obj);

	aprint_verbose(" %s %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
			BTDEVladdr,
			addr->b[5], addr->b[4], addr->b[3],
			addr->b[2], addr->b[1], addr->b[0]);

	aprint_normal("\n");
}

static int
bthub_detach(device_t self, int flags)
{
	struct bthub_softc *sc = device_private(self);
	struct btdev *btdev;
	int err;

	while (!LIST_EMPTY(&sc->sc_list)) {
		btdev = LIST_FIRST(&sc->sc_list);
		LIST_REMOVE(btdev, sc_next);

		err = config_detach(btdev->sc_dev, flags);
		if (err && (flags & DETACH_FORCE) == 0) {
			LIST_INSERT_HEAD(&sc->sc_list, btdev, sc_next);
			return err;
		}
	}

	return 0;
}


/*****************************************************************************
 *
 *	bthub access functions to control device
 */

int
bthubioctl(dev_t devno, unsigned long cmd, void *data, int flag, struct lwp *l)
{
	prop_dictionary_t dict;
	int err;

	switch(cmd) {
	case BTDEV_ATTACH:
	case BTDEV_DETACH:
		/* load dictionary */
		err = prop_dictionary_copyin_ioctl(data, cmd, &dict);
		if (err == 0) {
			err = bthub_pioctl(devno, cmd, dict, flag, l);
			prop_object_release(dict);
		}
		break;

	default:
		err = EPASSTHROUGH;
		break;
	}

	return err;
}

static int
bthub_pioctl(dev_t devno, unsigned long cmd, prop_dictionary_t dict,
    int flag, struct lwp *l)
{
	struct bthub_softc *sc;
	struct btdev *btdev;
	prop_data_t laddr, raddr;
	prop_string_t service;
	prop_dictionary_t prop;
	prop_object_t obj;
	device_t dev;
	int unit;

	/* validate local address */
	laddr = prop_dictionary_get(dict, BTDEVladdr);
	if (prop_data_size(laddr) != sizeof(bdaddr_t))
		return EINVAL;

	/* locate the relevant bthub */
	for (unit = 0 ; ; unit++) {
		if (unit == bthub_cd.cd_ndevs)
			return ENXIO;

		dev = bthub_cd.cd_devs[unit];
		if (dev == NULL)
			continue;

		prop = device_properties(dev);
		obj = prop_dictionary_get(prop, BTDEVladdr);
		if (prop_data_equals(laddr, obj))
			break;
	}
	sc = device_private(dev);

	/* validate remote address */
	raddr = prop_dictionary_get(dict, BTDEVraddr);
	if (prop_data_size(raddr) != sizeof(bdaddr_t)
	    || bdaddr_any(prop_data_data_nocopy(raddr)))
		return EINVAL;

	/* validate service name */
	service = prop_dictionary_get(dict, BTDEVservice);
	if (prop_object_type(service) != PROP_TYPE_STRING)
		return EINVAL;

	/* locate matching child device, if any */
	LIST_FOREACH(btdev, &sc->sc_list, sc_next) {
		prop = device_properties(btdev->sc_dev);

		obj = prop_dictionary_get(prop, BTDEVraddr);
		if (!prop_object_equals(raddr, obj))
			continue;

		obj = prop_dictionary_get(prop, BTDEVservice);
		if (!prop_object_equals(service, obj))
			continue;

		break;
	}

	switch (cmd) {
	case BTDEV_ATTACH:	/* attach BTDEV */
		if (btdev != NULL)
			return EADDRINUSE;

		dev = config_found(sc->sc_dev, dict, bthub_print);
		if (dev == NULL)
			return ENXIO;

		prop = device_properties(dev);
		prop_dictionary_set(prop, BTDEVladdr, laddr);
		prop_dictionary_set(prop, BTDEVraddr, raddr);
		prop_dictionary_set(prop, BTDEVservice, service);

		btdev = device_private(dev);
		btdev->sc_dev = dev;
		LIST_INSERT_HEAD(&sc->sc_list, btdev, sc_next);
		break;

	case BTDEV_DETACH:	/* detach BTDEV */
		if (btdev == NULL)
			return ENXIO;

		LIST_REMOVE(btdev, sc_next);
		config_detach(btdev->sc_dev, DETACH_FORCE);
		break;
	}

	return 0;
}

static int
bthub_print(void *aux, const char *pnp)
{
	prop_dictionary_t dict = aux;
	prop_object_t obj;
	const bdaddr_t *raddr;

	if (pnp != NULL) {
		obj = prop_dictionary_get(dict, BTDEVtype);
		aprint_normal("%s: %s '%s',", pnp, BTDEVtype,
					prop_string_cstring_nocopy(obj));
	}

	obj = prop_dictionary_get(dict, BTDEVraddr);
	raddr = prop_data_data_nocopy(obj);

	aprint_verbose(" %s %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
			BTDEVraddr,
			raddr->b[5], raddr->b[4], raddr->b[3],
			raddr->b[2], raddr->b[1], raddr->b[0]);

	return UNCONF;
}
