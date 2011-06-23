/* $NetBSD: gpio.c,v 1.33.6.1 2011/06/23 14:19:58 cherry Exp $ */
/*	$OpenBSD: gpio.c,v 1.6 2006/01/14 12:33:49 grange Exp $	*/

/*
 * Copyright (c) 2008, 2009, 2010 Marc Balmer <marc@msys.ch>
 * Copyright (c) 2004, 2006 Alexander Yurchenko <grange@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: gpio.c,v 1.33.6.1 2011/06/23 14:19:58 cherry Exp $");

/*
 * General Purpose Input/Output framework.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <sys/vnode.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/kauth.h>

#include <dev/gpio/gpiovar.h>

#include "locators.h"

#ifdef GPIO_DEBUG
#define DPRINTFN(n, x)	do { if (gpiodebug > (n)) printf x; } while (0)
int gpiodebug = 0;
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x)	DPRINTFN(0, x)

struct gpio_softc {
	device_t		 sc_dev;

	gpio_chipset_tag_t	 sc_gc;		/* GPIO controller */
	gpio_pin_t		*sc_pins;	/* pins array */
	int			 sc_npins;	/* number of pins */

	int			 sc_opened;
	LIST_HEAD(, gpio_dev)	 sc_devs;	/* devices */
	LIST_HEAD(, gpio_name)	 sc_names;	/* named pins */
};

int	gpio_match(device_t, cfdata_t, void *);
int	gpio_submatch(device_t, cfdata_t, const int *, void *);
void	gpio_attach(device_t, device_t, void *);
int	gpio_rescan(device_t, const char *, const int *);
void	gpio_childdetached(device_t, device_t);
bool	gpio_resume(device_t, const pmf_qual_t *);
int	gpio_detach(device_t, int);
int	gpio_search(device_t, cfdata_t, const int *, void *);
int	gpio_print(void *, const char *);
int	gpio_pinbyname(struct gpio_softc *, char *);

/* Old API */
int	gpio_ioctl_oapi(struct gpio_softc *, u_long, void *, int, kauth_cred_t);

CFATTACH_DECL3_NEW(gpio, sizeof(struct gpio_softc),
    gpio_match, gpio_attach, gpio_detach, NULL, gpio_rescan,
    gpio_childdetached, DVF_DETACH_SHUTDOWN);

dev_type_open(gpioopen);
dev_type_close(gpioclose);
dev_type_ioctl(gpioioctl);

const struct cdevsw gpio_cdevsw = {
	gpioopen, gpioclose, noread, nowrite, gpioioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER,
};

extern struct cfdriver gpio_cd;

int
gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

int
gpio_submatch(device_t parent, cfdata_t cf, const int *ip, void *aux)
{
	struct gpio_attach_args *ga = aux;

	if (ga->ga_offset == -1)
		return 0;

	return strcmp(ga->ga_dvname, cf->cf_name) == 0;
}

bool
gpio_resume(device_t self, const pmf_qual_t *qual)
{
	struct gpio_softc *sc = device_private(self);
	int pin;

	for (pin = 0; pin < sc->sc_npins; pin++) {
		gpiobus_pin_ctl(sc->sc_gc, pin, sc->sc_pins[pin].pin_flags);
		gpiobus_pin_write(sc->sc_gc, pin, sc->sc_pins[pin].pin_state);
	}
	return true;
}

void
gpio_childdetached(device_t self, device_t child)
{
	/* gpio(4) keeps no references to its children, so do nothing. */
}

int
gpio_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct gpio_softc *sc = device_private(self);

	config_search_loc(gpio_search, self, ifattr, locators, sc);

	return 0;
}

void
gpio_attach(device_t parent, device_t self, void *aux)
{
	struct gpio_softc *sc = device_private(self);
	struct gpiobus_attach_args *gba = aux;

	sc->sc_dev = self;
	sc->sc_gc = gba->gba_gc;
	sc->sc_pins = gba->gba_pins;
	sc->sc_npins = gba->gba_npins;

	printf(": %d pins\n", sc->sc_npins);

	if (!pmf_device_register(self, NULL, gpio_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/*
	 * Attach all devices that can be connected to the GPIO pins
	 * described in the kernel configuration file.
	 */
	gpio_rescan(self, "gpio", NULL);
}

int
gpio_detach(device_t self, int flags)
{
	int rc;

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

#if 0
	int maj, mn;

	/* Locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == gpioopen)
			break;

	/* Nuke the vnodes for any open instances (calls close) */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);
#endif
	return 0;
}

int
gpio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct gpio_attach_args ga;

	ga.ga_gpio = aux;
	ga.ga_offset = cf->cf_loc[GPIOCF_OFFSET];
	ga.ga_mask = cf->cf_loc[GPIOCF_MASK];

	if (config_match(parent, cf, &ga) > 0)
		config_attach(parent, cf, &ga, gpio_print);

	return 0;
}

int
gpio_print(void *aux, const char *pnp)
{
	struct gpio_attach_args *ga = aux;
	int i;

	printf(" pins");
	for (i = 0; i < 32; i++)
		if (ga->ga_mask & (1 << i))
			printf(" %d", ga->ga_offset + i);

	return UNCONF;
}

int
gpiobus_print(void *aux, const char *pnp)
{
#if 0
	struct gpiobus_attach_args *gba = aux;
#endif
	if (pnp != NULL)
		aprint_normal("gpiobus at %s", pnp);

	return UNCONF;
}

/* return 1 if all pins can be mapped, 0 if not */
int
gpio_pin_can_map(void *gpio, int offset, u_int32_t mask)
{
	struct gpio_softc *sc = gpio;
	int npins, pin, i;

	npins = gpio_npins(mask);
	if (npins > sc->sc_npins)
		return 0;

	for (npins = 0, i = 0; i < 32; i++)
		if (mask & (1 << i)) {
			pin = offset + i;
			if (pin < 0 || pin >= sc->sc_npins)
				return 0;
			if (sc->sc_pins[pin].pin_mapped)
				return 0;
		}

	return 1;
}

int
gpio_pin_map(void *gpio, int offset, u_int32_t mask, struct gpio_pinmap *map)
{
	struct gpio_softc *sc = gpio;
	int npins, pin, i;

	npins = gpio_npins(mask);
	if (npins > sc->sc_npins)
		return 1;

	for (npins = 0, i = 0; i < 32; i++)
		if (mask & (1 << i)) {
			pin = offset + i;
			if (pin < 0 || pin >= sc->sc_npins)
				return 1;
			if (sc->sc_pins[pin].pin_mapped)
				return 1;
			sc->sc_pins[pin].pin_mapped = 1;
			map->pm_map[npins++] = pin;
		}
	map->pm_size = npins;

	return 0;
}

void
gpio_pin_unmap(void *gpio, struct gpio_pinmap *map)
{
	struct gpio_softc *sc = gpio;
	int pin, i;

	for (i = 0; i < map->pm_size; i++) {
		pin = map->pm_map[i];
		sc->sc_pins[pin].pin_mapped = 0;
	}
}

int
gpio_pin_read(void *gpio, struct gpio_pinmap *map, int pin)
{
	struct gpio_softc *sc = gpio;

	return gpiobus_pin_read(sc->sc_gc, map->pm_map[pin]);
}

void
gpio_pin_write(void *gpio, struct gpio_pinmap *map, int pin, int value)
{
	struct gpio_softc *sc = gpio;

	gpiobus_pin_write(sc->sc_gc, map->pm_map[pin], value);
	sc->sc_pins[map->pm_map[pin]].pin_state = value;
}

void
gpio_pin_ctl(void *gpio, struct gpio_pinmap *map, int pin, int flags)
{
	struct gpio_softc *sc = gpio;

	return gpiobus_pin_ctl(sc->sc_gc, map->pm_map[pin], flags);
}

int
gpio_pin_caps(void *gpio, struct gpio_pinmap *map, int pin)
{
	struct gpio_softc *sc = gpio;

	return sc->sc_pins[map->pm_map[pin]].pin_caps;
}

int
gpio_npins(u_int32_t mask)
{
	int npins, i;

	for (npins = 0, i = 0; i < 32; i++)
		if (mask & (1 << i))
			npins++;

	return npins;
}

int
gpioopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct gpio_softc *sc;
	int ret;

	sc = device_lookup_private(&gpio_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	DPRINTF(("%s: opening\n", device_xname(sc->sc_dev)));
	if (sc->sc_opened) {
		DPRINTF(("%s: already opened\n", device_xname(sc->sc_dev)));
		return EBUSY;
	}

	if ((ret = gpiobus_open(sc->sc_gc, sc->sc_dev))) {
		DPRINTF(("%s: gpiobus_open returned %d\n",
		    device_xname(sc->sc_dev),
		    ret));
		return ret;
	}

	sc->sc_opened = 1;

	return 0;
}

int
gpioclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct gpio_softc *sc;

	sc = device_lookup_private(&gpio_cd, minor(dev));
	DPRINTF(("%s: closing\n", device_xname(sc->sc_dev)));
	(void)gpiobus_close(sc->sc_gc, sc->sc_dev);
	sc->sc_opened = 0;

	return 0;
}

int
gpio_pinbyname(struct gpio_softc *sc, char *gp_name)
{
        struct gpio_name *nm;

        LIST_FOREACH(nm, &sc->sc_names, gp_next)
                if (!strcmp(nm->gp_name, gp_name))
                        return nm->gp_pin;
        return -1;
}

int
gpioioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct gpio_softc *sc;
	gpio_chipset_tag_t gc;
	struct gpio_info *info;
	struct gpio_attach *attach;
	struct gpio_attach_args ga;
	struct gpio_dev *gdev;
	struct gpio_req *req;
	struct gpio_name *nm;
	struct gpio_set *set;
	device_t dv;
	cfdata_t cf;
	kauth_cred_t cred;
	int locs[GPIOCF_NLOCS];
	int pin, value, flags, npins;

	sc = device_lookup_private(&gpio_cd, minor(dev));
	gc = sc->sc_gc;

	if (cmd != GPIOINFO && !device_is_active(sc->sc_dev)) {
		DPRINTF(("%s: device is not active\n",
		    device_xname(sc->sc_dev)));
		return EBUSY;
	}
	
	cred = kauth_cred_get();

	switch (cmd) {
	case GPIOINFO:
		info = (struct gpio_info *)data;
		if (!kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			info->gpio_npins = sc->sc_npins;
		else {
			for (pin = npins = 0; pin < sc->sc_npins; pin++)
				if (sc->sc_pins[pin].pin_flags & GPIO_PIN_SET)
					++npins;
			info->gpio_npins = npins;
		}
		break;
	case GPIOREAD:
		req = (struct gpio_req *)data;

		if (req->gp_name[0] != '\0') {
			pin = gpio_pinbyname(sc, req->gp_name);
			if (pin == -1)
				return EINVAL;
		} else
			pin = req->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;

		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET) &&
		    kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		/* return read value */
		req->gp_value = gpiobus_pin_read(gc, pin);
		break;
	case GPIOWRITE:
		if ((flag & FWRITE) == 0)
			return EBADF;

		req = (struct gpio_req *)data;

		if (req->gp_name[0] != '\0') {
			pin = gpio_pinbyname(sc, req->gp_name);
			if (pin == -1)
				return EINVAL;
		} else
			pin = req->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;

		if (sc->sc_pins[pin].pin_mapped)
			return EBUSY;

		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET) &&
		    kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		value = req->gp_value;
		if (value != GPIO_PIN_LOW && value != GPIO_PIN_HIGH)
			return EINVAL;

		gpiobus_pin_write(gc, pin, value);
		/* return old value */
		req->gp_value = sc->sc_pins[pin].pin_state;
		/* update current value */
		sc->sc_pins[pin].pin_state = value;
		break;
	case GPIOTOGGLE:
		if ((flag & FWRITE) == 0)
			return EBADF;

		req = (struct gpio_req *)data;

		if (req->gp_name[0] != '\0') {
			pin = gpio_pinbyname(sc, req->gp_name);
			if (pin == -1)
				return EINVAL;
		} else
			pin = req->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;

		if (sc->sc_pins[pin].pin_mapped)
			return EBUSY;

		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET) &&
		    kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		value = (sc->sc_pins[pin].pin_state == GPIO_PIN_LOW ?
		    GPIO_PIN_HIGH : GPIO_PIN_LOW);
		gpiobus_pin_write(gc, pin, value);
		/* return old value */
		req->gp_value = sc->sc_pins[pin].pin_state;
		/* update current value */
		sc->sc_pins[pin].pin_state = value;
		break;
	case GPIOATTACH:
		if (kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		attach = (struct gpio_attach *)data;

		/* do not try to attach if the pins are already mapped */
		if (!gpio_pin_can_map(sc, attach->ga_offset, attach->ga_mask))
			return EBUSY;

		ga.ga_gpio = sc;
		ga.ga_dvname = attach->ga_dvname;
		ga.ga_offset = attach->ga_offset;
		ga.ga_mask = attach->ga_mask;
		DPRINTF(("%s: attach %s with offset %d and mask 0x%02x\n",
		    device_xname(sc->sc_dev), ga.ga_dvname, ga.ga_offset,
		    ga.ga_mask));

		locs[GPIOCF_OFFSET] = ga.ga_offset;
		locs[GPIOCF_MASK] = ga.ga_mask;

		cf = config_search_loc(NULL, sc->sc_dev, "gpio", locs, &ga);
		if (cf != NULL) {
			dv = config_attach_loc(sc->sc_dev, cf, locs, &ga,
			    gpiobus_print);
			if (dv != NULL) {
				gdev = kmem_alloc(sizeof(struct gpio_dev),
				    KM_SLEEP);
				gdev->sc_dev = dv;
				LIST_INSERT_HEAD(&sc->sc_devs, gdev, sc_next);
			} else
				return EINVAL;
		} else
			return EINVAL;
		break;
	case GPIODETACH:
		if (kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;
                
		attach = (struct gpio_attach *)data;
		LIST_FOREACH(gdev, &sc->sc_devs, sc_next) {
			if (strcmp(device_xname(gdev->sc_dev),
			    attach->ga_dvname) == 0) {
				if (config_detach(gdev->sc_dev, 0) == 0) {
					LIST_REMOVE(gdev, sc_next);
					kmem_free(gdev,
					    sizeof(struct gpio_dev));
					return 0;
				}
				break;
			}
		}
		return EINVAL;
		break;
	case GPIOSET:
		if (kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		set = (struct gpio_set *)data;

		if (set->gp_name[0] != '\0') {
			pin = gpio_pinbyname(sc, set->gp_name);
			if (pin == -1)
				return EINVAL;
		} else
			pin = set->gp_pin;
		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;
		flags = set->gp_flags;

		/* check that the controller supports all requested flags */
		if ((flags & sc->sc_pins[pin].pin_caps) != flags)
			return ENODEV;
		flags = set->gp_flags | GPIO_PIN_SET;

		set->gp_caps = sc->sc_pins[pin].pin_caps;
		/* return old value */
		set->gp_flags = sc->sc_pins[pin].pin_flags;
		if (flags > 0) {
			gpiobus_pin_ctl(gc, pin, flags);
			/* update current value */
			sc->sc_pins[pin].pin_flags = flags;
		}

		/* rename pin or new pin? */
		if (set->gp_name2[0] != '\0') {
			struct gpio_name *gnm;

			gnm = NULL;
			LIST_FOREACH(nm, &sc->sc_names, gp_next) {
				if (!strcmp(nm->gp_name, set->gp_name2) &&
				    nm->gp_pin != pin)
					return EINVAL;	/* duplicate name */
				if (nm->gp_pin == pin)
					gnm = nm;
			}
			if (gnm != NULL)
				strlcpy(gnm->gp_name, set->gp_name2,
				    sizeof(gnm->gp_name));
			else  {
				nm = kmem_alloc(sizeof(struct gpio_name),
				    KM_SLEEP);
				strlcpy(nm->gp_name, set->gp_name2,
				    sizeof(nm->gp_name));
				nm->gp_pin = set->gp_pin;
				LIST_INSERT_HEAD(&sc->sc_names, nm, gp_next);
			}
		}
		break;
	case GPIOUNSET:
		if (kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		set = (struct gpio_set *)data;
		if (set->gp_name[0] != '\0') {
			pin = gpio_pinbyname(sc, set->gp_name);
			if (pin == -1)
				return EINVAL;
		} else
			pin = set->gp_pin;
		
		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;
		if (sc->sc_pins[pin].pin_mapped)
			return EBUSY;
		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET))
			return EINVAL;

		LIST_FOREACH(nm, &sc->sc_names, gp_next) {
			if (nm->gp_pin == pin) {
				LIST_REMOVE(nm, gp_next);
				kmem_free(nm, sizeof(struct gpio_name));
				break;
			}
		}
		sc->sc_pins[pin].pin_flags &= ~GPIO_PIN_SET;
		break;
	default:
		/* Try the old API */
		DPRINTF(("%s: trying the old API\n", device_xname(sc->sc_dev)));
		return gpio_ioctl_oapi(sc, cmd, data, flag, cred);
	}
	return 0;
}

int
gpio_ioctl_oapi(struct gpio_softc *sc, u_long cmd, void *data, int flag,
    kauth_cred_t cred)
{
	gpio_chipset_tag_t gc;
	struct gpio_pin_op *op;
	struct gpio_pin_ctl *ctl;
	int pin, value, flags;

	gc = sc->sc_gc;

	switch (cmd) {
	case GPIOPINREAD:
		op = (struct gpio_pin_op *)data;

		pin = op->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;

		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET) &&
		    kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		/* return read value */
		op->gp_value = gpiobus_pin_read(gc, pin);
		break;
	case GPIOPINWRITE:
		if ((flag & FWRITE) == 0)
			return EBADF;

		op = (struct gpio_pin_op *)data;

		pin = op->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;

		if (sc->sc_pins[pin].pin_mapped)
			return EBUSY;

		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET) &&
		    kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		value = op->gp_value;
		if (value != GPIO_PIN_LOW && value != GPIO_PIN_HIGH)
			return EINVAL;

		gpiobus_pin_write(gc, pin, value);
		/* return old value */
		op->gp_value = sc->sc_pins[pin].pin_state;
		/* update current value */
		sc->sc_pins[pin].pin_state = value;
		break;
	case GPIOPINTOGGLE:
		if ((flag & FWRITE) == 0)
			return EBADF;

		op = (struct gpio_pin_op *)data;

		pin = op->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;

		if (sc->sc_pins[pin].pin_mapped)
			return EBUSY;

		if (!(sc->sc_pins[pin].pin_flags & GPIO_PIN_SET) &&
		    kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		value = (sc->sc_pins[pin].pin_state == GPIO_PIN_LOW ?
		    GPIO_PIN_HIGH : GPIO_PIN_LOW);
		gpiobus_pin_write(gc, pin, value);
		/* return old value */
		op->gp_value = sc->sc_pins[pin].pin_state;
		/* update current value */
		sc->sc_pins[pin].pin_state = value;
		break;
	case GPIOPINCTL:
		ctl = (struct gpio_pin_ctl *) data;

		if (kauth_authorize_device(cred, KAUTH_DEVICE_GPIO_PINSET,
		    NULL, NULL, NULL, NULL))
			return EPERM;

		pin = ctl->gp_pin;

		if (pin < 0 || pin >= sc->sc_npins)
			return EINVAL;
		if (sc->sc_pins[pin].pin_mapped)
			return EBUSY;
		flags = ctl->gp_flags;

		/* check that the controller supports all requested flags */
		if ((flags & sc->sc_pins[pin].pin_caps) != flags)
			return ENODEV;

		ctl->gp_caps = sc->sc_pins[pin].pin_caps;
		/* return old value */
		ctl->gp_flags = sc->sc_pins[pin].pin_flags;
		if (flags > 0) {
			gpiobus_pin_ctl(gc, pin, flags);
			/* update current value */
			sc->sc_pins[pin].pin_flags = flags;
		}
		break;
	default:
		return ENOTTY;
	}
	return 0;
}
