/* $NetBSD: spdmem_i2c.c,v 1.22.6.1 2021/08/09 00:30:09 thorpej Exp $ */

/*
 * Copyright (c) 2007 Nicolas Joly
 * Copyright (c) 2007 Paul Goyette
 * Copyright (c) 2007 Tobias Nygren
 * Copyright (c) 2015 Michael van Elst
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
 * Serial Presence Detect (SPD) memory identification
 *
 * JEDEC standard No. 21-C
 * JEDEC document 4_01_06R24
 * - Definitions of the EE1004-v 4 Kbit Serial Presence Detect EEPROM [...]
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spdmem_i2c.c,v 1.22.6.1 2021/08/09 00:30:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <machine/bswap.h>

#include <dev/i2c/i2cvar.h>
#include <dev/ic/spdmemreg.h>
#include <dev/ic/spdmemvar.h>

/* Constants for matching i2c bus address */
#define SPDMEM_I2C_ADDRMASK 0xfff8
#define SPDMEM_I2C_ADDR     0x50
#define SPDCTL_I2C_ADDR     0x30

/* set write protection */
#define SPDCTL_SWP0         (SPDCTL_I2C_ADDR + 1)
#define SPDCTL_SWP1         (SPDCTL_I2C_ADDR + 4)
#define SPDCTL_SWP2         (SPDCTL_I2C_ADDR + 5)
#define SPDCTL_SWP3         (SPDCTL_I2C_ADDR + 0)

/* clear write protections */
#define SPDCTL_CWP          (SPDCTL_I2C_ADDR + 3)

/* read protection status */
#define SPDCTL_RPS0         (SPDCTL_I2C_ADDR + 1)
#define SPDCTL_RPS1         (SPDCTL_I2C_ADDR + 4)
#define SPDCTL_RPS2         (SPDCTL_I2C_ADDR + 5)
#define SPDCTL_RPS3         (SPDCTL_I2C_ADDR + 0)

/* select page address */
#define SPDCTL_SPA0         (SPDCTL_I2C_ADDR + 6)
#define SPDCTL_SPA1         (SPDCTL_I2C_ADDR + 7)

/* read page address */
#define SPDCTL_RPA          (SPDCTL_I2C_ADDR + 6)

struct spdmem_i2c_softc {
	struct spdmem_softc sc_base;
	i2c_tag_t sc_tag;
	i2c_addr_t sc_addr; /* EEPROM */
	i2c_addr_t sc_page0;
	i2c_addr_t sc_page1;
};

static int  spdmem_reset_page(struct spdmem_i2c_softc *);
static int  spdmem_i2c_match(device_t, cfdata_t, void *);
static void spdmem_i2c_attach(device_t, device_t, void *);
static int  spdmem_i2c_detach(device_t, int);

CFATTACH_DECL_NEW(spdmem_iic, sizeof(struct spdmem_i2c_softc),
    spdmem_i2c_match, spdmem_i2c_attach, spdmem_i2c_detach, NULL);

static int spdmem_i2c_read(struct spdmem_softc *, uint16_t, uint8_t *);

static int
spdmem_reset_page(struct spdmem_i2c_softc *sc)
{
	uint8_t reg, byte0, byte2;
	static uint8_t dummy = 0;
	int rv;

	reg = 0;

	rv = iic_acquire_bus(sc->sc_tag, 0);
	if (rv)
		return rv;

	/*
	 * Try to read byte 0 and 2. If it failed, it's not spdmem or a device
	 * doesn't exist at the address.
	 */
	rv = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, &reg, 1,
	    &byte0, 1, 0);
	rv |= iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, &reg, 1,
	    &byte2, 1, 0);
	if (rv != 0)
		goto error;

	/*
	 * Quirk for BIOSes that leave page 1 of a 4kbit EEPROM selected.
	 *
	 * byte0 is the length, byte2 is the memory type. Both of them should
	 * not be zero. If zero, the current page might be 1 (DDR4 and newer).
	 * If page 1 is selected, offset 0 can be 0 (Module Characteristics
	 * (Energy backup is not available)) and also offset 2 can be 0
	 * (Megabytes, and a part of Capacity digits).
	 *
	 * Note: The encoding of byte0 is vary in memory type, so we check
	 * just with zero to be simple.
	 *
	 * Try to see if we are not at page 0. If it's not, select page 0.
	 */
	if ((byte0 == 0) || (byte2 == 0)) {
		/*
		 * Note that SDCTL_RPA is the same as sc->sc_page0(SPDCTL_SPA0)
		 * Write is SPA0, read is RPA.
		 *
		 * This call returns 0 on page 0 and returns -1 on page 1.
		 * I don't know whether our icc_exec()'s API is good or not.
		 */
		rv = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_page0,
		    &reg, 1, &dummy, 1, 0);
		if (rv != 0) {
			/*
			 * The possibilities are:
			 * a) page 1 is selected.
			 * b) The device doesn't support page select and
			 *    it's not a SPD ROM.
			 * Is there no way to distinguish them now?
			 */
			rv = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
			    sc->sc_page0, &reg, 1, &dummy, 1, 0);
			if (rv == 0) {
				aprint_debug("Page 1 was selected. Page 0 is "
				    "selected now.\n");
			} else {
				aprint_debug("Failed to select page 0. This "
				    "device isn't SPD ROM\n");
			}
		} else {
			/* This device isn't SPD ROM */
			rv = -1;
		}
	}
error:
	iic_release_bus(sc->sc_tag, 0);

	return rv;
}

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "atmel,spd" },
	{ .compat = "i2c-at34c02" },
	DEVICE_COMPAT_EOL
};

/*
 * Some device trees don't have real "compatible" entries, so we
 * end up having to match by name.
 */
static const struct device_compatible_entry name_data[] = {
	{ .compat = "dimm-spd" },
	{ .compat = "dimm" },
	DEVICE_COMPAT_EOL
};

#define	SPDMEM_HIGH_CONFIDENCE_MATCH	(I2C_MATCH_DIRECT_COMPATIBLE + 20)

static bool
spdmem_i2c_use_name_match(const struct i2c_attach_args *ia, int *match_resultp)
{
	const char *name = ia->ia_name;

	if (name != NULL) {
		*match_resultp = device_compatible_match(&name, 1, name_data)
		    ? SPDMEM_HIGH_CONFIDENCE_MATCH
		    : 0;
		return true;
	}
	return false;
}

static bool
spdmem_i2c_use_direct_match(const struct i2c_attach_args *ia, const cfdata_t cf,
			    const struct device_compatible_entry *cdata,
			    int *match_resultp)
{
	/*
	 * Matching by name is not ideal, but some device trees only
	 * have a name and no "compatible" property.
	 */
	if (spdmem_i2c_use_name_match(ia, match_resultp))
		return true;

	if (iic_use_direct_match(ia, cf, cdata, match_resultp))
		return true;

	return false;
}

static int
spdmem_i2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	struct spdmem_i2c_softc sc;
	int match_result;

	if (spdmem_i2c_use_direct_match(ia, match, compat_data, &match_result))
		goto do_probe;

	/* Filter out by address when not using direct config. */
	if ((ia->ia_addr & SPDMEM_I2C_ADDRMASK) != SPDMEM_I2C_ADDR)
		return 0;

	sc.sc_tag = ia->ia_tag;
	sc.sc_addr = ia->ia_addr;
	sc.sc_page0 = SPDCTL_SPA0;
	sc.sc_page1 = SPDCTL_SPA1;
	sc.sc_base.sc_read = spdmem_i2c_read;
	match_result = 0;

 do_probe:
	/* Check the bank and reset to the page 0 */
	if (spdmem_reset_page(&sc) != 0)
		return 0;

	if (spdmem_common_probe(&sc.sc_base)) {
		if (match_result < SPDMEM_HIGH_CONFIDENCE_MATCH) {
			match_result = SPDMEM_HIGH_CONFIDENCE_MATCH;
		}
	}

	return match_result;
}

static void
spdmem_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct spdmem_i2c_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	int match_result;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_page0 = SPDCTL_SPA0;
	sc->sc_page1 = SPDCTL_SPA1;
	sc->sc_base.sc_read = spdmem_i2c_read;

	/*
	 * SPD stands for "Serial Presence Detect".  If we're using
	 * direct configuration, the device tree may have given us
	 * a location where and SPD memory modudle *could* be found,
	 * not necessarily where it is known to be present.
	 *
	 * Accordingly, if we get a direct configuration match based
	 * on compatible data or device name, we still check to see
	 * if the device is there.
	 */
	if (spdmem_i2c_use_direct_match(ia, device_cfdata(self), compat_data,
					&match_result)) {
		if (! spdmem_common_probe(&sc->sc_base)) {
			aprint_normal(": module not present\n");
			return;
		}
	}

	spdmem_common_attach(&sc->sc_base, self);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
spdmem_i2c_detach(device_t self, int flags)
{
	struct spdmem_i2c_softc *sc = device_private(self);

	pmf_device_deregister(self);

	return spdmem_common_detach(&sc->sc_base, self);
}

static int
spdmem_i2c_read(struct spdmem_softc *softc, uint16_t addr, uint8_t *val)
{
	uint8_t reg;
	struct spdmem_i2c_softc *sc = (struct spdmem_i2c_softc *)softc;
	static uint8_t dummy = 0;
	int rv;

	reg = addr & 0xff;

	rv = iic_acquire_bus(sc->sc_tag, 0);
	if (rv)
		return rv;

	if (addr & 0x100) {
		rv = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_page1,
		    &dummy, 1, &dummy, 1, 0);
		rv |= iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
		    &reg, 1, val, 1, 0);
		rv |= iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_page0, &dummy, 1, &dummy, 1, 0);
	} else {
		rv = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
		    &reg, 1, val, 1, 0);
	}

	iic_release_bus(sc->sc_tag, 0);

	return rv;
}

MODULE(MODULE_CLASS_DRIVER, spdmem, "i2cexec");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
spdmem_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;
#ifdef _MODULE
	static struct sysctllog *spdmem_sysctl_clog;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_spdmem,
		    cfattach_ioconf_spdmem, cfdata_ioconf_spdmem);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_spdmem,
		    cfattach_ioconf_spdmem, cfdata_ioconf_spdmem);
		sysctl_teardown(&spdmem_sysctl_clog);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
