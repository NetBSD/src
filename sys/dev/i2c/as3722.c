/* $NetBSD: as3722.c,v 1.4.2.3 2016/10/05 20:55:41 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: as3722.c,v 1.4.2.3 2016/10/05 20:55:41 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/as3722.h>

#define AS3722_GPIO0_CTRL_REG		0x08
#define AS3722_GPIO0_CTRL_INVERT	__BIT(7)
#define AS3722_GPIO0_CTRL_IOSF		__BITS(6,3)
#define AS3722_GPIO0_CTRL_IOSF_GPIO	0
#define AS3722_GPIO0_CTRL_IOSF_WATCHDOG	9
#define AS3722_GPIO0_CTRL_MODE		__BITS(2,0)
#define AS3722_GPIO0_CTRL_MODE_PULLDOWN	5

#define AS3722_RESET_CTRL_REG		0x36
#define AS3722_RESET_CTRL_POWER_OFF	__BIT(1)
#define AS3722_RESET_CTRL_FORCE_RESET	__BIT(0)

#define AS3722_WATCHDOG_CTRL_REG	0x38
#define AS3722_WATCHDOG_CTRL_MODE	__BITS(2,1)
#define AS3722_WATCHDOG_CTRL_ON		__BIT(0)

#define AS3722_WATCHDOG_TIMER_REG	0x46
#define AS3722_WATCHDOG_TIMER_TIMER	__BITS(6,0)

#define AS3722_WATCHDOG_SIGNAL_REG	0x48
#define AS3722_WATCHDOG_SIGNAL_PWM_DIV	__BITS(7,6)
#define AS3722_WATCHDOG_SIGNAL_SW_SIG	__BIT(0)

#define AS3722_ASIC_ID1_REG		0x90
#define AS3722_ASIC_ID2_REG		0x91

struct as3722_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct sysmon_wdog sc_smw;
};

#define AS3722_WATCHDOG_DEFAULT_PERIOD	10

static int	as3722_match(device_t, cfdata_t, void *);
static void	as3722_attach(device_t, device_t, void *);

static int	as3722_wdt_setmode(struct sysmon_wdog *);
static int	as3722_wdt_tickle(struct sysmon_wdog *);

static int	as3722_read(struct as3722_softc *, uint8_t, uint8_t *, int);
static int	as3722_write(struct as3722_softc *, uint8_t, uint8_t, int);
static int	as3722_set_clear(struct as3722_softc *, uint8_t, uint8_t,
				 uint8_t, int);

CFATTACH_DECL_NEW(as3722pmic, sizeof(struct as3722_softc),
    as3722_match, as3722_attach, NULL, NULL);

static const char * as3722_compats[] = {
	"ams,as3722",
	NULL
};

static int
as3722_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	uint8_t reg, id1;
	int error;

	if (ia->ia_name == NULL) {
		iic_acquire_bus(ia->ia_tag, I2C_F_POLL);
		reg = AS3722_ASIC_ID1_REG;
		error = iic_exec(ia->ia_tag, I2C_OP_READ_WITH_STOP, ia->ia_addr,
		    &reg, 1, &id1, 1, I2C_F_POLL);
		iic_release_bus(ia->ia_tag, I2C_F_POLL);

		if (error == 0 && id1 == 0x0c)
			return 1;

		return 0;
	} else {
		return iic_compat_match(ia, as3722_compats);
	}
}

static void
as3722_attach(device_t parent, device_t self, void *aux)
{
	struct as3722_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": AMS AS3722\n");

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	error = as3722_write(sc, AS3722_GPIO0_CTRL_REG,
	    __SHIFTIN(AS3722_GPIO0_CTRL_IOSF_GPIO,
		      AS3722_GPIO0_CTRL_IOSF) |
	    __SHIFTIN(AS3722_GPIO0_CTRL_MODE_PULLDOWN,
		      AS3722_GPIO0_CTRL_MODE),
	    I2C_F_POLL);
	error += as3722_set_clear(sc, AS3722_WATCHDOG_CTRL_REG,
	    __SHIFTIN(1, AS3722_WATCHDOG_CTRL_MODE), 0, I2C_F_POLL);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	if (error)
		aprint_error_dev(self, "couldn't setup watchdog\n");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = as3722_wdt_setmode;
	sc->sc_smw.smw_tickle = as3722_wdt_tickle;
	sc->sc_smw.smw_period = AS3722_WATCHDOG_DEFAULT_PERIOD;

	aprint_normal_dev(self, "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "couldn't register with sysmon\n");
}

static int
as3722_read(struct as3722_softc *sc, uint8_t reg, uint8_t *val, int flags)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, val, 1, flags);
}

static int
as3722_write(struct as3722_softc *sc, uint8_t reg, uint8_t val, int flags)
{
	uint8_t buf[2] = { reg, val };
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, 2, flags);
}

static int
as3722_set_clear(struct as3722_softc *sc, uint8_t reg, uint8_t set,
    uint8_t clr, int flags)
{
	uint8_t old, new;
	int error;

	error = as3722_read(sc, reg, &old, flags);
	if (error) {
		return error;
	}
	new = set | (old & ~clr);

	return as3722_write(sc, reg, new, flags);
}

static int
as3722_wdt_setmode(struct sysmon_wdog *smw)
{
	struct as3722_softc * const sc = smw->smw_cookie;
	int error;

	const int flags = (cold ? I2C_F_POLL : 0);

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		iic_acquire_bus(sc->sc_i2c, flags);
		error = as3722_set_clear(sc, AS3722_WATCHDOG_CTRL_REG,
		    0, AS3722_WATCHDOG_CTRL_ON, flags);
		iic_release_bus(sc->sc_i2c, flags);
		return error;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		smw->smw_period = AS3722_WATCHDOG_DEFAULT_PERIOD;
	}
	if (smw->smw_period < 1 || smw->smw_period > 128) {
		return EINVAL;
	}
	sc->sc_smw.smw_period = smw->smw_period;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_set_clear(sc, AS3722_WATCHDOG_TIMER_REG,
	    __SHIFTIN(sc->sc_smw.smw_period - 1, AS3722_WATCHDOG_TIMER_TIMER),
	    AS3722_WATCHDOG_TIMER_TIMER, flags);
	if (error == 0) {
		error = as3722_set_clear(sc, AS3722_WATCHDOG_CTRL_REG,
		    AS3722_WATCHDOG_CTRL_ON, 0, flags);
	}
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

static int
as3722_wdt_tickle(struct sysmon_wdog *smw)
{
	struct as3722_softc * const sc = smw->smw_cookie;
	int error;

	const int flags = (cold ? I2C_F_POLL : 0);

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_set_clear(sc, AS3722_WATCHDOG_SIGNAL_REG,
	    AS3722_WATCHDOG_SIGNAL_SW_SIG, 0, flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

int
as3722_poweroff(device_t dev)
{
	struct as3722_softc * const sc = device_private(dev);
	int error;

	const int flags = I2C_F_POLL;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_write(sc, AS3722_RESET_CTRL_REG,
	    AS3722_RESET_CTRL_POWER_OFF, flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}

int
as3722_reboot(device_t dev)
{
	struct as3722_softc * const sc = device_private(dev);
	int error;

	const int flags = I2C_F_POLL;

	iic_acquire_bus(sc->sc_i2c, flags);
	error = as3722_write(sc, AS3722_RESET_CTRL_REG,
	    AS3722_RESET_CTRL_FORCE_RESET, flags);
	iic_release_bus(sc->sc_i2c, flags);

	return error;
}
