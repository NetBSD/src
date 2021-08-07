/*	$NetBSD: onewire.c,v 1.22 2021/08/07 16:19:14 thorpej Exp $	*/
/*	$OpenBSD: onewire.c,v 1.1 2006/03/04 16:27:03 grange Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Copyright (c) 2006 Alexander Yurchenko <grange@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: onewire.c,v 1.22 2021/08/07 16:19:14 thorpej Exp $");

/*
 * 1-Wire bus driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/module.h>

#ifdef _KERNEL_OPT
#include "opt_onewire.h"
#endif

#include <dev/onewire/onewirereg.h>
#include <dev/onewire/onewirevar.h>

#ifdef ONEWIRE_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

int	onewire_maxdevs = 8;
int	onewire_scantime = 10;	/* was 3 seconds - too often */

struct onewire_softc {
	device_t			sc_dev;
	struct onewire_bus *		sc_bus;
	kmutex_t			sc_lock;
	kcondvar_t			sc_scancv;
	struct lwp *			sc_thread;
	TAILQ_HEAD(, onewire_device)	sc_devs;
	int				sc_dying;
};

struct onewire_device {
	TAILQ_ENTRY(onewire_device)	d_list;
	device_t			d_dev;
	u_int64_t			d_rom;
	bool				d_present;
};

static int	onewire_match(device_t, cfdata_t, void *);
static void	onewire_attach(device_t, device_t, void *);
static int	onewire_detach(device_t, int);
static int	onewire_activate(device_t, enum devact);
int		onewire_print(void *, const char *);

static void	onewire_thread(void *);
static void	onewire_scan(struct onewire_softc *);

CFATTACH_DECL_NEW(onewire, sizeof(struct onewire_softc),
	onewire_match, onewire_attach, onewire_detach, onewire_activate);

extern struct cfdriver onewire_cd;

static int
onewire_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
onewire_attach(device_t parent, device_t self, void *aux)
{
	struct onewire_softc *sc = device_private(self);
	struct onewirebus_attach_args *oba = aux;

	sc->sc_dev = self;
	sc->sc_bus = oba->oba_bus;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_scancv, "owscan");
	TAILQ_INIT(&sc->sc_devs);

	aprint_normal("\n");

	if (kthread_create(PRI_NONE, KTHREAD_MUSTJOIN | KTHREAD_MPSAFE, NULL,
	    onewire_thread, sc, &sc->sc_thread, "%s", device_xname(self)) != 0) {
		aprint_error_dev(self, "can't create kernel thread\n");
		/* Normally the kthread destroys these. */
		mutex_destroy(&sc->sc_lock);
		cv_destroy(&sc->sc_scancv);
	}
}

static int
onewire_detach(device_t self, int flags)
{
	struct onewire_softc *sc = device_private(self);
	int rv;

	if (sc->sc_thread != NULL) {
		mutex_enter(&sc->sc_lock);
		sc->sc_dying = 1;
		cv_broadcast(&sc->sc_scancv);
		mutex_exit(&sc->sc_lock);
		/* Must no longer touch sc_lock nor sc_scancv. */
		kthread_join(sc->sc_thread);
	}

	//rv = config_detach_children(self, flags);
	rv = 0;  /* XXX riz */

	return rv;
}

static int
onewire_activate(device_t self, enum devact act)
{
	struct onewire_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

int
onewire_print(void *aux, const char *pnp)
{
	struct onewire_attach_args *oa = aux;
	const char *famname;

	if (pnp == NULL)
		aprint_normal(" ");

	famname = onewire_famname(ONEWIRE_ROM_FAMILY_TYPE(oa->oa_rom));
	if (famname == NULL)
		aprint_normal("family 0x%02x",
		    (uint)ONEWIRE_ROM_FAMILY_TYPE(oa->oa_rom));
	else
		aprint_normal("\"%s\"", famname);
	aprint_normal(" sn %012" PRIx64, ONEWIRE_ROM_SN(oa->oa_rom));

	if (pnp != NULL)
		aprint_normal(" at %s", pnp);

	return UNCONF;
}

int
onewirebus_print(void *aux, const char *pnp)
{
	if (pnp != NULL)
		aprint_normal("onewire at %s", pnp);

	return UNCONF;
}

void
onewire_lock(void *arg)
{
	struct onewire_softc *sc = arg;

	mutex_enter(&sc->sc_lock);
}

void
onewire_unlock(void *arg)
{
	struct onewire_softc *sc = arg;

	mutex_exit(&sc->sc_lock);
}

int
onewire_reset(void *arg)
{
	struct onewire_softc *sc = arg;
	struct onewire_bus *bus = sc->sc_bus;

	KASSERT(mutex_owned(&sc->sc_lock));

	return bus->bus_reset(bus->bus_cookie);
}

int
onewire_read_bit(void *arg)
{
	struct onewire_softc *sc = arg;
	struct onewire_bus *bus = sc->sc_bus;

	KASSERT(mutex_owned(&sc->sc_lock));

	return bus->bus_read_bit(bus->bus_cookie);
}

void
onewire_write_bit(void *arg, int value)
{
	struct onewire_softc *sc = arg;
	struct onewire_bus *bus = sc->sc_bus;

	KASSERT(mutex_owned(&sc->sc_lock));

	bus->bus_write_bit(bus->bus_cookie, value);
}

int
onewire_read_byte(void *arg)
{
	struct onewire_softc *sc = arg;
	struct onewire_bus *bus = sc->sc_bus;
	uint8_t value = 0;
	int i;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (bus->bus_read_byte != NULL)
		return bus->bus_read_byte(bus->bus_cookie);

	for (i = 0; i < 8; i++)
		value |= (bus->bus_read_bit(bus->bus_cookie) << i);

	return value;
}

void
onewire_write_byte(void *arg, int value)
{
	struct onewire_softc *sc = arg;
	struct onewire_bus *bus = sc->sc_bus;
	int i;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (bus->bus_write_byte != NULL)
		return bus->bus_write_byte(bus->bus_cookie, value);

	for (i = 0; i < 8; i++)
		bus->bus_write_bit(bus->bus_cookie, (value >> i) & 0x1);
}

int
onewire_triplet(void *arg, int dir)
{
	struct onewire_softc *sc = arg;
	struct onewire_bus *bus = sc->sc_bus;
	int rv;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (bus->bus_triplet != NULL)
		return bus->bus_triplet(bus->bus_cookie, dir);

	rv = bus->bus_read_bit(bus->bus_cookie);
	rv <<= 1;
	rv |= bus->bus_read_bit(bus->bus_cookie);

	switch (rv) {
	case 0x0:
		bus->bus_write_bit(bus->bus_cookie, dir);
		break;
	case 0x1:
		bus->bus_write_bit(bus->bus_cookie, 0);
		break;
	default:
		bus->bus_write_bit(bus->bus_cookie, 1);
	}

	return rv;
}

void
onewire_read_block(void *arg, void *buf, int len)
{
	struct onewire_softc *sc = arg;
	uint8_t *p = buf;

	KASSERT(mutex_owned(&sc->sc_lock));

	while (len--)
		*p++ = onewire_read_byte(sc);
}

void
onewire_write_block(void *arg, const void *buf, int len)
{
	struct onewire_softc *sc = arg;
	const uint8_t *p = buf;

	KASSERT(mutex_owned(&sc->sc_lock));

	while (len--)
		onewire_write_byte(sc, *p++);
}

void
onewire_matchrom(void *arg, u_int64_t rom)
{
	struct onewire_softc *sc = arg;
	int i;

	KASSERT(mutex_owned(&sc->sc_lock));

	onewire_write_byte(sc, ONEWIRE_CMD_MATCH_ROM);
	for (i = 0; i < 8; i++)
		onewire_write_byte(sc, (rom >> (i * 8)) & 0xff);
}

static void
onewire_thread(void *arg)
{
	struct onewire_softc *sc = arg;
	int unit, dly;

	/*
	 * There can be many onewire busses, potentially funneled through
	 * few GPIO controllers.  To avoid a thundering herd of kthreads and
	 * resulting contention for the GPIO controller, spread the probes
	 * out across an 8 second window.  The kthreads could converge later
	 * due to timing effects.
	 */
	unit = device_unit(sc->sc_dev);
	dly = (unit & 0x07) * hz + ((unit >> 3) * hz >> 3) + 1;
	(void)kpause("owdly", false, dly, NULL);

	mutex_enter(&sc->sc_lock);
	while (!sc->sc_dying) {
		onewire_scan(sc);
		(void)cv_timedwait(&sc->sc_scancv, &sc->sc_lock,
		    onewire_scantime * hz);
	}
	mutex_exit(&sc->sc_lock);

	/* Caller has set sc_dying and will no longer touch these. */
	cv_destroy(&sc->sc_scancv);
	mutex_destroy(&sc->sc_lock);
	kthread_exit(0);
}

static void
onewire_scan(struct onewire_softc *sc)
{
	struct onewire_device *d, *next, *nd;
	struct onewire_attach_args oa;
	int search = 1, count = 0, present;
	int dir, rv;
	uint64_t mask, rom = 0, lastrom;
	uint8_t data[8];
	int i, i0 = -1, lastd = -1;

	TAILQ_FOREACH(d, &sc->sc_devs, d_list) {
		d->d_present = false;
		KASSERT(d->d_dev != NULL);
	}

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(curlwp == sc->sc_thread);

	while (search && count++ < onewire_maxdevs) {
		/*
		 * Reset the bus, allowing for one retry if reset fails.  If
		 * there's no presence pulse don't search for any devices.
		 */
		if (onewire_reset(sc) != 0) {
			DPRINTF(("%s: scan: no presence pulse\n",
			    device_xname(sc->sc_dev)));
			if (onewire_reset(sc) != 0) {
				DPRINTF(("%s: scan: retry failed\n",
				    device_xname(sc->sc_dev)));
				break;
			}
		}

		/*
		 * Start new search. Go through the previous path to
		 * the point we made a decision last time and make an
		 * opposite decision. If we didn't make any decision
		 * stop searching.
		 */
		search = 0;
		lastrom = rom;
		rom = 0;
		onewire_write_byte(sc, ONEWIRE_CMD_SEARCH_ROM);
		for (i = 0,i0 = -1; i < 64; i++) {
			dir = (lastrom >> i) & 0x1;
			if (i == lastd)
				dir = 1;
			else if (i > lastd)
				dir = 0;
			rv = onewire_triplet(sc, dir);
			switch (rv) {
			case 0x0:
				if (i != lastd) {
					if (dir == 0)
						i0 = i;
					search = 1;
				}
				mask = dir;
				break;
			case 0x1:
				mask = 0;
				break;
			case 0x2:
				mask = 1;
				break;
			default:
				DPRINTF(("%s: scan: triplet error 0x%x, "
				    "step %d\n",
				    device_xname(sc->sc_dev), rv, i));
				return;
			}
			rom |= (mask << i);
		}
		lastd = i0;

		/*
		 * Yield processor, but continue to hold the lock
		 * so that scan is not interrupted.
		 */
		(void)kpause("owscan", false, 1, NULL);

		if (rom == 0)
			continue;

		/*
		 * The last byte of the ROM code contains a CRC calculated
		 * from the first 7 bytes. Re-calculate it to make sure
		 * we found a valid device.
		 */
		for (i = 0; i < 8; i++)
			data[i] = (rom >> (i * 8)) & 0xff;
		if (onewire_crc(data, 7) != data[7])
			continue;

		/*
		 * Go through the list of attached devices to see if we
		 * found a new one.
		 */
		present = 0;
	 	TAILQ_FOREACH(d, &sc->sc_devs, d_list) {
			if (d->d_rom == rom) {
				d->d_present = true;
				present = 1;
				break;
			}
		}
		if (!present) {
			nd = kmem_alloc(sizeof(*nd), KM_SLEEP);
			nd->d_dev = NULL;
			nd->d_rom = rom;
			nd->d_present = true;
			TAILQ_INSERT_TAIL(&sc->sc_devs, nd, d_list);
		}
	}

	/*
	 * Detach disappeared devices, and attach new devices.  Drop the
	 * lock when doing this in order to prevent lock order reversal
	 * against sysmon.  This is safe because nothing other than this
	 * kthread modifies our device list.
	 */
	for (d = TAILQ_FIRST(&sc->sc_devs); d != NULL; d = next) {
		next = TAILQ_NEXT(d, d_list);
		if (!d->d_present) {
			mutex_exit(&sc->sc_lock);

			KERNEL_LOCK(1, NULL); /* XXXSMP */
			config_detach(d->d_dev, DETACH_FORCE);
			d->d_dev = NULL;
			KERNEL_UNLOCK_ONE(NULL); /* XXXSMP */

			mutex_enter(&sc->sc_lock);
		} else if (d->d_dev == NULL) {
			memset(&oa, 0, sizeof(oa));
			oa.oa_onewire = sc;
			oa.oa_rom = d->d_rom;
			mutex_exit(&sc->sc_lock);

			KERNEL_LOCK(1, NULL); /* XXXSMP */
			d->d_dev = config_found(sc->sc_dev, &oa, onewire_print,
			    CFARGS_NONE);
			KERNEL_UNLOCK_ONE(NULL); /* XXXSMP */

			mutex_enter(&sc->sc_lock);
		}
		if (d->d_dev == NULL) {
			TAILQ_REMOVE(&sc->sc_devs, d, d_list);
			kmem_free(d, sizeof(*d));
		}
	}
}

MODULE(MODULE_CLASS_DRIVER, onewire, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
onewire_modcmd(modcmd_t cmd, void *opaque)
{
	int error;

	error = 0;
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_onewire,
		    cfattach_ioconf_onewire, cfdata_ioconf_onewire);
		if (error)
			aprint_error("%s: unable to init component\n",
			    onewire_cd.cd_name);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		config_fini_component(cfdriver_ioconf_onewire,
		    cfattach_ioconf_onewire, cfdata_ioconf_onewire);
#endif
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
