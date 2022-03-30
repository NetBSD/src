
/*	$NetBSD: scmdi2c.c,v 1.2 2022/03/30 00:06:50 pgoyette Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: scmdi2c.c,v 1.2 2022/03/30 00:06:50 pgoyette Exp $");

/*
 * I2C driver for the Sparkfun Serial motor controller.
 * Uses the common code to do the actual work.
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

#include <dev/i2c/i2cvar.h>
#include <dev/spi/spivar.h>
#include <dev/ic/scmdreg.h>
#include <dev/ic/scmdvar.h>

extern struct cdevsw scmd_cdevsw;

extern void	scmd_attach(struct scmd_sc *);

static int 	scmdi2c_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	scmdi2c_match(device_t, cfdata_t, void *);
static void 	scmdi2c_attach(device_t, device_t, void *);
static int 	scmdi2c_detach(device_t, int);
static int	scmdi2c_activate(device_t, enum devact);

#define SCMD_DEBUG
#ifdef SCMD_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_scmddebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(scmdi2c, sizeof(struct scmd_sc),
    scmdi2c_match, scmdi2c_attach, scmdi2c_detach, scmdi2c_activate);

static int
scmdi2c_read_reg_direct(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    uint8_t *buf)
{
	return iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, &reg, 1, buf,
	    1, 0);
}

static int
scmdi2c_read_reg(struct scmd_sc *sc, uint8_t reg, uint8_t *buf)
{
	return scmdi2c_read_reg_direct(sc->sc_tag, sc->sc_addr, reg, buf);
}

static int
scmdi2c_write_reg_direct(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    uint8_t buf)
{
	return iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &reg, 1, &buf,
	    1, 0);
}

static int
scmdi2c_write_reg(struct scmd_sc *sc, uint8_t reg, uint8_t buf)
{
	return scmdi2c_write_reg_direct(sc->sc_tag, sc->sc_addr, reg, buf);
}

static int
scmdi2c_acquire_bus(struct scmd_sc *sc)
{
	return(iic_acquire_bus(sc->sc_tag, 0));
}

static void
scmdi2c_release_bus(struct scmd_sc *sc)
{
	iic_release_bus(sc->sc_tag, 0);
}

static int
scmdi2c_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = SCMD_REG_ID;
	uint8_t buf;
	int error;

	error = scmdi2c_read_reg_direct(tag, addr, reg, &buf);
	if (matchdebug) {
		printf("poke X 1: %d %02x %d\n", addr, buf, error);
	}
	return error;
}

static int
scmdi2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	if (matchdebug) {
		printf("Looking at ia_addr: %x\n",ia->ia_addr);
	}

	/* indirect config - check for configured address */
	if (!(ia->ia_addr >= SCMD_LOW_I2C_ADDR &&
	    ia->ia_addr <= SCMD_HIGH_I2C_ADDR))
		return 0;

	/*
	 * Check to see if something is really at this i2c address.
	 * This will keep phantom devices from appearing
	 */
	if (iic_acquire_bus(ia->ia_tag, 0) != 0) {
		if (matchdebug)
			printf("in match acquire bus failed\n");
		return 0;
	}

	error = scmdi2c_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
scmdi2c_attach(device_t parent, device_t self, void *aux)
{
	struct scmd_sc *sc;
	struct i2c_attach_args *ia;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_scmddebug = 0;
	sc->sc_topaddr = 0xff;
	sc->sc_opened = false;
	sc->sc_dying = false;
	sc->sc_func_acquire_bus = &scmdi2c_acquire_bus;
	sc->sc_func_release_bus = &scmdi2c_release_bus;
	sc->sc_func_read_register = &scmdi2c_read_reg;
	sc->sc_func_write_register = &scmdi2c_write_reg;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_condmutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_dying_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condvar, "scmdi2ccv");
	cv_init(&sc->sc_cond_dying, "scmdi2cdc");

	scmd_attach(sc);

	return;
}

static int
scmdi2c_detach(device_t self, int flags)
{
	struct scmd_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);
	sc->sc_dying = true;
	/* If this is true we are still open, destroy the condvar */
	if (sc->sc_opened) {
		mutex_enter(&sc->sc_dying_mutex);
		DPRINTF(sc, 2, ("%s: Will wait for anything to exit\n",
		    device_xname(sc->sc_dev)));
		/* In the worst case this will time out after 5 seconds.
		 * It really should not take that long for the drain / whatever
		 * to happen
		 */
		cv_timedwait_sig(&sc->sc_cond_dying,
		    &sc->sc_dying_mutex, mstohz(5000));
		mutex_exit(&sc->sc_dying_mutex);
		cv_destroy(&sc->sc_cond_dying);
	}
	cv_destroy(&sc->sc_condvar);
	mutex_exit(&sc->sc_mutex);

	mutex_destroy(&sc->sc_mutex);
	mutex_destroy(&sc->sc_condmutex);

	return 0;
}

int
scmdi2c_activate(device_t self, enum devact act)
{
	struct scmd_sc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

MODULE(MODULE_CLASS_DRIVER, scmdi2c, "iic,scmd");

#ifdef _MODULE
/* Like other drivers, we do this because the scmd common
 * driver has the definitions already.
 */
#undef  CFDRIVER_DECL
#define CFDRIVER_DECL(name, class, attr)
#include "ioconf.c"
#endif

static int
scmdi2c_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	int error = 0;
	static struct cfdriver * const no_cfdriver_vec[] = { NULL };
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		/* I really do not understand why I had to do this this way.
		 * If I did not then the module would either not install when
		 * the SPI driver and hence the scmd dependent driver was compiled
		 * into the kernel, or it would not install if it was not.
		 * I suspect something is still not set up correctly somewhere, but
		 * I am at a lose to see what.
		 *
		 * The first config_init_component will fail if the SPI driver and the
		 * scmd dependent is in the kernel already, but the second one will work.
		 * Otherwise the other way around.
		 *
		 * This all manages to get this module set up in any situation.
		 */
		error = config_init_component(cfdriver_ioconf_scmdi2c,
		    cfattach_ioconf_scmdi2c, cfdata_ioconf_scmdi2c);
		if (error) {
			aprint_error("%s: Trying no_cfdriver_vec method: %d\n",
			    scmd_cd.cd_name, error);
			error = config_init_component(no_cfdriver_vec,
			    cfattach_ioconf_scmdi2c, cfdata_ioconf_scmdi2c);
		}

		return error;
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		/* See above.. same thing */
		error = config_fini_component(cfdriver_ioconf_scmdi2c,
		    cfattach_ioconf_scmdi2c, cfdata_ioconf_scmdi2c);
		if (error) {
			aprint_error("%s: Trying no_cfdriver_vec method: %d\n",
			    scmd_cd.cd_name, error);
			error = config_fini_component(no_cfdriver_vec,
			    cfattach_ioconf_scmdi2c, cfdata_ioconf_scmdi2c);
		}

		return error;
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
