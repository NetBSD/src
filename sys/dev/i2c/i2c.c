/*	$NetBSD: i2c.c,v 1.27 2011/08/02 18:46:35 pgoyette Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2c.c,v 1.27 2011/08/02 18:46:35 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/event.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <dev/i2c/i2cvar.h>

#include "locators.h"
#include <opt_i2cbus.h>

#define I2C_MAX_ADDR	0x3ff	/* 10-bit address, max */

struct iic_softc {
	i2c_tag_t sc_tag;
	int sc_type;
	device_t sc_devices[I2C_MAX_ADDR + 1];
};

static void	iic_smbus_intr_thread(void *);
static void	iic_fill_compat(struct i2c_attach_args*, const char*,
			size_t, char **);

int
iicbus_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		aprint_normal("iic at %s", pnp);

	return (UNCONF);
}

static int
iic_print_direct(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s addr 0x%02x", ia->ia_name, pnp,
			ia->ia_addr);
	else
		aprint_normal(" addr 0x%02x", ia->ia_addr);

	return UNCONF;
}

static int
iic_print(void *aux, const char *pnp)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr != (i2c_addr_t)-1)
		aprint_normal(" addr 0x%x", ia->ia_addr);

	return (UNCONF);
}

static int
iic_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct iic_softc *sc = device_private(parent);
	struct i2c_attach_args ia;

	ia.ia_tag = sc->sc_tag;
	ia.ia_addr = cf->cf_loc[IICCF_ADDR];
	ia.ia_size = cf->cf_loc[IICCF_SIZE];
	ia.ia_type = sc->sc_type;

	ia.ia_name = NULL;
	ia.ia_ncompat = 0;
	ia.ia_compat = NULL;

	if (config_match(parent, cf, &ia) > 0) {
		if (ia.ia_addr == (i2c_addr_t)-1) 
			config_attach(parent, cf, &ia, iic_print);
		else if (ia.ia_addr <= I2C_MAX_ADDR &&
			   !sc->sc_devices[ia.ia_addr])
			sc->sc_devices[ia.ia_addr] =
			    config_attach(parent, cf, &ia, iic_print);
	}

	return (0);
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
	config_search_ia(iic_search, self, ifattr, NULL);
	return 0;
}

static int
iic_match(device_t parent, cfdata_t cf, void *aux)
{

	return (1);
}

static void
iic_attach(device_t parent, device_t self, void *aux)
{
	struct iic_softc *sc = device_private(self);
	struct i2cbus_attach_args *iba = aux;
	prop_array_t child_devices;
	char *buf;
	i2c_tag_t ic;
	int rv;

	aprint_naive(": I2C bus\n");
	aprint_normal(": I2C bus\n");

	sc->sc_tag = iba->iba_tag;
	sc->sc_type = iba->iba_type;
	ic = sc->sc_tag;
	ic->ic_devname = device_xname(self);

	LIST_INIT(&(sc->sc_tag->ic_list));
	LIST_INIT(&(sc->sc_tag->ic_proc_list));

	rv = kthread_create(PRI_NONE, 0, NULL, iic_smbus_intr_thread,
	    ic, &ic->ic_intr_thread, "%s", ic->ic_devname);
	if (rv)
		aprint_error_dev(self, "unable to create intr thread\n");

#if I2C_SCAN
	if (sc->sc_type == I2C_TYPE_SMBUS) {
		int err;
		int found = 0;
		i2c_addr_t addr;
		uint8_t val;

		for (addr = 0x09; addr < 0x78; addr++) {
			/*
			 * Skip certain i2c addresses:
			 *	0x00		General Call / START
			 *	0x01		CBUS Address
			 *	0x02		Different Bus format
			 *	0x03 - 0x07	Reserved
			 *	0x08		Host Address
			 *	0x0c		Alert Response Address
			 *	0x28		ACCESS.Bus host
			 *	0x37		ACCESS.Bus default address
			 *	0x48 - 0x4b	Prototypes
			 *	0x61		Device Default Address
			 *	0x78 - 0x7b	10-bit addresses
			 *	0x7c - 0x7f	Reserved
			 *
			 * Some of these are skipped by judicious selection
			 * of the range of the above for (;;) statement.
			 *
			 * if (addr <= 0x08 || addr >= 0x78)
			 *	continue;
			 */
			if (addr == 0x0c || addr == 0x28 || addr == 0x37 ||
			    addr == 0x61 || (addr & 0x7c) == 0x48)
				continue;

			iic_acquire_bus(ic, 0);
			/*
			 * Use SMBus quick_write command to detect most
			 * addresses;  should avoid hanging the bus on
			 * some write-only devices (like clocks that show
			 * up at address 0x69)
			 *
			 * XXX The quick_write() is allegedly known to
			 * XXX corrupt the Atmel AT24RF08 EEPROM found
			 * XXX on some IBM Thinkpads!
			 */
			if ((addr & 0xf8) == 0x30 ||
			    (addr & 0xf0) == 0x50)
				err = iic_smbus_receive_byte(ic, addr, &val, 0);
			else
				err = iic_smbus_quick_write(ic, addr, 0);
			if (err == 0) {
				if (found == 0)
					aprint_normal("%s: devices at",
							ic->ic_devname);
				found++;
				aprint_normal(" 0x%02x", addr);
			}
			iic_release_bus(ic, 0);
		}
		if (found == 0)
			aprint_normal("%s: no devices found", ic->ic_devname);
		aprint_normal("\n");
	}
#endif

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	child_devices = prop_dictionary_get(device_properties(parent),
		"i2c-child-devices");
	if (child_devices) {
		unsigned int i, count;
		prop_dictionary_t dev;
		prop_data_t cdata;
		uint32_t addr, size;
		uint64_t cookie;
		const char *name;
		struct i2c_attach_args ia;
		int loc[2];

		memset(loc, 0, sizeof loc);
		count = prop_array_count(child_devices);
		for (i = 0; i < count; i++) {
			dev = prop_array_get(child_devices, i);
			if (!dev) continue;
 			if (!prop_dictionary_get_cstring_nocopy(
			    dev, "name", &name))
				continue;
			if (!prop_dictionary_get_uint32(dev, "addr", &addr))
				continue;
			if (!prop_dictionary_get_uint64(dev, "cookie", &cookie))
				cookie = 0;
			loc[0] = addr;
			if (prop_dictionary_get_uint32(dev, "size", &size))
				loc[1] = size;
			else
				loc[1] = -1;

			memset(&ia, 0, sizeof ia);
			ia.ia_addr = addr;
			ia.ia_type = sc->sc_type;
			ia.ia_tag = ic;
			ia.ia_name = name;
			ia.ia_cookie = cookie;

			buf = NULL;
			cdata = prop_dictionary_get(dev, "compatible");
			if (cdata)
				iic_fill_compat(&ia,
				    prop_data_data_nocopy(cdata),
				    prop_data_size(cdata), &buf);

			config_found_sm_loc(self, "iic", loc, &ia,
			    iic_print_direct, NULL);

			if (ia.ia_compat)
				free(ia.ia_compat, M_TEMP);
			if (buf)
				free(buf, M_TEMP);
		}
	} else {
		/*
		 * Attach all i2c devices described in the kernel
		 * configuration file.
		 */
		iic_rescan(self, "iic", NULL);
	}
}

static void
iic_smbus_intr_thread(void *aux)
{
	i2c_tag_t ic;
	struct ic_intr_list *il;
	int rv;

	ic = (i2c_tag_t)aux;
	ic->ic_running = 1;
	ic->ic_pending = 0;

	while (ic->ic_running) {
		if (ic->ic_pending == 0)
			rv = tsleep(ic, PZERO, "iicintr", hz);
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

int
iic_compat_match(struct i2c_attach_args *ia, const char ** compats)
{
	int i;

	for (; compats && *compats; compats++) {
		for (i = 0; i < ia->ia_ncompat; i++) {
			if (strcmp(*compats, ia->ia_compat[i]) == 0)
				return 1;
		}
	}
	return 0;
}


CFATTACH_DECL2_NEW(iic, sizeof(struct iic_softc),
    iic_match, iic_attach, NULL, NULL, iic_rescan, iic_child_detach);
