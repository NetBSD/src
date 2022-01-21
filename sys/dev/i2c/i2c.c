/*	$NetBSD: i2c.c,v 1.82 2022/01/21 15:55:36 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i2c.c,v 1.82 2022/01/21 15:55:36 thorpej Exp $");

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
#endif /* I2C_USE_ACPI */

#ifdef I2C_USE_FDT
#include <dev/fdt/fdtvar.h>
#endif /* I2C_USE_FDT */

#include <dev/i2c/i2cvar.h>

#include "ioconf.h"
#include "locators.h"

#ifndef I2C_MAX_ADDR
#define I2C_MAX_ADDR	0x3ff	/* 10-bit address, max */
#endif

struct iic_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	device_t sc_devices[I2C_MAX_ADDR + 1];
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

static void	iic_smbus_intr_thread(void *);
static void	iic_fill_compat(struct i2c_attach_args*, const char*,
			size_t, char **);

static int
iic_print_direct(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s addr 0x%02x",
			      ia->ia_name ? ia->ia_name : "(unknown)",
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

	ia.ia_tag = sc->sc_tag;

	ia.ia_name = NULL;
	ia.ia_ncompat = 0;
	ia.ia_compat = NULL;
	ia.ia_prop = NULL;

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

		/*
		 * Skip I2C addresses that are reserved for
		 * special purposes.
		 */
		if (iic_is_special_address(ia.ia_addr))
			continue;

		/*
		 * Skip addresses where a device is already attached.
		 */
		if (sc->sc_devices[ia.ia_addr] != NULL)
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
		if (match_result <= 0)
			continue;

		/*
		 * If the quality of the match by the driver was low
		 * (i.e. matched on being a valid address only, didn't
		 * perform any hardware probe), invoke our probe routine
		 * to see if it looks like something is really there.
		 */
		if (match_result == I2C_MATCH_ADDRESS_ONLY &&
		    (error = (*probe_func)(sc, &ia, 0)) != 0)
			continue;

		sc->sc_devices[ia.ia_addr] =
		    config_attach(parent, cf, &ia, iic_print, CFARGS_NONE);
	}

	return 0;
}

static void
iic_child_detach(device_t parent, device_t child)
{
	struct iic_softc *sc = device_private(parent);
	int i;

	for (i = 0; i <= I2C_MAX_ADDR; i++)
		if (sc->sc_devices[i] == child) {
			sc->sc_devices[i] = NULL;
			break;
		}
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
iic_attach(device_t parent, device_t self, void *aux)
{
	struct iic_softc *sc = device_private(self);
	struct i2cbus_attach_args *iba = aux;
	prop_array_t child_devices;
	prop_dictionary_t props;
	char *buf;
	i2c_tag_t ic;
	int rv;
	bool no_indirect_config = false;

	aprint_naive("\n");
	aprint_normal(": I2C bus\n");

	sc->sc_dev = self;
	sc->sc_tag = iba->iba_tag;
	ic = sc->sc_tag;
	ic->ic_devname = device_xname(self);

	LIST_INIT(&(sc->sc_tag->ic_list));
	LIST_INIT(&(sc->sc_tag->ic_proc_list));

	rv = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN, NULL,
	    iic_smbus_intr_thread, ic, &ic->ic_intr_thread,
	    "%s", ic->ic_devname);
	if (rv)
		aprint_error_dev(self, "unable to create intr thread\n");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if (iba->iba_child_devices) {
		child_devices = iba->iba_child_devices;
		no_indirect_config = true;
	} else {
		props = device_properties(parent);
		prop_dictionary_get_bool(props, "i2c-no-indirect-config",
		    &no_indirect_config);
		child_devices = prop_dictionary_get(props, "i2c-child-devices");
	}

	if (child_devices) {
		unsigned int i, count;
		prop_dictionary_t dev;
		prop_data_t cdata;
		uint32_t addr;
		uint64_t cookie;
		uint32_t cookietype;
		const char *name;
		struct i2c_attach_args ia;
		devhandle_t devhandle;
		int loc[IICCF_NLOCS];

		memset(loc, 0, sizeof loc);
		count = prop_array_count(child_devices);
		for (i = 0; i < count; i++) {
			dev = prop_array_get(child_devices, i);
			if (!dev) continue;
 			if (!prop_dictionary_get_string(
			    dev, "name", &name)) {
				/* "name" property is optional. */
				name = NULL;
			}
			if (!prop_dictionary_get_uint32(dev, "addr", &addr))
				continue;
			if (!prop_dictionary_get_uint64(dev, "cookie", &cookie))
				cookie = 0;
			if (!prop_dictionary_get_uint32(dev, "cookietype",
			    &cookietype))
				cookietype = I2C_COOKIE_NONE;
			loc[IICCF_ADDR] = addr;

			memset(&ia, 0, sizeof ia);
			ia.ia_addr = addr;
			ia.ia_tag = ic;
			ia.ia_name = name;
			ia.ia_cookie = cookie;
			ia.ia_cookietype = cookietype;
			ia.ia_prop = dev;

			devhandle = devhandle_invalid();
#ifdef I2C_USE_FDT
			if (cookietype == I2C_COOKIE_OF) {
				devhandle = devhandle_from_of((int)cookie);
			}
#endif /* I2C_USE_FDT */
#ifdef I2C_USE_ACPI
			if (cookietype == I2C_COOKIE_ACPI) {
				devhandle =
				    devhandle_from_acpi((ACPI_HANDLE)cookie);
			}
#endif /* I2C_USE_ACPI */

			buf = NULL;
			cdata = prop_dictionary_get(dev, "compatible");
			if (cdata)
				iic_fill_compat(&ia,
				    prop_data_value(cdata),
				    prop_data_size(cdata), &buf);

			if (name == NULL && cdata == NULL) {
				aprint_error_dev(self,
				    "WARNING: ignoring bad child device entry "
				    "for address 0x%02x\n", addr);
			} else {
				if (addr > I2C_MAX_ADDR) {
					aprint_error_dev(self,
					    "WARNING: ignoring bad device "
					    "address @ 0x%02x\n", addr);
				} else if (sc->sc_devices[addr] == NULL) {
					sc->sc_devices[addr] =
					    config_found(self, &ia,
					    iic_print_direct,
					    CFARGS(.locators = loc,
						   .devhandle = devhandle));
				}
			}

			if (ia.ia_compat)
				free(ia.ia_compat, M_TEMP);
			if (buf)
				free(buf, M_TEMP);
		}
	} else if (!no_indirect_config) {
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
	struct iic_softc *sc = device_private(self);
	i2c_tag_t ic = sc->sc_tag;
	int i, error;
	void *hdl;

	for (i = 0; i <= I2C_MAX_ADDR; i++) {
		if (sc->sc_devices[i]) {
			error = config_detach(sc->sc_devices[i], flags);
			if (error)
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

static void
iic_fill_compat(struct i2c_attach_args *ia, const char *compat, size_t len,
	char **buffer)
{
	int count, i;
	const char *c, *start, **ptr;

	*buffer = NULL;
	for (i = count = 0, c = compat; i < len; i++, c++)
		if (*c == 0)
			count++;
	count += 2;
	ptr = malloc(sizeof(char*)*count, M_TEMP, M_WAITOK);
	if (!ptr) return;

	for (i = count = 0, start = c = compat; i < len; i++, c++) {
		if (*c == 0) {
			ptr[count++] = start;
			start = c+1;
		}
	}
	if (start < compat+len) {
		/* last string not 0 terminated */
		size_t l = c-start;
		*buffer = malloc(l+1, M_TEMP, M_WAITOK);
		memcpy(*buffer, start, l);
		(*buffer)[l] = 0;
		ptr[count++] = *buffer;
	}
	ptr[count] = NULL;

	ia->ia_compat = ptr;
	ia->ia_ncompat = count;
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

	match_result = device_compatible_match(ia->ia_compat, ia->ia_ncompat,
					       compats);
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
	return device_compatible_lookup(ia->ia_compat, ia->ia_ncompat,
					compats);
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
		     const struct device_compatible_entry *compats,
		     int *match_resultp)
{
	KASSERT(match_resultp != NULL);

	if (ia->ia_name != NULL &&
	    strcmp(ia->ia_name, cf->cf_name) == 0) {
		*match_resultp = I2C_MATCH_DIRECT_SPECIFIC;
		return true;
	}

	if (ia->ia_ncompat > 0 && ia->ia_compat != NULL) {
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

#if 0
	/* Disallow userspace access to devices that have drivers attached. */
	if (sc->sc_devices[iie->iie_addr] != NULL)
		return EBUSY;
#endif

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
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_iic,
		    cfattach_ioconf_iic, cfdata_ioconf_iic);
		if (error != 0) {
			mutex_exit(&iic_mtx);
			break;
		}
		error = devsw_detach(NULL, &iic_cdevsw);
		if (error != 0)
			config_init_component(cfdriver_ioconf_iic,
			    cfattach_ioconf_iic, cfdata_ioconf_iic);
#endif
		mutex_exit(&iic_mtx);
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
