/*	$NetBSD: i2c.c,v 1.106 2026/01/15 06:25:45 skrll Exp $	*/

/*
 * Copyright (c) 2021, 2022, 2025 The NetBSD Foundation, Inc.
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
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef _KERNEL_OPT
#include "opt_i2c.h"

#include "opt_fdt.h"
#ifdef FDT
#define	I2C_USE_FDT
#endif /* FDT */

#if defined(__aarch64__) || defined(__amd64__)
#include "acpica.h"
#if NACPICA > 0
#define	I2C_USE_ACPI
#endif /* NACPICA > 0 */
#endif /* __aarch64__ || __amd64__ */

#endif /* _KERNEL_OPT */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2c.c,v 1.106 2026/01/15 06:25:45 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/event.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/module.h>
#include <sys/once.h>
#include <sys/mutex.h>

#ifdef I2C_USE_ACPI
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_i2c.h>
#endif /* I2C_USE_ACPI */

#ifdef I2C_USE_FDT
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_i2c.h>
#endif /* I2C_USE_FDT */

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_calls.h>

#include "ioconf.h"
#include "locators.h"

#ifndef I2C_MAX_ADDR
#define I2C_MAX_ADDR	0x3ff	/* 10-bit address, max */
#endif

/* Everything projected by iic_softc::sc_device_state_lock */
struct i2c_device {
	LIST_ENTRY(i2c_device)	d_link;
	device_t		d_dev;
	i2c_addr_t		d_addr;
	int			d_flags;
#define	ID_F_DIRECT		__BIT(0)
#define	ID_F_BUSY		__BIT(1)
#define	ID_F_WAIT_BUSY		__BIT(2)
};

struct iic_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	kmutex_t sc_device_state_lock;
	kcondvar_t sc_device_state_cond;
	LIST_HEAD(, i2c_device) sc_devices;
};

static dev_type_open(iic_open);
static dev_type_close(iic_close);
static dev_type_ioctl(iic_ioctl);

int iic_init(void);

kmutex_t iic_mtx;
int iic_refcnt;

ONCE_DECL(iic_once);

const struct cdevsw iic_cdevsw = {
	.d_open = iic_open,
	.d_close = iic_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = iic_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static void
iic_device_wait(struct iic_softc *sc, struct i2c_device *d)
{
	KASSERT(mutex_owned(&sc->sc_device_state_lock));

	while (d->d_flags & ID_F_BUSY) {
		d->d_flags |= ID_F_WAIT_BUSY;
		cv_wait(&sc->sc_device_state_cond, &sc->sc_device_state_lock);
	}
}

static struct i2c_device *
iic_device_lookup_addr(struct iic_softc *sc, i2c_addr_t addr, bool wait_busy)
{
	struct i2c_device *d;

	KASSERT(mutex_owned(&sc->sc_device_state_lock));

	LIST_FOREACH(d, &sc->sc_devices, d_link) {
		if (d->d_addr == addr) {
			if (wait_busy) {
				iic_device_wait(sc, d);
			}
			return d;
		}
	}
	return NULL;
}

static struct i2c_device *
iic_device_lookup_dev(struct iic_softc *sc, device_t dev)
{
	struct i2c_device *d;

	KASSERT(mutex_owned(&sc->sc_device_state_lock));

	LIST_FOREACH(d, &sc->sc_devices, d_link) {
		if (d->d_dev == dev) {
			iic_device_wait(sc, d);
			return d;
		}
	}
	return NULL;
}

static struct i2c_device *
iic_device_alloc(i2c_addr_t addr, int flags)
{
	struct i2c_device *d = kmem_zalloc(sizeof(*d), KM_SLEEP);
	d->d_addr = addr;
	d->d_flags = flags;

	return d;
}

static inline void
iic_device_free(struct i2c_device *d)
{
	kmem_free(d, sizeof(*d));
}

#define	ID_F_KEEPALIVE_MASK	(ID_F_DIRECT | ID_F_BUSY | ID_F_WAIT_BUSY)

static void
iic_device_release_and_unlock(struct iic_softc *sc, struct i2c_device *d)
{
	KASSERT(mutex_owned(&sc->sc_device_state_lock));

	if (d->d_dev == NULL &&
	    (d->d_flags & ID_F_KEEPALIVE_MASK) == 0) {
		LIST_REMOVE(d, d_link);
	} else {
		if (d->d_flags & ID_F_WAIT_BUSY) {
			d->d_flags &= ~ID_F_WAIT_BUSY;
			cv_broadcast(&sc->sc_device_state_cond);
		}
		d = NULL;
	}
	mutex_exit(&sc->sc_device_state_lock);

	if (d != NULL) {
		iic_device_free(d);
	}
}

/*
 * iic_addr_reserve --
 *	Mark an I2C address as reserved by an attach attempt.
 *	This is a short-lived state.
 */
static bool
iic_addr_reserve(struct iic_softc *sc, i2c_addr_t addr, int flags)
{
	struct i2c_device *d, *newd;
	bool rv = true;

	flags = (flags & ID_F_DIRECT) | ID_F_BUSY;
	newd = iic_device_alloc(addr, flags);

	mutex_enter(&sc->sc_device_state_lock);
	d = iic_device_lookup_addr(sc, addr, true);
	if (d == NULL) {
		LIST_INSERT_HEAD(&sc->sc_devices, newd, d_link);
		newd = NULL;
		mutex_exit(&sc->sc_device_state_lock);
	} else {
		if (d->d_dev != NULL ||
		    ((d->d_flags & ID_F_DIRECT) != 0 &&
		     (     flags & ID_F_DIRECT) == 0)) {
			rv = false;
		} else {
			d->d_flags |= flags;
		}
		iic_device_release_and_unlock(sc, d);
	}

	if (newd != NULL) {
		kmem_free(newd, sizeof(*newd));
	}
	return rv;
}

/*
 * iic_addr_claim --
 *	Claim an I2C address for a device.  The address must already
 *	be reserved.
 */
static void
iic_addr_claim(struct iic_softc *sc, i2c_addr_t addr, device_t dev)
{
	struct i2c_device *d;

	mutex_enter(&sc->sc_device_state_lock);
	d = iic_device_lookup_addr(sc, addr, false);
	KASSERT(d != NULL);
	KASSERT(d->d_dev == NULL);
	KASSERT(d->d_flags & ID_F_BUSY);
	d->d_dev = dev;
	d->d_flags &= ~ID_F_BUSY;
	iic_device_release_and_unlock(sc, d);
}

/*
 * iic_addr_release --
 *	Release an I2C address.  Address must have been previously
 *	reserved but not claimed.
 */
static void
iic_addr_release(struct iic_softc *sc, i2c_addr_t addr)
{
	struct i2c_device *d;

	mutex_enter(&sc->sc_device_state_lock);
	d = iic_device_lookup_addr(sc, addr, false);
	KASSERT(d != NULL);
	KASSERT(d->d_dev == NULL);
	KASSERT(d->d_flags & ID_F_BUSY);
	d->d_flags &= ~ID_F_BUSY;
	iic_device_release_and_unlock(sc, d);
}

/*
 * iic_addr_release_device --
 *	Release an I2C address by device.
 */
static void
iic_addr_release_device(struct iic_softc *sc, device_t dev)
{
	struct i2c_device *d;

	mutex_enter(&sc->sc_device_state_lock);
	d = iic_device_lookup_dev(sc, dev);
	KASSERT(d != NULL);
	d->d_dev = NULL;
	iic_device_release_and_unlock(sc, d);
}

static int
iic_print_direct(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (pnp != NULL) {
		aprint_normal("%s%s%s%s at %s addr 0x%02x",
			      ia->ia_name ? ia->ia_name : "(unknown)",
			      ia->ia_clist ? " (" : "",
			      ia->ia_clist ? ia->ia_clist : "",
			      ia->ia_clist ? ")" : "",
			      pnp, ia->ia_addr);
	} else {
		aprint_normal(" addr 0x%02x", ia->ia_addr);
	}

	return UNCONF;
}

static int
iic_print(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr != (i2c_addr_t)IICCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%02x", ia->ia_addr);

	return UNCONF;
}

static bool
iic_is_special_address(i2c_addr_t addr)
{

	/*
	 * See: https://www.i2c-bus.org/addressing/
	 */

	/* General Call (read) / Start Byte (write) */
	if (addr == 0x00)
		return (true);

	/* CBUS Addresses */
	if (addr == 0x01)
		return (true);

	/* Reserved for Different Bus Formats */
	if (addr == 0x02)
		return (true);

	/* Reserved for future purposes */
	if (addr == 0x03)
		return (true);

	/* High Speed Master Code */
	if ((addr & 0x7c) == 0x04)
		return (true);

	/* 10-bit Slave Addressing prefix */
	if ((addr & 0x7c) == 0x78)
		return (true);

	/* Reserved for future purposes */
	if ((addr & 0x7c) == 0x7c)
		return (true);

	return (false);
}

static int
iic_probe_none(struct iic_softc *sc,
	       const struct i2c_attach_args *ia, int flags)
{

	return (0);
}

static int
iic_probe_smbus_quick_write(struct iic_softc *sc,
			    const struct i2c_attach_args *ia, int flags)
{
	int error;

	if ((error = iic_acquire_bus(ia->ia_tag, flags)) == 0) {
		error = iic_smbus_quick_write(ia->ia_tag, ia->ia_addr, flags);
	}
	(void) iic_release_bus(ia->ia_tag, flags);

	return (error);
}

static int
iic_probe_smbus_receive_byte(struct iic_softc *sc,
			     const struct i2c_attach_args *ia, int flags)
{
	int error;

	if ((error = iic_acquire_bus(ia->ia_tag, flags)) == 0) {
		uint8_t dummy;

		error = iic_smbus_receive_byte(ia->ia_tag, ia->ia_addr,
					       &dummy, flags);
	}
	(void) iic_release_bus(ia->ia_tag, flags);

	return (error);
}

static bool
iic_indirect_driver_is_permitted(struct iic_softc *sc, cfdata_t cf)
{
	prop_object_iterator_t iter;
	prop_array_t permitlist;
	prop_string_t pstr;
	prop_type_t ptype;
	bool rv = false;

	permitlist = prop_dictionary_get(device_properties(sc->sc_dev),
					 I2C_PROP_INDIRECT_DEVICE_PERMITLIST);
	if (permitlist == NULL) {
		/* No permitlist -> everything allowed */
		return (true);
	}

	if ((ptype = prop_object_type(permitlist)) != PROP_TYPE_ARRAY) {
		aprint_error_dev(sc->sc_dev,
		    "invalid property type (%d) for '%s'; must be array (%d)\n",
		    ptype, I2C_PROP_INDIRECT_DEVICE_PERMITLIST,
		    PROP_TYPE_ARRAY);
		return (false);
	}

	iter = prop_array_iterator(permitlist);
	while ((pstr = prop_object_iterator_next(iter)) != NULL) {
		if (prop_string_equals_string(pstr, cf->cf_name)) {
			rv = true;
			break;
		}
	}
	prop_object_iterator_release(iter);

	return (rv);
}

static int
iic_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct iic_softc *sc = device_private(parent);
	int (*probe_func)(struct iic_softc *,
			  const struct i2c_attach_args *, int);
	prop_string_t pstr;
	i2c_addr_t first_addr, last_addr;

	/*
	 * Before we do any more work, consult the allowed-driver
	 * permit-list for this bus (if any).
	 */
	if (iic_indirect_driver_is_permitted(sc, cf) == false)
		return (0);

	/* default to "quick write". */
	probe_func = iic_probe_smbus_quick_write;

	pstr = prop_dictionary_get(device_properties(sc->sc_dev),
				   I2C_PROP_INDIRECT_PROBE_STRATEGY);
	if (pstr == NULL) {
		/* Use the default. */
	} else if (prop_string_equals_string(pstr,
					I2C_PROBE_STRATEGY_QUICK_WRITE)) {
		probe_func = iic_probe_smbus_quick_write;
	} else if (prop_string_equals_string(pstr,
					I2C_PROBE_STRATEGY_RECEIVE_BYTE)) {
		probe_func = iic_probe_smbus_receive_byte;
	} else if (prop_string_equals_string(pstr,
					I2C_PROBE_STRATEGY_NONE)) {
		probe_func = iic_probe_none;
	} else {
		aprint_error_dev(sc->sc_dev,
			"unknown probe strategy '%s'; defaulting to '%s'\n",
			prop_string_value(pstr),
			I2C_PROBE_STRATEGY_QUICK_WRITE);

		/* Use the default. */
	}

	struct i2c_attach_args ia = {
		.ia_tag = sc->sc_tag,
	};

	if (cf->cf_loc[IICCF_ADDR] == IICCF_ADDR_DEFAULT) {
		/*
		 * This particular config directive has
		 * wildcarded the address, so we will
		 * scan the entire bus for it.
		 */
		first_addr = 0;
		last_addr = I2C_MAX_ADDR;
	} else {
		/*
		 * This config directive hard-wires the i2c
		 * bus address for the device, so there is
		 * no need to go poking around at any other
		 * addresses.
		 */
		if (cf->cf_loc[IICCF_ADDR] < 0 ||
		    cf->cf_loc[IICCF_ADDR] > I2C_MAX_ADDR) {
			/* Invalid config directive! */
			return (0);
		}
		first_addr = last_addr = cf->cf_loc[IICCF_ADDR];
	}

	for (ia.ia_addr = first_addr; ia.ia_addr <= last_addr; ia.ia_addr++) {
		int error, match_result;
		device_t newdev;

		/*
		 * Skip I2C addresses that are reserved for
		 * special purposes.
		 */
		if (iic_is_special_address(ia.ia_addr))
			continue;

		/*
		 * Skip addresses where a device is already attached
		 * or that's reserved for direct-configuration.
		 */
		if (! iic_addr_reserve(sc, ia.ia_addr, 0))
			continue;

		/*
		 * Call the "match" routine for the device.  If that
		 * returns success, then call the probe strategy
		 * function.
		 *
		 * We do it in this order because i2c devices tend
		 * to be found at a small number of possible addresses
		 * (e.g. read-time clocks that are only ever found at
		 * 0x68).  This gives the driver a chance to skip any
		 * address that are not valid for the device, saving
		 * us from having to poke at the bus to see if anything
		 * is there.
		 */
		match_result = config_probe(parent, cf, &ia);/*XXX*/
		if (match_result <= 0) {
			iic_addr_release(sc, ia.ia_addr);
			continue;
		}

		/*
		 * If the quality of the match by the driver was low
		 * (i.e. matched on being a valid address only, didn't
		 * perform any hardware probe), invoke our probe routine
		 * to see if it looks like something is really there.
		 */
		if (match_result == I2C_MATCH_ADDRESS_ONLY &&
		    (error = (*probe_func)(sc, &ia, 0)) != 0) {
			iic_addr_release(sc, ia.ia_addr);
			continue;
		}

		newdev = config_attach(parent, cf, &ia, iic_print, CFARGS_NONE);
		if (newdev != NULL) {
			iic_addr_claim(sc, ia.ia_addr, newdev);
		} else {
			iic_addr_release(sc, ia.ia_addr);
		}
	}

	return 0;
}

static void
iic_child_detach(device_t parent, device_t child)
{
	struct iic_softc *sc = device_private(parent);

	iic_addr_release_device(sc, child);
}

static int
iic_rescan(device_t self, const char *ifattr, const int *locators)
{
	config_search(self, NULL,
	    CFARGS(.search = iic_search,
		   .locators = locators));
	return 0;
}

static int
iic_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
iic_attach_child_direct(struct iic_softc *sc, struct i2c_attach_args *ia)
{
	device_t newdev;
	int loc[IICCF_NLOCS] = {
		[IICCF_ADDR] = ia->ia_addr,
	};

	if (ia->ia_addr > I2C_MAX_ADDR) {
		aprint_error_dev(sc->sc_dev,
		    "WARNING: ignoring bad device address @ 0x%x\n",
		    ia->ia_addr);
		return;
	}

	if (! iic_addr_reserve(sc, ia->ia_addr, ID_F_DIRECT)) {
		return;
	}

	newdev = config_found(sc->sc_dev, ia, iic_print_direct,
	    CFARGS(.submatch = config_stdsubmatch,
		   .locators = loc,
		   .devhandle = ia->ia_devhandle));
	if (newdev != NULL) {
		iic_addr_claim(sc, ia->ia_addr, newdev);
	} else {
		iic_addr_release(sc, ia->ia_addr);
	}
}

static bool
i2c_enumerate_devices_callback(device_t self,
    struct i2c_enumerate_devices_args *args)
{
	struct iic_softc *sc = device_private(self);

	iic_attach_child_direct(sc, args->ia);

	return true;				/* keep enumerating */
}

static bool
iic_attach_children_direct(struct iic_softc *sc)
{
	device_t parent = device_parent(sc->sc_dev);
	prop_array_t child_devices;
	bool no_indirect_config;

	child_devices = prop_dictionary_get(device_properties(parent),
					    "i2c-child-devices");
	if (!prop_dictionary_get_bool(device_properties(parent),
				      "i2c-no-indirect-config",
				      &no_indirect_config)) {
		no_indirect_config = false;
	}

	if (child_devices != NULL) {
		no_indirect_config = true;
	}

	/*
	 * If no explicit child device array is provided, then attempt
	 * to enumerate i2c devices using the platform device tree.
	 */
	struct i2c_attach_args ia = {
		.ia_tag = sc->sc_tag,
	};
	if (child_devices == NULL) {
		struct i2c_enumerate_devices_args enumargs = {
			.ia = &ia,
			.callback = i2c_enumerate_devices_callback,
		};
		if (device_call(sc->sc_dev,
				I2C_ENUMERATE_DEVICES(&enumargs)) == 0) {
			no_indirect_config = true;
		}
		goto done;
	}

	/*
	 * We have an explicit child device array to enumerate.
	 */
	prop_object_iterator_t iter = prop_array_iterator(child_devices);
	prop_dictionary_t dev;

	while ((dev = prop_object_iterator_next(iter)) != NULL) {
		const void *vptr;
		size_t vsize;

		if (!prop_dictionary_get_uint16(dev, "addr", &ia.ia_addr)) {
			continue;
		}

		if (!prop_dictionary_get_string(dev, "name", &ia.ia_name)) {
			/* "name" property is optional. */
			ia.ia_name = NULL;
		}

		if (!prop_dictionary_get_data(dev, "compatible",
					      (const void **)&ia.ia_clist,
					      &ia.ia_clist_size)) {
			ia.ia_clist = NULL;
			ia.ia_clist_size = 0;
		}

		if (!prop_dictionary_get_data(dev, "devhandle",
					      &vptr, &vsize)) {
			vptr = NULL;
		} else if (vsize != sizeof(ia.ia_devhandle)) {
			vptr = NULL;
		}
		if (vptr != NULL) {
			memcpy(&ia.ia_devhandle, vptr,
			    sizeof(ia.ia_devhandle));
		} else {
			ia.ia_devhandle = devhandle_invalid();
		}

		if ((ia.ia_name == NULL && ia.ia_clist == NULL) ||
		    ia.ia_addr > I2C_MAX_ADDR) {
			aprint_error_dev(sc->sc_dev,
			    "WARNING: ignoring bad child device entry "
			    "for address 0x%x\n", ia.ia_addr);
			continue;
		}

		iic_attach_child_direct(sc, &ia);
	}
	prop_object_iterator_release(iter);

 done:
	/*
	 * We return "true" if we want to let indirect configuration
	 * proceed.
	 */
	return !no_indirect_config;
}

static void
iic_attach(device_t parent, device_t self, void *aux)
{
	struct iic_softc *sc = device_private(self);
	devhandle_t devhandle = device_handle(self);
	struct i2cbus_attach_args *iba = aux;

	aprint_naive("\n");
	aprint_normal(": I2C bus\n");

	sc->sc_dev = self;
	sc->sc_tag = iba->iba_tag;

	LIST_INIT(&sc->sc_devices);
	cv_init(&sc->sc_device_state_cond, "i2cdevst");
	mutex_init(&sc->sc_device_state_lock, MUTEX_DEFAULT, IPL_NONE);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* XXX There ought to be a generic way to do this. */
	switch (devhandle_type(devhandle)) {
#ifdef I2C_USE_ACPI
	case DEVHANDLE_TYPE_ACPI:
		acpi_i2c_register(self, sc->sc_tag);
		break;
#endif
#ifdef I2C_USE_FDT
	case DEVHANDLE_TYPE_OF:
#if 0
		/*
		 * XXX The same unfortunate situation as SPI controllers can
		 * XXX happen when attaching an I2C controller on an otherwise
		 * XXX FDT platform (CI20) that does not happen to currently
		 * XXX have any platform SoC I2C controller drivers that carry
		 * XXX the fdt_i2c config attribute that would pull in the
		 * XXX function being called here.
		 * XXX
		 * XXX As it happens we can fairly safely elide this call
		 * XXX because, at the moment (15 Jan 2026), there are no
		 * XXX consumers of the registration it performs.  However,
		 * XXX this points to a larger problem if needed a way to
		 * XXX resolve these situations at runtime with some sort
		 * XXX of "platform" abstraction rather than at kernel build
		 * XXX time.
		 */
		fdtbus_register_i2c_controller(self, sc->sc_tag);
#endif

		break;
#endif
	default:
		break;
	}

	if (iic_attach_children_direct(sc)) {
		/*
		 * Attach all i2c devices described in the kernel
		 * configuration file.
		 */
		iic_rescan(self, "iic", NULL);
	}
}

static int
iic_detach(device_t self, int flags)
{
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	pmf_device_deregister(self);

	return 0;
}

/*
 * iic_compatible_match --
 *	Match a device's "compatible" property against the list
 *	of compatible strings provided by the driver.
 */
int
iic_compatible_match(const struct i2c_attach_args *ia,
		     const struct device_compatible_entry *compats)
{
	int match_result;

	match_result = device_compatible_match_strlist(ia->ia_clist,
	    ia->ia_clist_size, compats);
	if (match_result) {
		match_result =
		    MIN(I2C_MATCH_DIRECT_COMPATIBLE + match_result - 1,
			I2C_MATCH_DIRECT_COMPATIBLE_MAX);
	}

	return match_result;
}

/*
 * iic_compatible_lookup --
 *	Look the compatible entry that matches one of the driver's
 *	"compatible" strings.  The first match is returned.
 */
const struct device_compatible_entry *
iic_compatible_lookup(const struct i2c_attach_args *ia,
		      const struct device_compatible_entry *compats)
{
	return device_compatible_lookup_strlist(ia->ia_clist,
	    ia->ia_clist_size, compats);
}

/*
 * iic_use_direct_match --
 *	Helper for direct-config of i2c.  Returns true if this is
 *	a direct-config situation, along with match result.
 *	Returns false if the driver should use indirect-config
 *	matching logic.
 */
bool
iic_use_direct_match(const struct i2c_attach_args *ia, const cfdata_t cf,
		     const struct device_compatible_entry *compats,
		     int *match_resultp)
{
	KASSERT(match_resultp != NULL);

	if (ia->ia_name != NULL &&
	    strcmp(ia->ia_name, cf->cf_name) == 0) {
		*match_resultp = I2C_MATCH_DIRECT_SPECIFIC;
		return true;
	}

	if (ia->ia_clist != NULL && ia->ia_clist_size != 0) {
		*match_resultp = iic_compatible_match(ia, compats);
		return true;
	}

	return false;
}

static int
iic_open(dev_t dev, int flag, int fmt, lwp_t *l)
{
	struct iic_softc *sc = device_lookup_private(&iic_cd, minor(dev));

	mutex_enter(&iic_mtx);
	if (sc == NULL) {
		mutex_exit(&iic_mtx);
		return ENXIO;
	}
	iic_refcnt++;
	mutex_exit(&iic_mtx);

	return 0;
}

static int
iic_close(dev_t dev, int flag, int fmt, lwp_t *l)
{

	mutex_enter(&iic_mtx);
	iic_refcnt--;
	mutex_exit(&iic_mtx);

	return 0;
}

static int
iic_ioctl_exec(struct iic_softc *sc, i2c_ioctl_exec_t *iie, int flag)
{
	i2c_tag_t ic = sc->sc_tag;
	uint8_t *buf = NULL;
	void *cmd = NULL;
	int error = 0;

	/* Validate parameters */
	if (iie->iie_addr > I2C_MAX_ADDR)
		return EINVAL;
	if (iie->iie_cmdlen > I2C_EXEC_MAX_CMDLEN ||
	    iie->iie_buflen > I2C_EXEC_MAX_BUFLEN)
		return EINVAL;
	if (iie->iie_cmd != NULL && iie->iie_cmdlen == 0)
		return EINVAL;
	if (iie->iie_buf != NULL && iie->iie_buflen == 0)
		return EINVAL;
	if (I2C_OP_WRITE_P(iie->iie_op) && (flag & FWRITE) == 0)
		return EBADF;

#if 0
	/* Disallow userspace access to devices that have drivers attached. */
	/* XXX */
#endif

	if (iie->iie_cmd != NULL) {
		cmd = kmem_alloc(iie->iie_cmdlen, KM_SLEEP);
		error = copyin(iie->iie_cmd, cmd, iie->iie_cmdlen);
		if (error)
			goto out;
	}

	if (iie->iie_buf != NULL) {
		buf = kmem_alloc(iie->iie_buflen, KM_SLEEP);
		if (I2C_OP_WRITE_P(iie->iie_op)) {
			error = copyin(iie->iie_buf, buf, iie->iie_buflen);
			if (error)
				goto out;
		}
	}

	iic_acquire_bus(ic, 0);
	error = iic_exec(ic, iie->iie_op, iie->iie_addr, cmd, iie->iie_cmdlen,
	    buf, iie->iie_buflen, 0);
	iic_release_bus(ic, 0);

	/*
	 * Some drivers return error codes on failure, and others return -1.
	 */
	if (error < 0)
		error = EIO;

out:
	if (!error && iie->iie_buf != NULL && I2C_OP_READ_P(iie->iie_op))
		error = copyout(buf, iie->iie_buf, iie->iie_buflen);

	if (buf)
		kmem_free(buf, iie->iie_buflen);

	if (cmd)
		kmem_free(cmd, iie->iie_cmdlen);

	return error;
}

static int
iic_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct iic_softc *sc = device_lookup_private(&iic_cd, minor(dev));

	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case I2C_IOCTL_EXEC:
		return iic_ioctl_exec(sc, (i2c_ioctl_exec_t *)data, flag);
	default:
		return ENODEV;
	}
}


CFATTACH_DECL3_NEW(iic, sizeof(struct iic_softc),
    iic_match, iic_attach, iic_detach, NULL, iic_rescan, iic_child_detach,
    DVF_DETACH_SHUTDOWN);

MODULE(MODULE_CLASS_DRIVER, iic, "i2cexec,i2c_bitbang,i2c_subr");

#ifdef _MODULE
#include "ioconf.c"
#endif

int
iic_init(void)
{

	mutex_init(&iic_mtx, MUTEX_DEFAULT, IPL_NONE);
	iic_refcnt = 0;
	return 0;
}

static int
iic_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	int bmajor, cmajor;
#endif
	int error;

	error = 0;
	switch (cmd) {
	case MODULE_CMD_INIT:
		RUN_ONCE(&iic_once, iic_init);

#ifdef _MODULE
		mutex_enter(&iic_mtx);
		bmajor = cmajor = -1;
		error = devsw_attach("iic", NULL, &bmajor,
		    &iic_cdevsw, &cmajor);
		if (error != 0) {
			mutex_exit(&iic_mtx);
			break;
		}
		error = config_init_component(cfdriver_ioconf_iic,
		    cfattach_ioconf_iic, cfdata_ioconf_iic);
		if (error) {
			aprint_error("%s: unable to init component\n",
			    iic_cd.cd_name);
			devsw_detach(NULL, &iic_cdevsw);
		}
		mutex_exit(&iic_mtx);
#endif
		break;
	case MODULE_CMD_FINI:
		mutex_enter(&iic_mtx);
		if (iic_refcnt != 0) {
			mutex_exit(&iic_mtx);
			return EBUSY;
		}
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_iic,
		    cfattach_ioconf_iic, cfdata_ioconf_iic);
		if (error != 0) {
			mutex_exit(&iic_mtx);
			break;
		}
		devsw_detach(NULL, &iic_cdevsw);
#endif
		mutex_exit(&iic_mtx);
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
