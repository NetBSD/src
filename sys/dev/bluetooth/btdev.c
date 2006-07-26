/*	$NetBSD: btdev.c,v 1.1 2006/07/26 10:38:51 tron Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: btdev.c,v 1.1 2006/07/26 10:38:51 tron Exp $");

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

/*****************************************************************************
 *
 *	Bluetooth device
 *
 *	Each device corresponds to a character device /dev/btdevN
 */

#define BTDEV_DEFAULT_COUNT	4	/* default number of btdevs */

struct btdev_softc {
	struct device	 sc_dev;
	int		 sc_busy;
	struct device	*sc_child;
};

/* pseudo-device initialization */
void	btdevattach(int);

/* autoconf(9) glue */
static int	btdev_match(struct device *, struct cfdata *, void *);
static void	btdev_attach(struct device *, struct device *, void *);

CFATTACH_DECL(btdev, sizeof(struct btdev_softc),
    btdev_match, btdev_attach, NULL, NULL);

/* device control */
dev_type_open(btdevopen);
dev_type_close(btdevclose);
dev_type_ioctl(btdevioctl);

const struct cdevsw btdev_cdevsw = {
	btdevopen, btdevclose, noread, nowrite, btdevioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

/* print attach args */
static int	btdev_print(void *, const char *);

/*****************************************************************************
 *
 *	btdev init routine called at system boot. attach proper devices
 *	for each requested pseudo so we can pass config args later.
 */

extern struct cfdriver btdev_cd;

static struct cfdata btdev_cfdata = {
	.cf_name =	"btdev",
	.cf_atname =	"btdev",
	.cf_unit =	DVUNIT_ANY,
	.cf_fstate =	FSTATE_STAR,
};

void
btdevattach(int num)
{
	int err, i;

	if (num < 1)
		num = BTDEV_DEFAULT_COUNT;

	err = config_cfattach_attach(btdev_cd.cd_name, &btdev_ca);
	if (err) {
		aprint_error("%s: unable to register cfattach (%d)\n",
			btdev_cd.cd_name, err);

		config_cfdriver_detach(&btdev_cd);
		return;
	}

	for (i = 0 ; i < num ; i++) {
		if (config_attach_pseudo(&btdev_cfdata) == NULL) {
			aprint_error("%s%d: attach failed (%d)\n",
				btdev_cd.cd_name, i, err);

			return;
		}
	}
}

/*****************************************************************************
 *
 *	btdev autoconf(9) routines
 *
 */

static int
btdev_match(struct device *self, struct cfdata *cfdata, void *arg)
{

	return 1;
}

static void
btdev_attach(struct device *parent, struct device *self, void *aux)
{
}


/*****************************************************************************
 *
 *	btdev access functions to control device
 */

int
btdevopen(dev_t devno, int flag, int mode, struct lwp *l)
{
	struct btdev_softc *sc;

	sc = device_lookup(&btdev_cd, minor(devno));
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_busy)
		return EBUSY;

	sc->sc_busy = 1;
	return 0;
}

int
btdevclose(dev_t devno, int flag, int mode, struct lwp *l)
{
	struct btdev_softc *sc;

	sc = device_lookup(&btdev_cd, minor(devno));
	sc->sc_busy = 0;
	return 0;
}

int
btdevioctl(dev_t devno, unsigned long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct btdev_softc *sc;
	prop_dictionary_t dict;
	prop_object_t obj;
	const struct plistref *plist;
	int err;

	plist = (const struct plistref *)data;
	sc = device_lookup(&btdev_cd, minor(devno));
	dict = NULL;
	err = 0;

	switch (cmd) {
	case BTDEV_ATTACH:	/* attach BTDEV */
		if (sc->sc_child) {
			err = EBUSY;
			break;
		}

		/*
		 * load dictionary and validate common items
		 */
		err = prop_dictionary_copyin_ioctl(plist, flag, &dict);
		if (err)
			break;

		obj = prop_dictionary_get(dict, "local-bdaddr");
		if (obj == NULL
		    || prop_object_type(obj) != PROP_TYPE_DATA
		    || prop_data_size(obj) != sizeof(bdaddr_t)) {
			err = EINVAL;
			break;
		}

		obj = prop_dictionary_get(dict, "remote-bdaddr");
		if (obj == NULL
		    || prop_object_type(obj) != PROP_TYPE_DATA
		    || prop_data_size(obj) != sizeof(bdaddr_t)
		    || bdaddr_any(prop_data_data_nocopy(obj))) {
			err = EINVAL;
			break;
		}

		obj = prop_dictionary_get(dict, "device-type");
		if (obj == NULL
		    || prop_object_type(obj) != PROP_TYPE_STRING) {
			err = EINVAL;
			break;
		}

		sc->sc_child = config_found(&sc->sc_dev, dict, btdev_print);
		if (sc->sc_child == NULL)
			err = ENXIO;

		break;

	case BTDEV_DETACH:	/* detach BTDEV */
		if (sc->sc_child == NULL) {
			err = ENXIO;
			break;
		}

		config_detach(sc->sc_child, DETACH_FORCE);
		sc->sc_child = NULL;
		break;

	default:
		err = EPASSTHROUGH;
		break;
	}

	if (dict != NULL)
		prop_object_release(dict);

	return err;
}

static int
btdev_print(void *aux, const char *pnp)
{
	prop_dictionary_t dict = aux;
	prop_object_t obj;
	const bdaddr_t *raddr;

	if (pnp != NULL) {
		obj = prop_dictionary_get(dict, "device-type");
		aprint_normal("%s: type '%s',", pnp, prop_string_cstring_nocopy(obj));
	}

	obj = prop_dictionary_get(dict, "remote-bdaddr");
	raddr = prop_data_data_nocopy(obj);

	aprint_verbose(" bdaddr %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
			raddr->b[5], raddr->b[4], raddr->b[3],
			raddr->b[2], raddr->b[1], raddr->b[0]);

	return UNCONF;
}
