
/*	$NetBSD: scmd.c,v 1.2 2022/03/31 19:30:16 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: scmd.c,v 1.2 2022/03/31 19:30:16 pgoyette Exp $");

/*
 * Common driver for the Sparkfun Serial motor controller.
 * Calls out to specific frontends to move bits.
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>
#include <dev/spi/spivar.h>
#include <dev/ic/scmdreg.h>
#include <dev/ic/scmdvar.h>

void		scmd_attach(struct scmd_sc *);
static void	scmd_wait_restart(struct scmd_sc *, bool);
static int	scmd_get_topaddr(struct scmd_sc *);
static int 	scmd_verify_sysctl(SYSCTLFN_ARGS);
static int	scmd_local_read(struct scmd_sc *, uint8_t, uint8_t *);
static int	scmd_remote_read(struct scmd_sc *, int, uint8_t *);
static int	scmd_local_write(struct scmd_sc *, uint8_t, uint8_t);
static int	scmd_remote_write(struct scmd_sc *, int, uint8_t);

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

extern struct cfdriver scmd_cd;

static dev_type_open(scmd_open);
static dev_type_read(scmd_read);
static dev_type_write(scmd_write);
static dev_type_close(scmd_close);
const struct cdevsw scmd_cdevsw = {
	.d_open = scmd_open,
	.d_close = scmd_close,
	.d_read = scmd_read,
	.d_write = scmd_write,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int
scmd_verify_sysctl(SYSCTLFN_ARGS)
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
scmd_sysctl_init(struct scmd_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_scmdlog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("scmd controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef SCMD_DEBUG
	if ((error = sysctl_createv(&sc->sc_scmdlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), scmd_verify_sysctl, 0,
	    &sc->sc_scmddebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

	return 0;
}

/* Restarts and re-enumeration of the device is a little strange.
 * It will take a very long time to complete.  It would be more polite
 * to use a condvar for this wait, but it was noticed that those may
 * not work if done too early in boot and will just hang the boot, so
 * delay is also offered as an option.
 */
static void
scmd_wait_restart(struct scmd_sc *sc, bool usedelay)
{
	int error;
	uint8_t buf = SCMD_HOLE_VALUE;
	int c = 0;

	do {
		if (usedelay) {
			delay(1000000);
		} else {
			mutex_enter(&sc->sc_condmutex);
			cv_timedwait(&sc->sc_condvar, &sc->sc_condmutex,
			    mstohz(1000));
			mutex_exit(&sc->sc_condmutex);
		}

		error = (*(sc->sc_func_read_register))(sc, SCMD_REG_STATUS_1, &buf);

		DPRINTF(sc, 2, ("%s: Read back status after restart: %02x %d\n",
		    device_xname(sc->sc_dev), buf, error));

		c++;
	} while (c <= 20 && buf != 0x00);
}

static int
scmd_get_topaddr(struct scmd_sc *sc)
{
	uint8_t topaddr;
	int error;

	error = (*(sc->sc_func_read_register))(sc, SCMD_REG_SLV_TOP_ADDR, &topaddr);

	if (error) {
		topaddr = 0;
	}
	return topaddr;
}

/* Note that this assumes that you can actually access the device.
 * In at least one case right now, SPI on a Raspberry PI 3, the pins
 * have not been set up to allow SPI to function, but nothing is
 * returned as an error either.  We do the best that can be done right
 * now.
 */
void
scmd_attach(struct scmd_sc *sc)
{
	int error;

	aprint_normal("\n");

	if ((error = scmd_sysctl_init(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	error = (*(sc->sc_func_acquire_bus))(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	error = (*(sc->sc_func_write_register))(sc, SCMD_REG_CONTROL_1, SCMD_CONTROL_1_RESTART);
	if (error != 0)
		aprint_error_dev(sc->sc_dev, "Reset failed: %d\n", error);

	scmd_wait_restart(sc, true);

	sc->sc_topaddr = scmd_get_topaddr(sc);

	DPRINTF(sc, 2, ("%s: Top remote module address: %02x\n",
	    device_xname(sc->sc_dev), sc->sc_topaddr));

	uint8_t fwversion;
	uint8_t id;
	uint8_t pins;

	error = (*(sc->sc_func_read_register))(sc, SCMD_REG_FID, &fwversion);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Read of FID failed: %d\n",
		    error);
		goto out;
	}

	error = (*(sc->sc_func_read_register))(sc, SCMD_REG_ID, &id);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Read of ID failed: %d\n",
		    error);
		goto out;
	}

	error = (*(sc->sc_func_read_register))(sc, SCMD_REG_CONFIG_BITS, &pins);
	if (error) {
		aprint_error_dev(sc->sc_dev, "Read of CONFIG_BITS failed: %d\n",
		    error);
		goto out;
	}

	aprint_normal_dev(sc->sc_dev, "Sparkfun Serial motor controller, "
	    "Firmware version: %02x, ID: %02x%s, Jumper pins: %02x\n",
	    fwversion, id, (id == SCMD_EXPECTED_ID) ? " (expected ID)" : " (unexpected ID)",
	    pins);

 out:
	(*(sc->sc_func_release_bus))(sc);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "Unable to setup device\n");
	}

	return;
}

/* This device has the effect of creating a virtual register space of all
 * of the attached modules.  All you have to do is read and write to anything
 * in that space and you can hit the main module and all chained slave modules
 * without having to worry about the view port set up.
 *
 * 0x00 - 0x7E -- the first and main module
 * 0x7F - 0xFD -- the first slaved module
 * ...etc...
 *
 */
static int
scmd_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct scmd_sc *sc;

	sc = device_lookup_private(&scmd_cd, minor(dev));
	if (!sc)
		return ENXIO;

	if (sc->sc_opened)
		return EBUSY;

	/* This is a meaningless assigment to keep GCC from
	 * complaining.
	 */
	sc->sc_func_attach = &scmd_attach;

	mutex_enter(&sc->sc_mutex);
	sc->sc_opened = true;
	mutex_exit(&sc->sc_mutex);

	return 0;
}

static int
scmd_maxregister(int topaddr)
{
	if (topaddr >= SCMD_REMOTE_ADDR_LOW &&
	    topaddr <= SCMD_REMOTE_ADDR_HIGH) {
		int i = (topaddr - SCMD_REMOTE_ADDR_LOW) + 2;
		return (SCMD_REG_SIZE * i) - 1;
	} else {
		return SCMD_LAST_REG;
	}
}

/* Please note that that setting up and using the view port
 * to get access to SCMD devices that are chained off of the main
 * device is not atomic.  Hopefully this all happens fast enough
 * so that nothing can sneak in and mess with the registers.
 */
static int
scmd_set_view_port(struct scmd_sc *sc, int reg)
{
	int err;
	int loc = reg / SCMD_REG_SIZE;
	uint8_t vpi2creg = reg % SCMD_REG_SIZE;
	uint8_t vpi2caddr = (SCMD_REMOTE_ADDR_LOW + loc) - 1;

	DPRINTF(sc, 2, ("%s: View port addr: %02x ; View port register: %02x ; Orig register: %04x\n",
	    device_xname(sc->sc_dev), vpi2caddr, vpi2creg, reg));

	err = (*(sc->sc_func_write_register))(sc, SCMD_REG_REM_ADDR, vpi2caddr);
	if (! err)
		err = (*(sc->sc_func_write_register))(sc, SCMD_REG_REM_OFFSET, vpi2creg);

	return err;
}

/* It is not defined what happens if the Not Defined in the datasheet
 * registers are accessed, so block them.
 */
static int
scmd_local_read(struct scmd_sc *sc, uint8_t reg, uint8_t *buf)
{
	if (SCMD_IS_HOLE(reg)) {
		*buf = SCMD_HOLE_VALUE;
		return 0;
	}

	return (*(sc->sc_func_read_register))(sc, reg, buf);
}

static int
scmd_remote_read(struct scmd_sc *sc, int reg, uint8_t *buf)
{
	int err;
	int c;
	uint8_t b;

	if (SCMD_IS_HOLE(reg % SCMD_REG_SIZE)) {
		*buf = SCMD_HOLE_VALUE;
		return 0;
	}

	err = scmd_set_view_port(sc, reg);
	if (! err) {
		b = 0xff; /* you can write anything here.. it doesn't matter */
		err = (*(sc->sc_func_write_register))(sc, SCMD_REG_REM_READ, b);
		if (! err) {
			/* So ...  there is no way to really know that the data is ready and
			 * there is no way to know if there was an error in the master module reading
			 * the data from the slave module.  The data sheet says wait 5ms.. so do that
			 * and see if the register cleared, but don't wait forever...  I can't see how
			 * it would not be possible to read junk at times.
			 */
			c = 0;
			do {
				delay(5000);
				err = (*(sc->sc_func_read_register))(sc, SCMD_REG_REM_READ, &b);
				c++;
			} while ((c < 10) && (b != 0x00) && (!err));
			/* We can only hope that whatever was read from the slave module is there */
			if (! err)
				err = (*(sc->sc_func_read_register))(sc, SCMD_REG_REM_DATA_RD, buf);
		}
	}

	return err;
}

static int
scmd_read(dev_t dev, struct uio *uio, int flags)
{
	struct scmd_sc *sc;
	int error;

	if ((sc = device_lookup_private(&scmd_cd, minor(dev))) == NULL)
		return ENXIO;

	/* We do not make this an error.  There is nothing wrong with running
	 * off the end here, just return EOF.
	 */
	if (uio->uio_offset > scmd_maxregister(sc->sc_topaddr))
		return 0;

	if ((error = (*(sc->sc_func_acquire_bus))(sc)) != 0)
		return error;

	while (uio->uio_resid &&
	    uio->uio_offset <= scmd_maxregister(sc->sc_topaddr) &&
	    !sc->sc_dying) {
		uint8_t buf;
		int reg_addr = uio->uio_offset;

		if (reg_addr <= SCMD_LAST_REG) {
			if ((error = scmd_local_read(sc, (uint8_t)reg_addr, &buf)) != 0) {
				(*(sc->sc_func_release_bus))(sc);
				aprint_error_dev(sc->sc_dev,
				    "%s: local read failed at 0x%02x: %d\n",
				    __func__, reg_addr, error);
				return error;
			}
		} else {
			if ((error = scmd_remote_read(sc, reg_addr, &buf)) != 0) {
				(*(sc->sc_func_release_bus))(sc);
				aprint_error_dev(sc->sc_dev,
				    "%s: remote read failed at 0x%02x: %d\n",
				    __func__, reg_addr, error);
				return error;
			}
		}

		if (sc->sc_dying)
			break;

		if ((error = uiomove(&buf, 1, uio)) != 0) {
			(*(sc->sc_func_release_bus))(sc);
			return error;
		}
	}

	(*(sc->sc_func_release_bus))(sc);

	if (sc->sc_dying) {
		return EIO;
	}

	return 0;
}

/* Same thing about the undefined registers.  Don't actually allow
 * writes as it is not clear what happens when you do that.
 */
static int
scmd_local_write(struct scmd_sc *sc, uint8_t reg, uint8_t buf)
{
	if (SCMD_IS_HOLE(reg))
		return 0;

	return (*(sc->sc_func_write_register))(sc, reg, buf);
}

static int
scmd_remote_write(struct scmd_sc *sc, int reg, uint8_t buf)
{
	int err;
	int c;
	uint8_t b;

	if (SCMD_IS_HOLE(reg % SCMD_REG_SIZE)) {
		return 0;
	}

	err = scmd_set_view_port(sc, reg);
	if (! err) {
		/* We just sort of send this write off and wait to see if the register
		 * clears.  There really isn't any indication that the data made it to the
		 * slave modules and there really are not any errors reported.
		 */
		err = (*(sc->sc_func_write_register))(sc, SCMD_REG_REM_DATA_WR, buf);
		if (! err) {
			b = 0xff; /* you can write anything here.. it doesn't matter */
			err = (*(sc->sc_func_write_register))(sc, SCMD_REG_REM_WRITE, b);
			if (! err) {
				c = 0;
				do {
					delay(5000);
					err = (*(sc->sc_func_read_register))(sc, SCMD_REG_REM_WRITE, &b);
					c++;
				} while ((c < 10) && (b != 0x00) && (!err));
			}
		}
	}

	return err;
}

static int
scmd_write(dev_t dev, struct uio *uio, int flags)
{
	struct scmd_sc *sc;
	int error;

	if ((sc = device_lookup_private(&scmd_cd, minor(dev))) == NULL)
		return ENXIO;

	/* Same thing as read, this is not considered an error */
	if (uio->uio_offset > scmd_maxregister(sc->sc_topaddr))
		return 0;

	if ((error = (*(sc->sc_func_acquire_bus))(sc)) != 0)
		return error;

	while (uio->uio_resid &&
	    uio->uio_offset <= scmd_maxregister(sc->sc_topaddr) &&
	    !sc->sc_dying) {
		uint8_t buf;
		int reg_addr = uio->uio_offset;

		if ((error = uiomove(&buf, 1, uio)) != 0)
			break;

		if (sc->sc_dying)
			break;

		if (reg_addr <= SCMD_LAST_REG) {
			if ((error = scmd_local_write(sc, (uint8_t)reg_addr, buf)) != 0) {
				(*(sc->sc_func_release_bus))(sc);
				aprint_error_dev(sc->sc_dev,
				    "%s: local write failed at 0x%02x: %d\n",
				    __func__, reg_addr, error);
				return error;
			}

			/* If this was a local command to the control register that
			 * can perform re-enumeration, then do the wait thing.
			 * It is not as important that this be done for remote module
			 * access as the only thing that you could really do there is
			 * a restart and not re-emumeration, which is really what the wait
			 * is all about.
			 */
			if (reg_addr == SCMD_REG_CONTROL_1) {
				scmd_wait_restart(sc, false);

				sc->sc_topaddr = scmd_get_topaddr(sc);
				aprint_normal_dev(sc->sc_dev, "Highest I2C address on expansion bus is: %02x\n",
				    sc->sc_topaddr);
			}
		} else {
			if ((error = scmd_remote_write(sc, reg_addr, buf)) != 0) {
				(*(sc->sc_func_release_bus))(sc);
				aprint_error_dev(sc->sc_dev,
				    "%s: remote write failed at 0x%02x: %d\n",
				    __func__, reg_addr, error);
				return error;
			}
		}
	}

	(*(sc->sc_func_release_bus))(sc);

	if (sc->sc_dying) {
		return EIO;
	}

	return error;
}

static int
scmd_close(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct scmd_sc *sc;

	sc = device_lookup_private(&scmd_cd, minor(dev));

	if (sc->sc_dying) {
		DPRINTF(sc, 2, ("%s: Telling all we are almost dead\n",
		    device_xname(sc->sc_dev)));
		mutex_enter(&sc->sc_dying_mutex);
		cv_signal(&sc->sc_cond_dying);
		mutex_exit(&sc->sc_dying_mutex);
		return EIO;
	}

	mutex_enter(&sc->sc_mutex);
	sc->sc_opened = false;
	mutex_exit(&sc->sc_mutex);

	return(0);
}

MODULE(MODULE_CLASS_DRIVER, scmd, NULL);

#ifdef _MODULE
CFDRIVER_DECL(scmd, DV_DULL, NULL);
#include "ioconf.c"
#endif

static int
scmd_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	int error = 0;
	int bmaj = -1, cmaj = -1;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = devsw_attach("scmd", NULL, &bmaj,
		    &scmd_cdevsw, &cmaj);
		if (error) {
			aprint_error("%s: unable to attach devsw: %d\n",
			    scmd_cd.cd_name, error);
			return error;
		}

		error = config_init_component(cfdriver_ioconf_scmd,
		    cfattach_ioconf_scmd, cfdata_ioconf_scmd);
		if (error) {
			aprint_error("%s: unable to init component: %d\n",
			    scmd_cd.cd_name, error);
			devsw_detach(NULL, &scmd_cdevsw);
		}
		return error;
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_scmd,
		    cfattach_ioconf_scmd, cfdata_ioconf_scmd);
		devsw_detach(NULL, &scmd_cdevsw);

		return error;
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
