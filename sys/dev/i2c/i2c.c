/*	$NetBSD: i2c.c,v 1.80.2.5 2021/09/10 15:45:28 thorpej Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2c.c,v 1.80.2.5 2021/09/10 15:45:28 thorpej Exp $");

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

#include <dev/i2c/i2cvar.h>

#include "ioconf.h"
#include "locators.h"

#ifndef I2C_MAX_ADDR
#define I2C_MAX_ADDR	0x3ff	/* 10-bit address, max */
#endif

struct i2c_device_link {
	TAILQ_ENTRY(i2c_device_link) l_list;
	device_t		l_device;
	i2c_addr_t		l_addr;
};

TAILQ_HEAD(i2c_devlist_head, i2c_device_link);

struct iic_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;

	kmutex_t sc_devlist_lock;
	struct i2c_devlist_head sc_devlist;
};

static dev_type_open(iic_open);
static dev_type_close(iic_close);
static dev_type_ioctl(iic_ioctl);

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
	.d_flag = D_OTHER | D_MCLOSE,
};

static void	iic_smbus_intr_thread(void *);

static kmutex_t iic_mtx;
static int iic_refcnt;
static bool iic_unloading;
static ONCE_DECL(iic_once);

static struct i2c_device_link *
iic_devslot_lookup(struct iic_softc *sc, i2c_addr_t addr)
{
	struct i2c_device_link *link;

	KASSERT(mutex_owned(&sc->sc_devlist_lock));

	/*
	 * A common pattern is "reserve then insert or delete", and
	 * this is often done in increasing address order.  So check
	 * if the last entry is the one we're looking for before we
	 * search the list from the front.
	 */
	link = TAILQ_LAST(&sc->sc_devlist, i2c_devlist_head);
	if (link == NULL) {
		/* List is empty. */
		return NULL;
	}
	if (link->l_addr == addr) {
		return link;
	}

	TAILQ_FOREACH(link, &sc->sc_devlist, l_list) {
		/*
		 * The list is sorted, so if the current list element
		 * has an address larger than the one we're looking
		 * for, then it's not in the list.
		 */
		if (link->l_addr > addr) {
			break;
		}
		if (link->l_addr == addr) {
			return link;
		}
	}
	return NULL;
}

static bool
iic_devslot_reserve(struct iic_softc *sc, i2c_addr_t addr)
{
	struct i2c_device_link *link, *new_link;

	new_link = kmem_zalloc(sizeof(*new_link), KM_SLEEP);
	new_link->l_addr = addr;

	mutex_enter(&sc->sc_devlist_lock);

	/* Optimize for reserving in increasing i2c address order. */
	link = TAILQ_LAST(&sc->sc_devlist, i2c_devlist_head);
	if (link == NULL || link->l_addr < new_link->l_addr) {
		TAILQ_INSERT_TAIL(&sc->sc_devlist, new_link, l_list);
		new_link = NULL;
		goto done;
	}
	KASSERT(!TAILQ_EMPTY(&sc->sc_devlist));

	/* Sort the new entry into the list. */
	TAILQ_FOREACH(link, &sc->sc_devlist, l_list) {
		if (link->l_addr < new_link->l_addr) {
			continue;
		}
		if (link->l_addr == new_link->l_addr) {
			/* Address is already reserved / in-use. */
			goto done;
		}
		/*
		 * If we get here, we know we should be inserted
		 * before this element, because we checked to see
		 * if we should be the last entry before entering
		 * the loop.
		 */
		KASSERT(link->l_addr > new_link->l_addr);
		TAILQ_INSERT_BEFORE(link, new_link, l_list);
		break;
	}
	/*
	 * Because we checked for an empty list early, if we got
	 * here it means we inserted before "link".
	 */
	KASSERT(link != NULL);
	KASSERT(TAILQ_NEXT(new_link, l_list) == link);
	new_link = NULL;

 done:
	mutex_exit(&sc->sc_devlist_lock);

	if (new_link != NULL) {
		kmem_free(new_link, sizeof(*new_link));
		return false;
	}
	return true;
}

static bool
iic_devslot_insert(struct iic_softc *sc, device_t dev, i2c_addr_t addr)
{
	struct i2c_device_link *link;
	bool rv = false;

	mutex_enter(&sc->sc_devlist_lock);

	link = iic_devslot_lookup(sc, addr);
	if (link != NULL) {
		if (link->l_device == NULL) {
			link->l_device = dev;
			rv = true;
		}
	}

	mutex_exit(&sc->sc_devlist_lock);

	return rv;
}

static bool
iic_devslot_remove(struct iic_softc *sc, device_t dev, i2c_addr_t addr)
{
	struct i2c_device_link *link;
	bool rv = false;

	mutex_enter(&sc->sc_devlist_lock);

	link = iic_devslot_lookup(sc, addr);
	if (link != NULL) {
		if (link->l_device == dev) {
			TAILQ_REMOVE(&sc->sc_devlist, link, l_list);
			rv = true;
		} else {
			link = NULL;
		}
	}

	mutex_exit(&sc->sc_devlist_lock);

	if (link != NULL) {
		kmem_free(link, sizeof(*link));
		return false;
	}
	return rv;
}

static bool
iic_devslot_set(struct iic_softc *sc, device_t dev, i2c_addr_t addr)
{
	return dev != NULL ? iic_devslot_insert(sc, dev, addr)
			   : iic_devslot_remove(sc, dev, addr);
}

static bool
iic_devslot_lookup_addr(struct iic_softc *sc, device_t dev, i2c_addr_t *addrp)
{
	struct i2c_device_link *link;
	bool rv = false;

	KASSERT(dev != NULL);
	KASSERT(addrp != NULL);

	mutex_enter(&sc->sc_devlist_lock);

	TAILQ_FOREACH(link, &sc->sc_devlist, l_list) {
		if (link->l_device == dev) {
			*addrp = link->l_addr;
			rv = true;
			break;
		}
	}

	mutex_exit(&sc->sc_devlist_lock);

	return rv;
}

static int
iic_print_direct(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (pnp != NULL)
		aprint_normal("%s%s%s%s at %s addr 0x%02x",
			      ia->ia_name ? ia->ia_name : "(unknown)",
			      ia->ia_clist ? " (" : "",
			      ia->ia_clist ? ia->ia_clist : "",
			      ia->ia_clist ? ")" : "",
			      pnp, ia->ia_addr);
	else
		aprint_normal(" addr 0x%02x", ia->ia_addr);

	return UNCONF;
}

static int
iic_print(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr != (i2c_addr_t)IICCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%x", ia->ia_addr);

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
	struct i2c_attach_args ia;
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

	memset(&ia, 0, sizeof(ia));
	ia.ia_tag = sc->sc_tag;

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
		bool rv __diagused;

		/*
		 * Skip I2C addresses that are reserved for
		 * special purposes.
		 */
		if (iic_is_special_address(ia.ia_addr))
			continue;

		/*
		 * Skip addresses where a device is already attached.
		 */
		if (! iic_devslot_reserve(sc, ia.ia_addr))
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
			rv = iic_devslot_remove(sc, NULL, ia.ia_addr);
			KASSERT(rv);
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
			rv = iic_devslot_remove(sc, NULL, ia.ia_addr);
			KASSERT(rv);
			continue;
		}

		newdev = config_attach(parent, cf, &ia, iic_print, CFARGS_NONE);
		rv = iic_devslot_set(sc, newdev, ia.ia_addr);
		KASSERT(rv);
	}

	return 0;
}

static void
iic_child_detach(device_t parent, device_t child)
{
	struct iic_softc *sc = device_private(parent);
	i2c_addr_t addr;
	bool rv __diagused;

	if (! iic_devslot_lookup_addr(sc, child, &addr)) {
		return;
	}

	rv = iic_devslot_remove(sc, child, addr);
	KASSERT(rv);
}

static int
iic_rescan(device_t self, const char *ifattr, const int *locators)
{
	config_search(self, NULL,
	    CFARGS(.search = iic_search,
		   .locators = locators));
	return 0;
}

static bool
iic_enumerate_devices_callback(device_t self,
    struct i2c_enumerate_devices_args *args)
{
	struct iic_softc *sc = device_private(self);
	int loc[IICCF_NLOCS] = { 0 };
	device_t newdev;
	bool rv __diagused;

	loc[IICCF_ADDR] = args->ia->ia_addr;

	if (args->ia->ia_addr > I2C_MAX_ADDR) {
		aprint_error_dev(self,
		    "WARNING: ignoring bad device address @ 0x%02x\n",
		    args->ia->ia_addr);
		return true;			/* keep enumerating */
	}
	if (iic_devslot_reserve(sc, args->ia->ia_addr)) {
		newdev = config_found(self, args->ia, iic_print_direct,
		    CFARGS(/* .submatch = config_stdsubmatch, XXX */
			   .locators = loc,
			   .devhandle = args->ia->ia_devhandle));
		rv = iic_devslot_set(sc, newdev, args->ia->ia_addr);
		KASSERT(rv);
	}
	return true;				/* keep enumerating */
}

static int
iic_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
iic_attach(device_t parent, device_t self, void *aux)
{
	struct iic_softc *sc = device_private(self);
	struct i2cbus_attach_args *iba = aux;
	i2c_tag_t ic;
	int rv;

	aprint_naive("\n");
	aprint_normal(": I2C bus\n");

	sc->sc_dev = self;
	sc->sc_tag = iba->iba_tag;

	ic = sc->sc_tag;
	ic->ic_devname = device_xname(self);

	TAILQ_INIT(&sc->sc_devlist);
	mutex_init(&sc->sc_devlist_lock, MUTEX_DEFAULT, IPL_NONE);

	LIST_INIT(&(sc->sc_tag->ic_list));
	LIST_INIT(&(sc->sc_tag->ic_proc_list));

	rv = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN, NULL,
	    iic_smbus_intr_thread, ic, &ic->ic_intr_thread,
	    "%s", ic->ic_devname);
	if (rv)
		aprint_error_dev(self, "unable to create intr thread\n");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/*
	 * Attempt to enumerate the devices on the bus.  If there is no
	 * enumeration method, then we will attempt indirect configuration.
	 */
	struct i2c_attach_args ia = {
		.ia_tag = ic,
	};
	struct i2c_enumerate_devices_args enumargs = {
		.ia = &ia,
		.callback = iic_enumerate_devices_callback,
	};

	rv = device_call(self, "i2c-enumerate-devices", &enumargs);
	if (rv == ENOTSUP) {
		/*
		 * Direct configuration is not supported on this
		 * bus; perform indirect configuration: attempt
		 * to attach the i2c devices listed in the kernel
		 * configuration file.
		 */
		iic_rescan(self, NULL, NULL);
	}
}

static int
iic_detach(device_t self, int flags)
{
	struct iic_softc *sc = device_private(self);
	i2c_tag_t ic = sc->sc_tag;
	struct i2c_device_link *link;
	device_t child;
	int error;
	void *hdl;

	/* Detach all children in address order. */
	for (;;) {
		mutex_enter(&sc->sc_devlist_lock);
		link = TAILQ_FIRST(&sc->sc_devlist);
		if (link == NULL) {
			mutex_exit(&sc->sc_devlist_lock);
			break;
		}
		child = link->l_device;
		mutex_exit(&sc->sc_devlist_lock);

		error = config_detach(child, flags);
		if (error) {
			return error;
		}
	}

	if (ic->ic_running) {
		ic->ic_running = 0;
		wakeup(ic);
		kthread_join(ic->ic_intr_thread);
	}

	if (!LIST_EMPTY(&ic->ic_list)) {
		device_printf(self, "WARNING: intr handler list not empty\n");
		while (!LIST_EMPTY(&ic->ic_list)) {
			hdl = LIST_FIRST(&ic->ic_list);
			iic_smbus_intr_disestablish(ic, hdl);
		}
	}
	if (!LIST_EMPTY(&ic->ic_proc_list)) {
		device_printf(self, "WARNING: proc handler list not empty\n");
		while (!LIST_EMPTY(&ic->ic_proc_list)) {
			hdl = LIST_FIRST(&ic->ic_proc_list);
			iic_smbus_intr_disestablish_proc(ic, hdl);
		}
	}

	pmf_device_deregister(self);

	return 0;
}

static void
iic_smbus_intr_thread(void *aux)
{
	i2c_tag_t ic;
	struct ic_intr_list *il;

	ic = (i2c_tag_t)aux;
	ic->ic_running = 1;
	ic->ic_pending = 0;

	while (ic->ic_running) {
		if (ic->ic_pending == 0)
			tsleep(ic, PZERO, "iicintr", hz);
		if (ic->ic_pending > 0) {
			LIST_FOREACH(il, &(ic->ic_proc_list), il_next) {
				(*il->il_intr)(il->il_intrarg);
			}
			ic->ic_pending--;
		}
	}

	kthread_exit(0);
}

void *
iic_smbus_intr_establish(i2c_tag_t ic, int (*intr)(void *), void *intrarg)
{
	struct ic_intr_list *il;

	il = malloc(sizeof(struct ic_intr_list), M_DEVBUF, M_WAITOK);
	if (il == NULL)
		return NULL;

	il->il_intr = intr;
	il->il_intrarg = intrarg;

	LIST_INSERT_HEAD(&(ic->ic_list), il, il_next);

	return il;
}

void
iic_smbus_intr_disestablish(i2c_tag_t ic, void *hdl)
{
	struct ic_intr_list *il;

	il = (struct ic_intr_list *)hdl;

	LIST_REMOVE(il, il_next);
	free(il, M_DEVBUF);

	return;
}

void *
iic_smbus_intr_establish_proc(i2c_tag_t ic, int (*intr)(void *), void *intrarg)
{
	struct ic_intr_list *il;

	il = malloc(sizeof(struct ic_intr_list), M_DEVBUF, M_WAITOK);
	if (il == NULL)
		return NULL;

	il->il_intr = intr;
	il->il_intrarg = intrarg;

	LIST_INSERT_HEAD(&(ic->ic_proc_list), il, il_next);

	return il;
}

void
iic_smbus_intr_disestablish_proc(i2c_tag_t ic, void *hdl)
{
	struct ic_intr_list *il;

	il = (struct ic_intr_list *)hdl;

	LIST_REMOVE(il, il_next);
	free(il, M_DEVBUF);

	return;
}

int
iic_smbus_intr(i2c_tag_t ic)
{
	struct ic_intr_list *il;

	LIST_FOREACH(il, &(ic->ic_list), il_next) {
		(*il->il_intr)(il->il_intrarg);
	}

	ic->ic_pending++;
	wakeup(ic);

	return 1;
}

/*
 * iic_compatible_match --
 *	Match a device's "compatible" property against the list
 *	of compatible strings provided by the driver.
 */
int
iic_compatible_match(const struct i2c_attach_args *ia,
		     const struct device_compatible_entry *compat_data)
{
	int match_result;

	match_result = device_compatible_match_strlist(ia->ia_clist,
	    ia->ia_clist_size, compat_data);
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
		      const struct device_compatible_entry *compat_data)
{
	return device_compatible_lookup_strlist(ia->ia_clist,
	    ia->ia_clist_size, compat_data);
}

/*
 * iic_use_direct_match --
 *	Helper for direct-config of i2c.  Returns true if this is
 *	a direct-config situation, along with with match result.
 *	Returns false if the driver should use indirect-config
 *	matching logic.
 */
bool
iic_use_direct_match(const struct i2c_attach_args *ia, const cfdata_t cf,
		     const struct device_compatible_entry *compat_data,
		     int *match_resultp)
{
	KASSERT(match_resultp != NULL);

	/* XXX Should not really be using "name". */
	if (ia->ia_name != NULL &&
	    strcmp(ia->ia_name, cf->cf_name) == 0) {
		*match_resultp = I2C_MATCH_DIRECT_SPECIFIC;
		return true;
	}

	if (ia->ia_clist != NULL) {
		KASSERT(ia->ia_clist_size != 0);
		*match_resultp = iic_compatible_match(ia, compat_data);
		return true;
	}

	return false;
}

static int
iic_open(dev_t dev, int flag, int fmt, lwp_t *l)
{
	struct iic_softc *sc;

	mutex_enter(&iic_mtx);

	if (iic_unloading) {
		mutex_exit(&iic_mtx);
		return ENXIO;
	}

	/* Hold a refrence while we look up the softc. */
	if (iic_refcnt == INT_MAX) {
		mutex_exit(&iic_mtx);
		return EBUSY;
	}
	iic_refcnt++;

	mutex_exit(&iic_mtx);

	sc = device_lookup_private(&iic_cd, minor(dev));

	if (sc == NULL) {
		mutex_enter(&iic_mtx);
		iic_refcnt--;
		mutex_exit(&iic_mtx);
		return ENXIO;
	}

	return 0;
}

static int
iic_close(dev_t dev, int flag, int fmt, lwp_t *l)
{
	struct iic_softc *sc __diagused =
	    device_lookup_private(&iic_cd, minor(dev));;

	KASSERT(iic_refcnt != 0);
	KASSERT(sc != NULL);

	mutex_enter(&iic_mtx);
	iic_refcnt--;
	mutex_exit(&iic_mtx);

	return 0;
}

static int
iic_ioctl_exec(struct iic_softc *sc, i2c_ioctl_exec_t *iie, int flag)
{
	i2c_tag_t ic = sc->sc_tag;
	uint8_t buf[I2C_EXEC_MAX_BUFLEN];
	void *cmd = NULL;
	int error;

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

	if (iie->iie_cmd != NULL) {
		cmd = kmem_alloc(iie->iie_cmdlen, KM_SLEEP);
		error = copyin(iie->iie_cmd, cmd, iie->iie_cmdlen);
		if (error)
			goto out;
	}

	if (iie->iie_buf != NULL && I2C_OP_WRITE_P(iie->iie_op)) {
		error = copyin(iie->iie_buf, buf, iie->iie_buflen);
		if (error)
			goto out;
	}

	if ((error = iic_acquire_bus(ic, 0)) != 0) {
		goto out;
	}
	error = iic_exec(ic, iie->iie_op, iie->iie_addr, cmd, iie->iie_cmdlen,
	    buf, iie->iie_buflen, 0);
	iic_release_bus(ic, 0);

	/*
	 * Some drivers return error codes on failure, and others return -1.
	 */
	if (error < 0)
		error = EIO;

out:
	if (cmd)
		kmem_free(cmd, iie->iie_cmdlen);

	if (error)
		return error;

	if (iie->iie_buf != NULL && I2C_OP_READ_P(iie->iie_op))
		error = copyout(buf, iie->iie_buf, iie->iie_buflen);

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

MODULE(MODULE_CLASS_DRIVER, iic, "i2cexec,i2c_bitbang");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
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
			(void)devsw_detach(NULL, &iic_cdevsw);
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
		iic_unloading = true;
		mutex_exit(&iic_mtx);
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_iic,
		    cfattach_ioconf_iic, cfdata_ioconf_iic);
		if (error != 0) {
			mutex_enter(&iic_mtx);
			iic_unloading = false;
			mutex_exit(&iic_mtx);
			break;
		}
		error = devsw_detach(NULL, &iic_cdevsw);
		if (error != 0) {
			config_init_component(cfdriver_ioconf_iic,
			    cfattach_ioconf_iic, cfdata_ioconf_iic);
			mutex_enter(&iic_mtx);
			iic_unloading = false;
			mutex_exit(&iic_mtx);
		}
#endif
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
