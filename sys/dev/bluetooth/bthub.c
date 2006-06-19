/*	$NetBSD: bthub.c,v 1.1 2006/06/19 15:44:45 gdamore Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bthub.c,v 1.1 2006/06/19 15:44:45 gdamore Exp $");

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

#include <netbt/bluetooth.h>
#include <netbt/l2cap.h>

#include <dev/bluetooth/btdev.h>

/*****************************************************************************
 *
 *	Bluetooth HUB pseudo-device
 *
 *	The Bluetooth hub is a conventent place to hang devices that are
 *	actually sitting on top of the protocol stack, and not necessarily
 *	attached to any particular bluetooth interface.
 */

struct bthub_softc {
	struct device		sc_dev;
	LIST_HEAD(,btdev)	sc_list;
};

/* our pseudo-device hook */
static struct bthub_softc *hook;

/* pseudo-device initialization */
void	bthubattach(int);

/* autoconf(9) glue */
static int	bthub_match(struct device *, struct cfdata *, void *);
static void	bthub_attach(struct device *, struct device *, void *);
static int	bthub_detach(struct device *, int);
static int	bthub_print(void *, const char *);

CFATTACH_DECL(bthub, sizeof(struct bthub_softc),
    bthub_match, bthub_attach, bthub_detach, NULL);

/* pseudo-device control */
dev_type_ioctl(bthubioctl);

const struct cdevsw bthub_cdevsw = {
	nullopen, nullclose, noread, nowrite, bthubioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

/*****************************************************************************
 *
 *	bthub init routine called at system boot
 *
 *	we take this opportunity to hang a real device on our hook
 *	so that we can operate like a proper parent to our children.
 */

extern struct cfdriver bthub_cd;

static struct cfdata bthub_cfdata = {
	.cf_name =	"bthub",
	.cf_atname =	"bthub",
	.cf_unit =	DVUNIT_ANY,
	.cf_fstate =	FSTATE_STAR,
};

void
bthubattach(int num)
{
	int err;

	err = config_cfattach_attach(bthub_cd.cd_name, &bthub_ca);
	if (err) {
		aprint_error("%s: unable to register cfattach (%d)\n",
			bthub_cd.cd_name, err);

		config_cfdriver_detach(&bthub_cd);
		return;
	}

	hook = (struct bthub_softc *)config_attach_pseudo(&bthub_cfdata);
	if (hook == NULL) {
		aprint_error("%s: unable to attach bthub hook(%d)\n",
			bthub_cd.cd_name, err);

		config_cfattach_detach(bthub_cd.cd_name, &bthub_ca);
		config_cfdriver_detach(&bthub_cd);
		return;
	}
}

/*****************************************************************************
 *
 *	bthub autoconf(9) routines
 *
 *	This is the real device hanging on our hook. Only the
 *	attach function will ever be called, I guess.
 */

static int
bthub_match(struct device *self, struct cfdata *cfdata, void *arg)
{

	return 1;
}

static void
bthub_attach(struct device *parent, struct device *self, void *aux)
{
	struct bthub_softc *sc = (struct bthub_softc *)self;

	LIST_INIT(&sc->sc_list);
}

static int
bthub_detach(struct device *self, int flags)
{
	struct bthub_softc *sc = (struct bthub_softc *)self;
	struct btdev *btdev;

	while ((btdev = LIST_FIRST(&sc->sc_list)) != NULL) {
		LIST_REMOVE(btdev, sc_next);
		config_detach(&btdev->sc_dev, flags);
	}
	return 0;
}

/*****************************************************************************
 *
 *	bthub control device ioctl(2)
 *
 *	This is the method we use to control bluetooth devices,
 *	called from userland.
 */

int
bthubioctl(dev_t devno, unsigned long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct btdev_attach_args *bda;
	struct btdev *btdev;
	bdaddr_t *bdaddr;

	if (hook == NULL)
		return ENXIO;

	switch (cmd) {
	case BTDEV_ATTACH:	/* attach BTDEV */
		bda = (struct btdev_attach_args *)data;

		/*
		 * validate our configuration
		 */
		if (bdaddr_any(&bda->bd_raddr)
		    || bda->bd_type == 0)
			return EINVAL;

		LIST_FOREACH(btdev, &hook->sc_list, sc_next) {
			if (bdaddr_same(&btdev->sc_addr, &bda->bd_raddr))
				return EADDRINUSE;
		}

		btdev = (struct btdev *)config_found((struct device *)hook,
						bda, bthub_print);
		if (btdev == NULL)
			return ENXIO;

		bdaddr_copy(&btdev->sc_addr, &bda->bd_raddr);
		LIST_INSERT_HEAD(&hook->sc_list, btdev, sc_next);
		return 0;

	case BTDEV_DETACH:	/* detach BTDEV */
		bdaddr = (bdaddr_t *)data;

		LIST_FOREACH(btdev, &hook->sc_list, sc_next) {
			if (bdaddr_same(bdaddr, &btdev->sc_addr) == 0)
				continue;

			LIST_REMOVE(btdev, sc_next);
			config_detach((struct device *)btdev, DETACH_FORCE);
			return 0;
		}
		return ENXIO;

	default:
		break;
	}

	return EPASSTHROUGH;
}

static int
bthub_print(void *aux, const char *pnp)
{
	struct btdev_attach_args *bda = aux;

	if (pnp != NULL)
		aprint_normal("%s: ", pnp);

	aprint_verbose(" bdaddr %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
		bda->bd_raddr.b[5], bda->bd_raddr.b[4],
		bda->bd_raddr.b[3], bda->bd_raddr.b[2],
		bda->bd_raddr.b[1], bda->bd_raddr.b[0]);

	return UNCONF;
}
