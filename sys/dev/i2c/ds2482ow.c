/*	$NetBSD: ds2482ow.c,v 1.1 2024/11/04 20:43:38 brad Exp $	*/

/*
 * Copyright (c) 2024 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: ds2482ow.c,v 1.1 2024/11/04 20:43:38 brad Exp $");

/*
  Driver for the DS2482-100 and DS2482-800 I2C to Onewire bridge
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>

#include <dev/i2c/i2cvar.h>
#include <dev/onewire/onewirevar.h>
#include <dev/i2c/ds2482owreg.h>
#include <dev/i2c/ds2482owvar.h>

#define DS2482_ONEWIRE_SINGLE_BIT_READ	0xF7 /* Artifical */
#define DS2482_ONEWIRE_SINGLE_BIT_WRITE	0xF8 /* Artifical */

static int 	ds2482_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	ds2482_match(device_t, cfdata_t, void *);
static void 	ds2482_attach(device_t, device_t, void *);
static int 	ds2482_detach(device_t, int);
static int 	ds2482_verify_sysctl(SYSCTLFN_ARGS);

static int	ds2482_ow_reset(void *);
static int	ds2482_ow_read_bit(void *);
static void	ds2482_ow_write_bit(void *, int);
static int	ds2482_ow_read_byte(void *);
static void	ds2482_ow_write_byte(void *, int);
static int	ds2482_ow_triplet(void *, int);

#define DS2482_DEBUG

#ifdef DS2482_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_ds2482debug) \
	    aprint_normal x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

#ifdef DS2482_DEBUG
#define DPRINTF2(dl, l, x)			\
    do { \
	if (l <= dl) \
	    aprint_normal x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF2(dl, l, x)
#endif

CFATTACH_DECL_NEW(ds2482ow, sizeof(struct ds2482ow_sc),
    ds2482_match, ds2482_attach, ds2482_detach, NULL);


#define DS2482_QUICK_DELAY 18
#define DS2482_SLOW_DELAY 35

int
ds2482_verify_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

static int
ds2482_set_pullup(i2c_tag_t tag, i2c_addr_t addr, bool activepullup,
    bool strongpullup, int debuglevel)
{
	int error;
	uint8_t cmd = DS2482_WRITE_CONFIG;
	uint8_t pu = 0;
	uint8_t pux;

	if (activepullup == true)
		pu = pu | DS2482_CONFIG_APU;
	if (strongpullup == true)
		pu = pu | DS2482_CONFIG_SPU;

	/* The Write Config command wants the top bits of the config buffer to be
	 * the ones complement of the lower bits.
	 */

	pux = ~(pu << 4);
	pux = pux & 0xf0;
	pu = pu | pux;

	error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &cmd, 1, &pu, 1, 0);

	DPRINTF2(debuglevel, 4, ("ds2482_set_pullup: pu: %02x ; error: %x %d\n", pu, error, error));

	return error;
}

static int
ds2482_wait_with_status(i2c_tag_t tag, i2c_addr_t addr, uint8_t *status,
    unsigned int d, bool set_pointer, int debuglevel)
{
	int error = 0;
	uint8_t xcmd, xbuf;

	DPRINTF2(debuglevel, 5, ("ds2482_wait_with_status: start\n"));

	xcmd = DS2482_SET_READ_POINTER;
	xbuf = DS2482_REGISTER_STATUS;
	if (set_pointer == true)
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
	if (! error) {
		error = iic_exec(tag, I2C_OP_READ, addr, NULL, 0, status, 1, 0);
		if ((*status & DS2482_STATUS_1WB) && (! error)) {
			do {
				delay(d);
				error = iic_exec(tag, I2C_OP_READ, addr, NULL, 0, status, 1, 0);
			} while ((*status & DS2482_STATUS_1WB) &&
			    (! error));
		}
	}

	DPRINTF2(debuglevel, 5, ("ds2482_wait_with_status: end ; status: %02x %d ; error: %x %d\n", *status, *status, error, error));

	return error;
}

static int
ds2482_cmd(i2c_tag_t tag, i2c_addr_t addr, uint8_t *cmd,
    uint8_t *cmdarg, uint8_t *obuf, size_t obuflen, bool activepullup,
    bool strongpullup, int debuglevel)
{
	int error;
	uint8_t xcmd;
	uint8_t xbuf;

	switch (*cmd) {
		/* The datasheet says that none of these are effected by what sort of pullup
		 * is set and only the Write Config command needs to happen when idle.
		 */
	case DS2482_SET_READ_POINTER:
	case DS2482_WRITE_CONFIG:
	case DS2482_SELECT_CHANNEL:
		KASSERT(cmdarg != NULL);

		error = 0;

		if (*cmd == DS2482_WRITE_CONFIG)
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, true, debuglevel);

		if (! error)
			error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, cmd, 1, cmdarg, 1, 0);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: cmd: %02x ; error: %x %d\n", *cmd, error, error));

		break;


	case DS2482_DEVICE_RESET:
	case DS2482_ONEWIRE_RESET:
		/* Device reset resets everything, including pullup
		 * configuration settings, but that doesn't matter as we will
		 * always set the config before doing anything that actions on
		 * the 1-Wire bus.
		 *
		 * The data sheet warns about using the strong pull up feature
		 * with a 1-Wire reset, so we will simply not allow that
		 * combination.
		 *
		 * The data sheet does not mention if the 1-Wire reset effects
		 * just a single channel all channels.  It seems likely that it
		 * is the currently active channel, and the driver works on that
		 * assumption.
		*/

		error = 0;
		if (*cmd == DS2482_ONEWIRE_RESET) {
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, true, debuglevel);
			if (! error)
				error = ds2482_set_pullup(tag, addr, activepullup, false, debuglevel);
		}

		if (! error)
			error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, cmd, 1, NULL, 0, 0);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: cmd: %02x ; error: %x %d\n", *cmd, error, error));

		if (*cmd == DS2482_DEVICE_RESET)
			delay(1);
		if (*cmd == DS2482_ONEWIRE_RESET)
			delay(1300);

		break;

	case DS2482_ONEWIRE_SINGLE_BIT_WRITE:
		KASSERT(cmdarg != NULL);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: DS2482_ONEWIRE_SINGLE_BIT_WRITE: cmdarg: %02x %d\n", *cmdarg, *cmdarg));

		xcmd = DS2482_SET_READ_POINTER;
		xbuf = DS2482_REGISTER_STATUS;
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
		if (! error)
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, false, debuglevel);

		if (! error) {
			xcmd = DS2482_ONEWIRE_SINGLE_BIT;
			xbuf = DS2482_ONEWIRE_BIT_ZERO;
			if (*cmdarg & 0x01)
				xbuf = DS2482_ONEWIRE_BIT_ONE;
			error = ds2482_set_pullup(tag, addr, activepullup, strongpullup, debuglevel);
			if (! error)
				error = iic_exec(tag, I2C_OP_WRITE, addr, &xcmd, 1, &xbuf, 1, 0);
			if (! error) {
				xbuf = 0xff;
				error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_SLOW_DELAY, false, debuglevel);
			}
		}
		break;

	case DS2482_ONEWIRE_SINGLE_BIT_READ:
		KASSERT(obuf != NULL);
		KASSERT(obuflen == 1);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: DS2482_ONEWIRE_SINGLE_BIT_READ\n"));

		xcmd = DS2482_SET_READ_POINTER;
		xbuf = DS2482_REGISTER_STATUS;
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
		if (! error)
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, false, debuglevel);

		if (! error) {
			xcmd = DS2482_ONEWIRE_SINGLE_BIT;
			xbuf = DS2482_ONEWIRE_BIT_ONE;
			error = ds2482_set_pullup(tag, addr, activepullup, strongpullup, debuglevel);
			if (! error)
				error = iic_exec(tag, I2C_OP_WRITE, addr, &xcmd, 1, &xbuf, 1, 0);
			if (! error) {
				xbuf = 0xff;
				error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_SLOW_DELAY, false, debuglevel);
				if (! error) {
					*obuf = (xbuf & DS2482_STATUS_SBR) >> DS2482_STATUS_SBR_SHIFT;
				}
			}
		}
		break;

	case DS2482_ONEWIRE_WRITE_BYTE:
		KASSERT(cmdarg != NULL);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: DS2482_ONEWIRE_WRITE_BYTE: cmdarg: %02x %d\n", *cmdarg, *cmdarg));

		xcmd = DS2482_SET_READ_POINTER;
		xbuf = DS2482_REGISTER_STATUS;
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
		if (! error)
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, false, debuglevel);

		if (! error) {
			error = ds2482_set_pullup(tag, addr, activepullup, strongpullup, debuglevel);
			if (! error)
				error = iic_exec(tag, I2C_OP_WRITE, addr, cmd, 1, cmdarg, 1, 0);
			if (! error) {
				xbuf = 0xff;
				error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_SLOW_DELAY, false, debuglevel);
			}
		}
		break;

	case DS2482_ONEWIRE_READ_BYTE:
		KASSERT(obuf != NULL);
		KASSERT(obuflen == 1);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: DS2482_ONEWIRE_READ_BYTE\n"));

		xcmd = DS2482_SET_READ_POINTER;
		xbuf = DS2482_REGISTER_STATUS;
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
		if (! error)
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, false, debuglevel);

		if (! error) {
			error = ds2482_set_pullup(tag, addr, activepullup, strongpullup, debuglevel);
			if (! error)
				error = iic_exec(tag, I2C_OP_WRITE, addr, cmd, 1, NULL, 0, 0);
			if (! error) {
				xbuf = 0xff;
				error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_SLOW_DELAY, false, debuglevel);
				if (! error) {
					xcmd = DS2482_SET_READ_POINTER;
					xbuf = DS2482_REGISTER_DATA;
					error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
					if (! error) {
						xbuf = 0xff;
						error = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, NULL, 0, &xbuf, 1, 0);
						if (! error) {
							*obuf = xbuf;
						}
					}
				}
			}
		}
		break;

	case DS2482_ONEWIRE_TRIPLET:
		KASSERT(cmdarg != NULL);
		KASSERT(obuf != NULL);
		KASSERT(obuflen == 1);

		DPRINTF2(debuglevel, 4, ("ds2482_cmd: DS2482_ONEWIRE_TRIPLET: cmdarg: %02x %d\n", *cmdarg, *cmdarg));

		xcmd = DS2482_SET_READ_POINTER;
		xbuf = DS2482_REGISTER_STATUS;
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xcmd, 1, &xbuf, 1, 0);
		if (! error)
			error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_QUICK_DELAY, false, debuglevel);

		if (! error) {
			xbuf = DS2482_TRIPLET_DIR_ZERO;
			if (*cmdarg & 0x01) {
				xbuf = DS2482_TRIPLET_DIR_ONE;
			}
			error = ds2482_set_pullup(tag, addr, activepullup, strongpullup, debuglevel);
			if (! error)
				error = iic_exec(tag, I2C_OP_WRITE, addr, cmd, 1, &xbuf, 1, 0);
			if (! error) {
				xbuf = 0xff;
				error = ds2482_wait_with_status(tag, addr, &xbuf, DS2482_SLOW_DELAY, false, debuglevel);
				if (! error) {
					/* This is undocumented anywhere I could find, but what has to be returned is
					 * 0x01 is the triplet path was taken, 0x02 is the Not-triplet path was taken,
					 * and 0x00 is neither was taken.  The DIR bit in the status of the DS2482 may
					 * help with this some, but what is below seems to work.
					 */
					*obuf = 0;
					if (xbuf & DS2482_STATUS_TSB) {
						*obuf = 0x01;
					} else {
						if (xbuf & DS2482_STATUS_SBR) {
							*obuf = 0x02;
						}
					}
				}
			}
		}

		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
ds2482_cmdr(struct ds2482ow_sc *sc, uint8_t cmd, uint8_t cmdarg, uint8_t *buf, size_t blen)
{
	DPRINTF(sc, 3, ("%s: ds2482_cmdr: cmd: %02x\n",
	    device_xname(sc->sc_dev), cmd));
	return ds2482_cmd(sc->sc_tag, sc->sc_addr, &cmd, &cmdarg, buf, blen, sc->sc_activepullup, sc->sc_strongpullup, sc->sc_ds2482debug);
}

static const uint8_t ds2482_channels[] = {
	DS2482_CHANNEL_IO0,
	DS2482_CHANNEL_IO1,
	DS2482_CHANNEL_IO2,
	DS2482_CHANNEL_IO3,
	DS2482_CHANNEL_IO4,
	DS2482_CHANNEL_IO5,
	DS2482_CHANNEL_IO6,
	DS2482_CHANNEL_IO7
};

static int
ds2482_set_channel(struct ds2482ow_sc *sc, int channel)
{
	int error = 0;

	KASSERT(channel >= 0 && channel < DS2482_NUM_INSTANCES);

	if (sc->sc_is_800 == true)
		error = ds2482_cmdr(sc, DS2482_SELECT_CHANNEL, ds2482_channels[channel], NULL, 0);

	return error;
}

static int
ds2482_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = DS2482_SET_READ_POINTER;
	uint8_t rbuf = DS2482_REGISTER_STATUS;
	uint8_t obuf;
	int error;

	error = ds2482_cmd(tag, addr, &reg, &rbuf, &obuf, 1, false, false, 0);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
ds2482_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	/* indirect config - check for configured address */
	if (!(ia->ia_addr >= DS2482_LOWEST_ADDR &&
	    ia->ia_addr <= DS2482_HIGHEST_ADDR))
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

	error = ds2482_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
ds2482_attach(device_t parent, device_t self, void *aux)
{
	struct ds2482ow_sc *sc;
	struct i2c_attach_args *ia;
	int error, i, num_channels = 1;
	struct onewirebus_attach_args oba;
	const struct sysctlnode *cnode;
	int sysctlroot_num, pullup_num;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_ds2482debug = 0;
	sc->sc_activepullup = false;
	sc->sc_strongpullup = false;
	sc->sc_is_800 = false;

	aprint_normal("\n");

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);

	if ((error = sysctl_createv(&sc->sc_ds2482log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("DS2482 controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto out;

	sysctlroot_num = cnode->sysctl_num;

#ifdef DS2482_DEBUG
	if ((error = sysctl_createv(&sc->sc_ds2482log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), ds2482_verify_sysctl, 0,
	    &sc->sc_ds2482debug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto out;
#endif

	if ((error = sysctl_createv(&sc->sc_ds2482log, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, "pullup",
	    SYSCTL_DESCR("Pullup controls"), NULL, 0, NULL, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

	pullup_num = cnode->sysctl_num;

	if ((error = sysctl_createv(&sc->sc_ds2482log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "active",
	    SYSCTL_DESCR("Active pullup"), NULL, 0, &sc->sc_activepullup,
	    0, CTL_HW, sysctlroot_num, pullup_num, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

	if ((error = sysctl_createv(&sc->sc_ds2482log, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "strong",
	    SYSCTL_DESCR("Strong pullup"), NULL, 0, &sc->sc_strongpullup,
	    0, CTL_HW, sysctlroot_num, pullup_num, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	error = ds2482_cmdr(sc, DS2482_DEVICE_RESET, 0, NULL, 0);
	if (error != 0)
		aprint_error_dev(self, "Reset failed: %d\n", error);

	if (! error) {
		int xerror;
		xerror = ds2482_cmdr(sc, DS2482_SELECT_CHANNEL, DS2482_CHANNEL_IO0, NULL, 0);
		if (! xerror)
			sc->sc_is_800 = true;
	}

	iic_release_bus(sc->sc_tag, 0);

	if (error != 0) {
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	if (sc->sc_is_800 == true) {
		num_channels = DS2482_NUM_INSTANCES;
	}

	aprint_normal_dev(self, "Maxim DS2482-%s I2C to 1-Wire bridge, Channels available: %d\n",
	    (sc->sc_is_800 == true) ? "800" : "100",
	    num_channels);

	for(i = 0;i < num_channels;i++) {
		sc->sc_instances[i].sc_i_channel = i;
		sc->sc_instances[i].sc = sc;
		sc->sc_instances[i].sc_i_ow_bus.bus_cookie = &sc->sc_instances[i];
		sc->sc_instances[i].sc_i_ow_bus.bus_reset = ds2482_ow_reset;
		sc->sc_instances[i].sc_i_ow_bus.bus_read_bit = ds2482_ow_read_bit;
		sc->sc_instances[i].sc_i_ow_bus.bus_write_bit = ds2482_ow_write_bit;
		sc->sc_instances[i].sc_i_ow_bus.bus_read_byte = ds2482_ow_read_byte;
		sc->sc_instances[i].sc_i_ow_bus.bus_write_byte = ds2482_ow_write_byte;
		sc->sc_instances[i].sc_i_ow_bus.bus_triplet = ds2482_ow_triplet;

		memset(&oba, 0, sizeof(oba));
		oba.oba_bus = &sc->sc_instances[i].sc_i_ow_bus;
		sc->sc_instances[i].sc_i_ow_dev = config_found(self, &oba, onewirebus_print, CFARGS_NONE);
	}

out:
	return;
}

/* Hmmm...  except in the case of reset, there really doesn't seem to be any
 * way with the onewire(4) API to indicate an error condition.
*/

static int
ds2482_generic_action(struct ds2482_instance *sci, uint8_t cmd, uint8_t cmdarg, uint8_t *buf, size_t blen)
{
	struct ds2482ow_sc *sc = sci->sc;
	int rv;

	mutex_enter(&sc->sc_mutex);
	rv = iic_acquire_bus(sc->sc_tag, 0);
	if (!rv) {
		rv = ds2482_set_channel(sc, sci->sc_i_channel);
		if (!rv)
			rv = ds2482_cmdr(sc, cmd, cmdarg, buf, blen);
	}
	iic_release_bus(sc->sc_tag, 0);
	mutex_exit(&sc->sc_mutex);

	return rv;
}

static int
ds2482_ow_reset(void *arg)
{
	struct ds2482_instance *sci = arg;
	struct ds2482ow_sc *sc = sci->sc;
	int rv;

	rv = ds2482_generic_action(sci, DS2482_ONEWIRE_RESET, 0, NULL, 0);

	DPRINTF(sc, 3, ("%s: ds2482_ow_reset: channel: %d ; rv: %x %d\n",
	    device_xname(sc->sc_dev), sci->sc_i_channel, rv, rv));

	return rv;
}

static int
ds2482_ow_read_bit(void *arg)
{
	struct ds2482_instance *sci = arg;
	struct ds2482ow_sc *sc = sci->sc;
	int rv;
	uint8_t buf = 0x55;

	rv = ds2482_generic_action(sci, DS2482_ONEWIRE_SINGLE_BIT_READ, 0, &buf, 1);

	DPRINTF(sc, 3, ("%s: ds2482_read_bit: channel: %d ; rv: %x %d ; buf: %02x %d\n",
	    device_xname(sc->sc_dev), sci->sc_i_channel, rv, rv, buf, buf));

	return (int)buf;
}

static void
ds2482_ow_write_bit(void *arg, int value)
{
	struct ds2482_instance *sci = arg;
	struct ds2482ow_sc *sc = sci->sc;
	int rv;

	rv = ds2482_generic_action(sci, DS2482_ONEWIRE_SINGLE_BIT_WRITE, (uint8_t)value, NULL, 0);

	DPRINTF(sc, 3, ("%s: ds2482_write_bit: channel: %d ; rv: %x %d ; value: %02x %d\n",
	    device_xname(sc->sc_dev), sci->sc_i_channel, rv, rv, (uint8_t)value, (uint8_t)value));

	return;
}

static int
ds2482_ow_read_byte(void *arg)
{
	struct ds2482_instance *sci = arg;
	uint8_t buf = 0x55;
	struct ds2482ow_sc *sc = sci->sc;
	int rv;

	rv = ds2482_generic_action(sci, DS2482_ONEWIRE_READ_BYTE, 0, &buf, 1);

	DPRINTF(sc, 3, ("%s: ds2482_read_byte: channel: %d ; rv: %x %d ; buf: %02x %d\n",
	    device_xname(sc->sc_dev), sci->sc_i_channel, rv, rv, buf, buf));

	return (int)buf;
}

static void
ds2482_ow_write_byte(void *arg, int value)
{
	struct ds2482_instance *sci = arg;
	struct ds2482ow_sc *sc = sci->sc;
	int rv;

	rv = ds2482_generic_action(sci, DS2482_ONEWIRE_WRITE_BYTE, (uint8_t)value, NULL, 0);

	DPRINTF(sc, 3, ("%s: ds2482_write_byte: channel: %d ; rv: %x %d ; value: %02x %d\n",
	    device_xname(sc->sc_dev), sci->sc_i_channel, rv, rv, (uint8_t)value, (uint8_t)value));

	return;
}

static int
ds2482_ow_triplet(void *arg, int dir)
{
	struct ds2482_instance *sci = arg;
	uint8_t buf = 0x55;
	struct ds2482ow_sc *sc = sci->sc;
	int rv;

	rv = ds2482_generic_action(sci, DS2482_ONEWIRE_TRIPLET, (uint8_t) dir, &buf, 1);

	DPRINTF(sc, 3, ("%s: ds2482_triplet: channel: %d ; rv: %x %d ; dir: %x %d ; buf: %02x %d\n",
	    device_xname(sc->sc_dev), sci->sc_i_channel, rv, rv, dir, dir, (uint8_t)buf, (uint8_t)buf));

	return (int)buf;
}

static int
ds2482_detach(device_t self, int flags)
{
	struct ds2482ow_sc *sc;

	sc = device_private(self);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_ds2482log);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, ds2482ow, "iic,onewire");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
ds2482ow_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_ds2482ow,
		    cfattach_ioconf_ds2482ow, cfdata_ioconf_ds2482ow);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_ds2482ow,
		      cfattach_ioconf_ds2482ow, cfdata_ioconf_ds2482ow);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
