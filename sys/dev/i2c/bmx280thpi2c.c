/*	$NetBSD: bmx280thpi2c.c,v 1.1 2022/12/03 01:04:43 brad Exp $	*/

/*
 * Copyright (c) 2022 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: bmx280thpi2c.c,v 1.1 2022/12/03 01:04:43 brad Exp $");

/*
 * I2C driver for the Bosch BMP280 / BME280 sensor.
 * Uses the common bmx280thp driver to do the real work.
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/pool.h>
#include <sys/kmem.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/spi/spivar.h>
#include <dev/ic/bmx280reg.h>
#include <dev/ic/bmx280var.h>

extern void	bmx280_attach(struct bmx280_sc *);

static int 	bmx280thpi2c_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	bmx280thpi2c_match(device_t, cfdata_t, void *);
static void 	bmx280thpi2c_attach(device_t, device_t, void *);
static int 	bmx280thpi2c_detach(device_t, int);

#define BMX280_DEBUG
#ifdef BMX280_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_bmx280debug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(bmx280thpi2c, sizeof(struct bmx280_sc),
    bmx280thpi2c_match, bmx280thpi2c_attach, bmx280thpi2c_detach, NULL);

/* For the BMX280, a read consists of writing on the I2C bus
 * a I2C START, I2C SLAVE address, then the starting register.
 * If that works, then following will be another I2C START,
 * I2C SLAVE address, followed by as many I2C reads that is
 * desired and then a I2C STOP
 */

static int
bmx280thpi2c_read_register_direct(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    uint8_t *buf, size_t blen)
{
	int error;

	error = iic_exec(tag,I2C_OP_WRITE,addr,&reg,1,NULL,0,0);

	if (error == 0) {
		error = iic_exec(tag,I2C_OP_READ_WITH_STOP,addr,NULL,0,
		    buf,blen,0);
	}

	return error;
}

static int
bmx280thpi2c_read_register(struct bmx280_sc *sc, uint8_t reg,
    uint8_t *buf, size_t blen)
{
	int error;

	KASSERT(blen > 0);

	error = bmx280thpi2c_read_register_direct(sc->sc_tag, sc->sc_addr, reg,
	    buf, blen);

	return error;
}

/* For the BMX280, a write consists of sending a I2C START, I2C SLAVE
 * address and then pairs of registers and data until a I2C STOP is
 * sent.
 */

static int
bmx280thpi2c_write_register(struct bmx280_sc *sc,
    uint8_t *buf, size_t blen)
{
	int error;

	KASSERT(blen > 0);
	/* XXX - there should be a KASSERT for blen at least
	   being an even number */

	error = iic_exec(sc->sc_tag,I2C_OP_WRITE_WITH_STOP,sc->sc_addr,NULL,0,
	    buf,blen,0);

	return error;
}

static int
bmx280thpi2c_acquire_bus(struct bmx280_sc *sc)
{
	return(iic_acquire_bus(sc->sc_tag, 0));
}

static void
bmx280thpi2c_release_bus(struct bmx280_sc *sc)
{
	iic_release_bus(sc->sc_tag, 0);
}

static int
bmx280thpi2c_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = BMX280_REGISTER_ID;
	uint8_t buf[1];
	int error;

	error = bmx280thpi2c_read_register_direct(tag, addr, reg, buf, 1);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
bmx280thpi2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	/* indirect config - check for configured address */
	if (ia->ia_addr != BMX280_TYPICAL_ADDR_1 &&
	    ia->ia_addr != BMX280_TYPICAL_ADDR_2)
		return 0;

	/*
	 * Check to see if something is really at this i2c address. This will
	 * keep phantom devices from appearing
	 */
	if (iic_acquire_bus(ia->ia_tag, 0) != 0) {
		if (matchdebug)
			printf("in match acquire bus failed\n");
		return 0;
	}

	error = bmx280thpi2c_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
bmx280thpi2c_attach(device_t parent, device_t self, void *aux)
{
	struct bmx280_sc *sc;
	struct i2c_attach_args *ia;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_bmx280debug = 0;
	sc->sc_func_acquire_bus = &bmx280thpi2c_acquire_bus;
	sc->sc_func_release_bus = &bmx280thpi2c_release_bus;
	sc->sc_func_read_register = &bmx280thpi2c_read_register;
	sc->sc_func_write_register = &bmx280thpi2c_write_register;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);

	bmx280_attach(sc);

	return;
}

static int
bmx280thpi2c_detach(device_t self, int flags)
{
	struct bmx280_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);

	/* Remove the sensors */
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	mutex_exit(&sc->sc_mutex);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_bmx280log);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, bmx280thpi2c, "iic,bmx280thp");

#ifdef _MODULE
/* Like other drivers, we do this because the bmx280 common
 * driver has the definitions already.
 */
#undef  CFDRIVER_DECL
#define CFDRIVER_DECL(name, class, attr)
#include "ioconf.c"
#endif

static int
bmx280thpi2c_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_bmx280thpi2c,
		    cfattach_ioconf_bmx280thpi2c, cfdata_ioconf_bmx280thpi2c);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_bmx280thpi2c,
		      cfattach_ioconf_bmx280thpi2c, cfdata_ioconf_bmx280thpi2c);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
