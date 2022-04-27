
/*	$NetBSD: sht3x.c,v 1.7 2022/04/27 23:11:25 brad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sht3x.c,v 1.7 2022/04/27 23:11:25 brad Exp $");

/*
  Driver for the Sensirion SHT30/SHT31/SHT35
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
#include <sys/kthread.h>
#include <sys/pool.h>
#include <sys/kmem.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/sht3xreg.h>
#include <dev/i2c/sht3xvar.h>

static int	sht3x_take_break(void *, bool);
static int	sht3x_get_status_register(void *, uint16_t *, bool);
static int	sht3x_clear_status_register(void *, bool);
static uint8_t 	sht3x_crc(uint8_t *, size_t);
static int	sht3x_cmdr(struct sht3x_sc *, uint16_t, uint8_t *, size_t);
static int 	sht3x_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	sht3x_match(device_t, cfdata_t, void *);
static void 	sht3x_attach(device_t, device_t, void *);
static int 	sht3x_detach(device_t, int);
static void 	sht3x_refresh(struct sysmon_envsys *, envsys_data_t *);
static int 	sht3x_verify_sysctl(SYSCTLFN_ARGS);
static int 	sht3x_verify_sysctl_heateron(SYSCTLFN_ARGS);
static int 	sht3x_verify_sysctl_modes(SYSCTLFN_ARGS);
static int 	sht3x_verify_sysctl_repeatability(SYSCTLFN_ARGS);
static int 	sht3x_verify_sysctl_rate(SYSCTLFN_ARGS);
static int	sht3x_set_heater(struct sht3x_sc *);
static void     sht3x_thread(void *);
static int	sht3x_init_periodic_measurement(void *, int *);
static void     sht3x_take_periodic_measurement(void *);
static void     sht3x_start_thread(void *);
static void     sht3x_stop_thread(void *);
static int	sht3x_activate(device_t, enum devact);

#define SHT3X_DEBUG
#ifdef SHT3X_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_sht3xdebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(sht3xtemp, sizeof(struct sht3x_sc),
    sht3x_match, sht3x_attach, sht3x_detach, sht3x_activate);

extern struct cfdriver sht3xtemp_cd;

static dev_type_open(sht3xopen);
static dev_type_read(sht3xread);
static dev_type_close(sht3xclose);
const struct cdevsw sht3x_cdevsw = {
	.d_open = sht3xopen,
	.d_close = sht3xclose,
	.d_read = sht3xread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static struct sht3x_sensor sht3x_sensors[] = {
	{
		.desc = "humidity",
		.type = ENVSYS_SRELHUMIDITY,
	},
	{
		.desc = "temperature",
		.type = ENVSYS_STEMP,
	}
};

/* The typical delays are MOSTLY documented in the datasheet for the chip.
   There is no need to be very accurate with these, just rough estimates
   will work fine.
*/

static struct sht3x_timing sht3x_timings[] = {
	{
		.cmd = SHT3X_SOFT_RESET,
		.typicaldelay = 3000,
	},
	{
		.cmd = SHT3X_GET_STATUS_REGISTER,
		.typicaldelay = 100,
	},
	{
		.cmd = SHT3X_BREAK,
		.typicaldelay = 100,
	},
	{
		.cmd = SHT3X_CLEAR_STATUS_REGISTER,
		.typicaldelay = 100,
	},
	{
		.cmd = SHT3X_MEASURE_REPEATABILITY_CS_HIGH,
		.typicaldelay = 15000,
	},
	{
		.cmd = SHT3X_MEASURE_REPEATABILITY_CS_MEDIUM,
		.typicaldelay = 6000,
	},
	{
		.cmd = SHT3X_MEASURE_REPEATABILITY_CS_LOW,
		.typicaldelay = 4000,
	},
	{
		.cmd = SHT3X_MEASURE_REPEATABILITY_NOCS_HIGH,
		.typicaldelay = 15000,
	},
	{
		.cmd = SHT3X_MEASURE_REPEATABILITY_NOCS_MEDIUM,
		.typicaldelay = 6000,
	},
	{
		.cmd = SHT3X_MEASURE_REPEATABILITY_NOCS_LOW,
		.typicaldelay = 4000,
	},
	{
		.cmd = SHT3X_WRITE_HIGH_ALERT_SET,
		.typicaldelay = 5000,
	},
	{
		.cmd = SHT3X_WRITE_HIGH_ALERT_CLEAR,
		.typicaldelay = 5000,
	},
	{
		.cmd = SHT3X_WRITE_LOW_ALERT_SET,
		.typicaldelay = 5000,
	},
	{
		.cmd = SHT3X_WRITE_LOW_ALERT_CLEAR,
		.typicaldelay = 5000,
	}
};

/* In single shot mode, find the command */

static struct sht3x_repeatability sht3x_repeatability_ss[] = {
	{
		.text = "high",
		.cmd = SHT3X_MEASURE_REPEATABILITY_NOCS_HIGH,
	},
	{
		.text = "medium",
		.cmd = SHT3X_MEASURE_REPEATABILITY_NOCS_MEDIUM,
	},
	{
		.text = "low",
		.cmd = SHT3X_MEASURE_REPEATABILITY_NOCS_LOW,
	}
};


/* For periodic, look at the repeatability and the rate.
 * ART is a bit fake here, as the repeatability is not really
 * used.
 */

static struct sht3x_periodic sht3x_periodic_rate[] = {
	{
		.repeatability = "high",
		.rate = "0.5mps",
		.sdelay = 1000,
		.cmd = SHT3X_HALF_MPS_HIGH,
	},
	{
		.repeatability = "medium",
		.rate = "0.5mps",
		.sdelay = 1000,
		.cmd = SHT3X_HALF_MPS_MEDIUM,
	},
	{
		.repeatability = "low",
		.rate = "0.5mps",
		.sdelay = 1000,
		.cmd = SHT3X_HALF_MPS_LOW,
	},
	{
		.repeatability = "high",
		.rate = "1.0mps",
		.sdelay = 500,
		.cmd = SHT3X_ONE_MPS_HIGH,
	},
	{
		.repeatability = "medium",
		.rate = "1.0mps",
		.sdelay = 500,
		.cmd = SHT3X_ONE_MPS_MEDIUM,
	},
	{
		.repeatability = "low",
		.rate = "1.0mps",
		.sdelay = 500,
		.cmd = SHT3X_ONE_MPS_LOW,
	},
	{
		.repeatability = "high",
		.rate = "2.0mps",
		.sdelay = 250,
		.cmd = SHT3X_TWO_MPS_HIGH,
	},
	{
		.repeatability = "medium",
		.rate = "2.0mps",
		.sdelay = 250,
		.cmd = SHT3X_TWO_MPS_MEDIUM,
	},
	{
		.repeatability = "low",
		.rate = "2.0mps",
		.sdelay = 250,
		.cmd = SHT3X_TWO_MPS_LOW,
	},
	{
		.repeatability = "high",
		.rate = "4.0mps",
		.sdelay = 100,
		.cmd = SHT3X_FOUR_MPS_HIGH,
	},
	{
		.repeatability = "medium",
		.rate = "4.0mps",
		.sdelay = 100,
		.cmd = SHT3X_FOUR_MPS_MEDIUM,
	},
	{
		.repeatability = "low",
		.rate = "4.0mps",
		.sdelay = 100,
		.cmd = SHT3X_FOUR_MPS_LOW,
	},
	{
		.repeatability = "high",
		.rate = "10.0mps",
		.sdelay = 50,
		.cmd = SHT3X_TEN_MPS_HIGH,
	},
	{
		.repeatability = "medium",
		.rate = "10.0mps",
		.sdelay = 50,
		.cmd = SHT3X_FOUR_MPS_MEDIUM,
	},
	{
		.repeatability = "low",
		.rate = "10.0mps",
		.sdelay = 50,
		.cmd = SHT3X_FOUR_MPS_LOW,
	},
	{
		.repeatability = "high",
		.rate = "ART",
		.sdelay = 100,
		.cmd = SHT3X_ART_ENABLE,
	},
	{
		.repeatability = "medium",
		.rate = "ART",
		.sdelay = 100,
		.cmd = SHT3X_ART_ENABLE,
	},
	{
		.repeatability = "low",
		.rate = "ART",
		.sdelay = 100,
		.cmd = SHT3X_ART_ENABLE,
	}
};

static const char sht3x_rate_names[] =
    "0.5mps, 1.0mps, 2.0mps, 4.0mps, 10.0mps, ART";

static const char sht3x_mode_names[] =
    "single-shot, periodic";

static const char sht3x_repeatability_names[] =
    "high, medium, low";

static int
sht3x_take_break(void *aux, bool have_bus)
{
	struct sht3x_sc *sc;
	sc = aux;
	int error = 0;

	if (! have_bus) {
		error = iic_acquire_bus(sc->sc_tag, 0);
		if (error) {
			DPRINTF(sc, 2, ("%s: Could not acquire iic bus for "
			    "breaking %d\n", device_xname(sc->sc_dev), error));
			goto out;
		}
	}
	error = sht3x_cmdr(sc, SHT3X_BREAK, NULL, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Error breaking: %d\n",
		    device_xname(sc->sc_dev), error));
	}
out:
	if (! have_bus) {
		iic_release_bus(sc->sc_tag, 0);
	}

	sc->sc_isperiodic = false;
	strlcpy(sc->sc_mode, "single-shot", SHT3X_MODE_NAME);

	return error;
}

static int
sht3x_get_status_register(void *aux, uint16_t *reg, bool have_bus)
{
	struct sht3x_sc *sc = aux;
	uint8_t buf[3];
	int error;

	if (! have_bus) {
		error = iic_acquire_bus(sc->sc_tag, 0);
		if (error) {
			DPRINTF(sc, 2, ("%s: Could not acquire iic bus for "
			    "getting status %d\n", device_xname(sc->sc_dev),
			    error));
			return error;
		}
	}
	error = sht3x_cmdr(sc, SHT3X_GET_STATUS_REGISTER, buf, 3);
	if (error) {
		DPRINTF(sc, 2, ("%s: Error getting status: %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	uint8_t c = sht3x_crc(&buf[0], 2);
	if (c == buf[2]) {
		*reg = buf[0] << 8 | buf[1];
	} else {
		error = EINVAL;
	}
out:
	if (! have_bus) {
		iic_release_bus(sc->sc_tag, 0);
	}

	return error;
}

static int
sht3x_clear_status_register(void *aux, bool have_bus)
{
	struct sht3x_sc *sc = aux;
	int error;

	if (! have_bus) {
		error = iic_acquire_bus(sc->sc_tag, 0);
		if (error) {
			DPRINTF(sc, 2, ("%s: Could not acquire iic bus for "
			    "clearing status %d\n", device_xname(sc->sc_dev),
			    error));
			return error;
		}
	}
	error = sht3x_cmdr(sc, SHT3X_CLEAR_STATUS_REGISTER, NULL, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Error clear status register: %d\n",
		    device_xname(sc->sc_dev), error));
	}
	if (! have_bus) {
		iic_release_bus(sc->sc_tag, 0);
	}

	return error;
}

void
sht3x_thread(void *aux)
{
	struct sht3x_sc *sc = aux;
	int error, rv;
	int sdelay = 100;

	mutex_enter(&sc->sc_threadmutex);

	while (!sc->sc_stopping && !sc->sc_dying) {
		if (sc->sc_initperiodic) {
			error = sht3x_init_periodic_measurement(sc, &sdelay);
			if (error) {
				DPRINTF(sc, 2, ("%s: Error initing periodic "
				    "measurement in thread: %d\n",
				    device_xname(sc->sc_dev), error));
			}
			sc->sc_initperiodic = false;
		}
		rv = cv_timedwait(&sc->sc_condvar, &sc->sc_threadmutex,
		    mstohz(sdelay));
		if (rv == EWOULDBLOCK && !sc->sc_stopping &&
		    !sc->sc_initperiodic && !sc->sc_dying) {
			sht3x_take_periodic_measurement(sc);
		}
	}
	mutex_exit(&sc->sc_threadmutex);
	kthread_exit(0);
}

int
sht3x_init_periodic_measurement(void *aux, int *sdelay)
{
	struct sht3x_sc *sc = aux;
	size_t i;
	int error;
	uint16_t r;

	for (i = 0; i < __arraycount(sht3x_periodic_rate); i++) {
		if (strncmp(sc->sc_repeatability,
		    sht3x_periodic_rate[i].repeatability, SHT3X_REP_NAME) == 0 &&
		    strncmp(sc->sc_periodic_rate, sht3x_periodic_rate[i].rate,
		    SHT3X_RATE_NAME) == 0)
		{
			r = sht3x_periodic_rate[i].cmd;
			*sdelay = sht3x_periodic_rate[i].sdelay;
			break;
		}
	}

	if (i == __arraycount(sht3x_periodic_rate)) {
		*sdelay = 100;
		return ENODEV;
	}

	DPRINTF(sc, 2, ("%s: Would init with: %x\n",
	    device_xname(sc->sc_dev), r));

	mutex_enter(&sc->sc_mutex);

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire iic bus for initing: "
		    " %d\n", device_xname(sc->sc_dev), error));
		goto outm;
	}

	error = sht3x_take_break(sc, true);
	if (error) {
	    DPRINTF(sc, 2, ("%s: Could not acquire iic bus for initing: "
		" %d\n", device_xname(sc->sc_dev), error));
	    goto out;
	}

	error = sht3x_cmdr(sc, r, NULL, 0);
	if (error) {
		DPRINTF(sc, 2,
		    ("%s: Error sending periodic measurement command: %d\n",
		    device_xname(sc->sc_dev), error));
		goto out;
	}

	sc->sc_isperiodic = true;
	strlcpy(sc->sc_mode, "periodic", SHT3X_MODE_NAME);

out:
	iic_release_bus(sc->sc_tag, 0);
outm:
	mutex_exit(&sc->sc_mutex);
	return error;
}

static void
sht3x_take_periodic_measurement(void *aux)
{
	struct sht3x_sc *sc = aux;
	int error;
	struct sht3x_read_q *pp;
	uint8_t rawbuf[MAX(sizeof(sc->sc_pbuffer), sizeof(pp->measurement))];
	uint16_t status_reg;

	mutex_enter(&sc->sc_mutex);
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire iic bus for getting "
		    "periodic data: %d\n", device_xname(sc->sc_dev), error));
		goto out;
	}

	error = sht3x_get_status_register(sc, &status_reg, true);
	if (error) {
		DPRINTF(sc, 2,
		    ("%s: Error getting status register periodic: %d\n",
		    device_xname(sc->sc_dev), error));
		goto err;
	}

	if (status_reg & SHT3X_RESET_DETECTED) {
		aprint_error_dev(sc->sc_dev, "Reset detected in periodic mode. "
		    "Heater may have been reset.\n");
		delay(3000);
		sht3x_take_break(sc, true);
		sht3x_clear_status_register(sc, true);
		sc->sc_heateron = status_reg & SHT3X_HEATER_STATUS;
		sc->sc_initperiodic = true;
	} else {
		int data_error = sht3x_cmdr(sc, SHT3X_PERIODIC_FETCH_DATA,
		    rawbuf, sizeof(rawbuf));
		/*
		 * EIO is actually expected if the poll interval is faster
		 * than the rate that the sensor is set to.  Unfortunally,
		 * this will also mess with the ability to detect an actual
		 * problem with the sensor in periodic mode, so we do the best
		 * we can here.
		 */
		if (data_error) {
			if (data_error != EIO) {
				DPRINTF(sc, 2, ("%s: Error sending periodic "
				    "fetch command: %d\n",
				    device_xname(sc->sc_dev), data_error));
			}
			goto err;
		}
	}

	iic_release_bus(sc->sc_tag, 0);
	/*
	 * If there was no errors from anything then the data should be
	 * valid.
	 */
	DPRINTF(sc, 2, ("%s: Raw periodic: %x%x - %x -- %x%x - %x\n",
	    device_xname(sc->sc_dev), rawbuf[0], rawbuf[1], rawbuf[2],
	    rawbuf[3], rawbuf[4], rawbuf[5]));
	memcpy(sc->sc_pbuffer, rawbuf, sizeof(sc->sc_pbuffer));

	if (sc->sc_opened) {
		mutex_enter(&sc->sc_read_mutex);
		pp = pool_cache_get(sc->sc_readpool, PR_NOWAIT);
		if (pp == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "Could not allocate memory for pool read\n");
		} else {
			memcpy(pp->measurement, rawbuf, sizeof(pp->measurement));
			DPRINTF(sc, 4, ("%s: Queue insert\n",
			    device_xname(sc->sc_dev)));
			SIMPLEQ_INSERT_HEAD(&sc->sc_read_queue, pp, read_q);
		}
		cv_signal(&sc->sc_condreadready);
		mutex_exit(&sc->sc_read_mutex);
	}
out:
	mutex_exit(&sc->sc_mutex);
	return;
err:
	/*
	 * We are only going to worry about errors when it was not related
	 * to actually getting data.  That is a likely indicator of a problem
	 * with the sensor.
	 */
	DPRINTF(sc, 2, ("%s: Raw periodic with error: %x%x - %x -- "
	    "%x%x - %x -- %d\n", device_xname(sc->sc_dev), rawbuf[0], rawbuf[1],
	    rawbuf[2], rawbuf[3], rawbuf[4], rawbuf[5], error));
	iic_release_bus(sc->sc_tag, 0);
	if (error != 0) {
		memcpy(sc->sc_pbuffer, "dedbef", sizeof(sc->sc_pbuffer));
	}
	mutex_exit(&sc->sc_mutex);
}

static void
sht3x_stop_thread(void *aux)
{
	struct sht3x_sc *sc;
	sc = aux;

	if (!sc->sc_isperiodic) {
		return;
	}

	mutex_enter(&sc->sc_threadmutex);
	sc->sc_stopping = true;
	cv_signal(&sc->sc_condvar);
	mutex_exit(&sc->sc_threadmutex);

	/* wait for the thread to exit */
	kthread_join(sc->sc_thread);

	mutex_enter(&sc->sc_mutex);
	sht3x_take_break(sc,false);
	mutex_exit(&sc->sc_mutex);
}

static void
sht3x_start_thread(void *aux)
{
	struct sht3x_sc *sc;
	sc = aux;
	int error;

	error = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN, NULL,
	    sht3x_thread, sc, &sc->sc_thread, "%s", device_xname(sc->sc_dev));
	if (error) {
		DPRINTF(sc, 2, ("%s: Unable to create measurement thread: %d\n",
		    device_xname(sc->sc_dev), error));
	}
}

int
sht3x_verify_sysctl(SYSCTLFN_ARGS)
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

int
sht3x_verify_sysctl_heateron(SYSCTLFN_ARGS)
{
	int 		error;
	bool 		t;
	struct sht3x_sc *sc;
	struct sysctlnode node;

	node = *rnode;
	sc = node.sysctl_data;
	t = sc->sc_heateron;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	sc->sc_heateron = t;
	error = sht3x_set_heater(sc);

	return error;
}

static int
sht3x_set_heater(struct sht3x_sc *sc)
{
	int error = 0;
	uint16_t cmd;

	mutex_enter(&sc->sc_mutex);
	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s:%s: Failed to acquire bus: %d\n",
		    device_xname(sc->sc_dev), __func__, error));
		goto out;
	}

	if (sc->sc_heateron) {
		cmd = SHT3X_HEATER_ENABLE;
	} else {
		cmd = SHT3X_HEATER_DISABLE;
	}

	error = sht3x_cmdr(sc, cmd, NULL, 0);

	iic_release_bus(sc->sc_tag,0);
out:
	mutex_exit(&sc->sc_mutex);

	return error;
}

int
sht3x_verify_sysctl_modes(SYSCTLFN_ARGS)
{
	char buf[SHT3X_MODE_NAME];
	struct sht3x_sc *sc;
	struct sysctlnode node;
	bool is_ss = false;
	bool is_periodic = false;
	int error;

	node = *rnode;
	sc = node.sysctl_data;
	(void) memcpy(buf, sc->sc_mode, SHT3X_MODE_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (sc->sc_opened) {
		return EINVAL;
	}

	is_ss = strncmp(node.sysctl_data, "single-shot", SHT3X_MODE_NAME) == 0;
	is_periodic = strncmp(node.sysctl_data, "periodic", SHT3X_MODE_NAME)
	    == 0;

	if (!is_ss && !is_periodic) {
		return EINVAL;
	}

	(void) memcpy(sc->sc_mode, node.sysctl_data, SHT3X_MODE_NAME);
	if (is_ss) {
		sht3x_stop_thread(sc);
		sc->sc_stopping = false;
		sc->sc_initperiodic = false;
		sc->sc_isperiodic = false;
	}

	if (is_periodic) {
		sc->sc_stopping = false;
		sc->sc_initperiodic = true;
		sc->sc_isperiodic = true;
		sht3x_start_thread(sc);
	}

	return 0;
}

int
sht3x_verify_sysctl_repeatability(SYSCTLFN_ARGS)
{
	char buf[SHT3X_REP_NAME];
	struct sht3x_sc *sc;
	struct sysctlnode node;
	int error;
	size_t i;

	node = *rnode;
	sc = node.sysctl_data;
	(void) memcpy(buf, sc->sc_repeatability, SHT3X_REP_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(sht3x_repeatability_ss); i++) {
		if (strncmp(node.sysctl_data, sht3x_repeatability_ss[i].text,
		    SHT3X_REP_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(sht3x_repeatability_ss))
		return EINVAL;
	(void) memcpy(sc->sc_repeatability, node.sysctl_data, SHT3X_REP_NAME);

	if (sc->sc_isperiodic) {
		sc->sc_initperiodic = true;
	}

	return error;
}

int
sht3x_verify_sysctl_rate(SYSCTLFN_ARGS)
{
	char buf[SHT3X_RATE_NAME];
	struct sht3x_sc *sc;
	struct sysctlnode node;
	int error;
	size_t i;

	node = *rnode;
	sc = node.sysctl_data;
	(void) memcpy(buf, sc->sc_periodic_rate, SHT3X_RATE_NAME);
	node.sysctl_data = buf;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (i = 0; i < __arraycount(sht3x_periodic_rate); i++) {
		if (strncmp(node.sysctl_data, sht3x_periodic_rate[i].rate,
		    SHT3X_RATE_NAME) == 0) {
			break;
		}
	}

	if (i == __arraycount(sht3x_periodic_rate))
		return EINVAL;

	(void) memcpy(sc->sc_periodic_rate, node.sysctl_data, SHT3X_RATE_NAME);

	if (sc->sc_isperiodic) {
		sc->sc_initperiodic = true;
	}

	return error;
}

static int
sht3x_cmddelay(uint16_t cmd)
{
	size_t i;

	for (i = 0; i < __arraycount(sht3x_timings); i++) {
		if (cmd == sht3x_timings[i].cmd) {
			break;
		}
	}

	if (i == __arraycount(sht3x_timings)) {
		return -1;
	}
	return sht3x_timings[i].typicaldelay;
}

static int
sht3x_cmd(i2c_tag_t tag, i2c_addr_t addr, uint16_t *cmd,
    uint8_t clen, uint8_t *buf, size_t blen, int readattempts)
{
	int error;
	int cmddelay;
	uint8_t cmd8[2];

	/* All commands are two bytes and must be in a proper order */
	KASSERT(clen == 2);

	cmd8[0] = cmd[0] >> 8;
	cmd8[1] = cmd[0] & 0x00ff;

	error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &cmd8[0], clen,
	    NULL, 0, 0);
	if (error)
		return error;

	cmddelay = sht3x_cmddelay(cmd[0]);
	if (cmddelay != -1) {
		delay(cmddelay);
	}

	/* Not all commands return anything  */
	if (blen == 0) {
		return 0;
	}

	for (int aint = 0; aint < readattempts; aint++) {
		error = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, NULL, 0, buf,
		    blen, 0);
		if (error == 0)
			break;
		delay(1000);
	}

	return error;
}

static int
sht3x_cmdr(struct sht3x_sc *sc, uint16_t cmd, uint8_t *buf, size_t blen)
{
	return sht3x_cmd(sc->sc_tag, sc->sc_addr, &cmd, 2, buf, blen,
	    sc->sc_readattempts);
}

static	uint8_t
sht3x_crc(uint8_t *data, size_t size)
{
	uint8_t crc = 0xFF;

	for (size_t i = 0; i < size; i++) {
		crc ^= data[i];
		for (size_t j = 8; j > 0; j--) {
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc <<= 1;
		}
	}
	return crc;
}

static int
sht3x_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint16_t reg = SHT3X_GET_STATUS_REGISTER;
	uint8_t buf[3];
	int error;

	error = sht3x_cmd(tag, addr, &reg, 2, buf, 3, 10);
	if (matchdebug) {
		printf("poke X 1: %d\n", error);
	}
	return error;
}

static int
sht3x_sysctl_init(struct sht3x_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("sht3x controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef SHT3X_DEBUG
	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), sht3x_verify_sysctl, 0,
	    &sc->sc_sht3xdebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "readattempts",
	    SYSCTL_DESCR("The number of times to attempt to read the values"),
	    sht3x_verify_sysctl, 0, &sc->sc_readattempts, 0, CTL_HW,
	    sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "modes",
	    SYSCTL_DESCR("Valid modes"), 0, 0,
	    __UNCONST(sht3x_mode_names),
	    sizeof(sht3x_mode_names) + 1,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "mode",
	    SYSCTL_DESCR("Mode for measurement collection"),
	    sht3x_verify_sysctl_modes, 0, (void *) sc,
	    SHT3X_MODE_NAME, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "repeatabilities",
	    SYSCTL_DESCR("Valid repeatability values"), 0, 0,
	    __UNCONST(sht3x_repeatability_names),
	    sizeof(sht3x_repeatability_names) + 1,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "repeatability",
	    SYSCTL_DESCR("Repeatability of RH and Temp"),
	    sht3x_verify_sysctl_repeatability, 0, (void *) sc,
	    SHT3X_REP_NAME, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "rates",
	    SYSCTL_DESCR("Valid peridoic rates"), 0, 0,
	    __UNCONST(sht3x_rate_names),
	    sizeof(sht3x_rate_names) + 1,
	    CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "rate",
	    SYSCTL_DESCR("Rate for periodic measurements"),
	    sht3x_verify_sysctl_rate, 0, (void *) sc,
	    SHT3X_RATE_NAME, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ignorecrc",
	    SYSCTL_DESCR("Ignore the CRC byte"), NULL, 0, &sc->sc_ignorecrc,
	    0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sht3xlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "heateron",
	    SYSCTL_DESCR("Heater on"), sht3x_verify_sysctl_heateron, 0,
	    (void *)sc, 0, CTL_HW, sysctlroot_num, CTL_CREATE, CTL_EOL)) != 0)
		return error;

	return 0;
}

static int
sht3x_match(device_t parent, cfdata_t match, void *aux)
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
	if (ia->ia_addr != SHT3X_TYPICAL_ADDR_1 &&
	    ia->ia_addr != SHT3X_TYPICAL_ADDR_2)
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

	error = sht3x_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
sht3x_attach(device_t parent, device_t self, void *aux)
{
	struct sht3x_sc *sc;
	struct i2c_attach_args *ia;
	int error, i;
	int ecount = 0;
	uint8_t buf[6];
	uint32_t serialnumber;
	uint8_t sncrcpt1, sncrcpt2;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_sht3xdebug = 0;
	strlcpy(sc->sc_mode, "single-shot", SHT3X_MODE_NAME);
	sc->sc_isperiodic = false;
	strlcpy(sc->sc_repeatability, "high", SHT3X_REP_NAME);
	strlcpy(sc->sc_periodic_rate, "1.0mps", SHT3X_RATE_NAME);
	sc->sc_readattempts = 10;
	sc->sc_ignorecrc = false;
	sc->sc_heateron = false;
	sc->sc_sme = NULL;
	sc->sc_stopping = false;
	sc->sc_initperiodic = false;
	sc->sc_opened = false;
	sc->sc_dying = false;
	sc->sc_readpoolname = NULL;

	aprint_normal("\n");

	mutex_init(&sc->sc_dying_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_read_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_threadmutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condvar, "sht3xcv");
	cv_init(&sc->sc_condreadready, "sht3xread");
	cv_init(&sc->sc_cond_dying, "sht3xdie");
	sc->sc_numsensors = __arraycount(sht3x_sensors);

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}
	if ((error = sht3x_sysctl_init(sc)) != 0) {
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	sc->sc_readpoolname = kmem_asprintf("sht3xrp%d",device_unit(self));
	sc->sc_readpool = pool_cache_init(sizeof(struct sht3x_read_q), 0, 0, 0,
	    sc->sc_readpoolname, NULL, IPL_VM, NULL, NULL, NULL);
	pool_cache_sethiwat(sc->sc_readpool,100);

	SIMPLEQ_INIT(&sc->sc_read_queue);

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	error = sht3x_cmdr(sc, SHT3X_SOFT_RESET, NULL, 0);
	if (error != 0)
		aprint_error_dev(self, "Reset failed: %d\n", error);

	error = sht3x_clear_status_register(sc, true);
	if (error) {
		aprint_error_dev(self, "Failed to clear status register: %d\n",
		    error);
		ecount++;
	}

	uint16_t status_reg;
	error = sht3x_get_status_register(sc, &status_reg, true);
	if (error) {
		aprint_error_dev(self, "Failed to read status register: %d\n",
		    error);
		ecount++;
	}

	DPRINTF(sc, 2, ("%s: read status register values: %04x\n",
	    device_xname(sc->sc_dev), status_reg));

	error = sht3x_cmdr(sc, SHT3X_READ_SERIAL_NUMBER, buf, 6);
	if (error) {
		aprint_error_dev(self, "Failed to read serial number: %d\n",
		    error);
		ecount++;
	}

	sncrcpt1 = sht3x_crc(&buf[0],2);
	sncrcpt2 = sht3x_crc(&buf[3],2);
	serialnumber = (buf[0] << 24) | (buf[1] << 16) | (buf[3] << 8) | buf[4];

	DPRINTF(sc, 2, ("%s: read serial number values: %02x%02x - %02x - "
	    "%02x%02x - %02x -- %02x %02x\n", device_xname(sc->sc_dev), buf[0],
	    buf[1], buf[2], buf[3], buf[4], buf[5], sncrcpt1, sncrcpt2));

	iic_release_bus(sc->sc_tag, 0);
	if (error != 0) {
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	for (i = 0; i < sc->sc_numsensors; i++) {
		strlcpy(sc->sc_sensors[i].desc, sht3x_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc));

		sc->sc_sensors[i].units = sht3x_sensors[i].type;
		sc->sc_sensors[i].state = ENVSYS_SINVALID;

		DPRINTF(sc, 2, ("%s: registering sensor %d (%s)\n", __func__, i,
		    sc->sc_sensors[i].desc));

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[i]);
		if (error) {
			aprint_error_dev(self,
			    "Unable to attach sensor %d: %d\n", i, error);
			goto out;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = sht3x_refresh;

	DPRINTF(sc, 2, ("sht3x_attach: registering with envsys\n"));

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	/*
	 * There is no documented way to ask the chip what version it is. This
	 * is likely fine as the only apparent difference is in how precise the
	 * measurements will be. The actual conversation with the chip is
	 * identical no matter which one you are talking to.
	 */

	aprint_normal_dev(self, "Sensirion SHT30/SHT31/SHT35, "
	    "Serial number: %x%s", serialnumber,
	    (sncrcpt1 == buf[2] && sncrcpt2 == buf[5]) ? "\n" : " (bad crc)\n");
	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

static uint16_t
sht3x_compute_measure_command_ss(const char *repeatability)
{
	int i;
	uint16_t r;

	for (i = 0; i < __arraycount(sht3x_repeatability_ss); i++) {
		if (strncmp(repeatability, sht3x_repeatability_ss[i].text,
		    SHT3X_REP_NAME) == 0) {
			r = sht3x_repeatability_ss[i].cmd;
			break;
		}
	}

	if (i == __arraycount(sht3x_repeatability_ss))
		panic("Single-shot could not find command for "
		    "repeatability: %s\n", repeatability);

	return r;
}

/*
 * The documented conversion calculations for the raw values are as follows:
 *
 * %RH = (-6 + 125 * rawvalue / 65535)
 *
 * T in Celsius = (-45 + 175 * rawvalue / 65535)
 *
 * It follows then:
 *
 * T in Kelvin = (228.15 + 175 * rawvalue / 65535)
 *
 * given the relationship between Celsius and Kelvin
 *
 * What follows reorders the calculation a bit and scales it up to avoid
 * the use of any floating point.  All that would really have to happen
 * is a scale up to 10^6 for the sysenv framework, which wants
 * temperature in micro-kelvin and percent relative humidity scaled up
 * 10^6, but since this conversion uses 64 bits due to intermediate
 * values that are bigger than 32 bits the conversion first scales up to
 * 10^9 and the scales back down by 10^3 at the end.  This preserves some
 * precision in the conversion that would otherwise be lost.
 */

static uint64_t
sht3x_compute_temp_from_raw(uint8_t msb, uint8_t lsb) {
	uint64_t svalue;
	int64_t v1;
	uint64_t v2;
	uint64_t d1 = 65535;
	uint64_t mul1;
	uint64_t mul2;
	uint64_t div1 = 10000;
	uint64_t q;

	svalue = msb << 8 | lsb;

	v1 = 22815; /* this is scaled up already from 228.15 */
	v2 = 175;
	mul1 = 10000000000;
	mul2 = 100000000;

	svalue = svalue * mul1;
	v1 = v1 * mul2;
	/* Perform the conversion */
	q = ((v2 * (svalue / d1)) + v1) / div1;

	return q;
}

static uint64_t
sht3x_compute_rh_from_raw(uint8_t msb, uint8_t lsb) {
	uint64_t svalue;
	int64_t v1;
	uint64_t v2;
	uint64_t d1 = 65535;
	uint64_t mul1;
	uint64_t mul2;
	uint64_t div1 = 10000;
	uint64_t q;

	svalue = msb << 8 | lsb;

	v1 = 0;
	v2 = 100;
	mul1 = 10000000000;
	mul2 = 10000000000;

	svalue = svalue * mul1;
	v1 = v1 * mul2;
	/* Perform the conversion */
	q = ((v2 * (svalue / d1)) + v1) / div1;

	return q;
}

static int
sht3x_parse_data(struct sht3x_sc *sc, envsys_data_t *edata, uint8_t *rawdata)
{
	uint64_t current_value;
	uint8_t *svalptr;

	DPRINTF(sc, 2, ("%s: Raw data: %02x%02x %02x - %02x%02x %02x\n",
	    device_xname(sc->sc_dev), rawdata[0], rawdata[1], rawdata[2],
	    rawdata[3], rawdata[4], rawdata[5]));

	switch (edata->sensor) {
	case SHT3X_TEMP_SENSOR:
		current_value = sht3x_compute_temp_from_raw(rawdata[0],
		    rawdata[1]);
		svalptr = &rawdata[0];
		break;
	case SHT3X_HUMIDITY_SENSOR:
		current_value = sht3x_compute_rh_from_raw(rawdata[3],
		    rawdata[4]);
		svalptr = &rawdata[3];
		break;
	default:
		DPRINTF(sc, 2, ("%s: bad sensor type %d\n",
		    device_xname(sc->sc_dev), edata->sensor));
		return EINTR;
	}
	uint8_t testcrc;
	/* Fake out the CRC check if being asked to ignore CRC */
	if (sc->sc_ignorecrc) {
		testcrc = *(svalptr + 2);
	} else {
		testcrc = sht3x_crc(svalptr, 2);
	}

	if (*(svalptr + 2) != testcrc) {
	    DPRINTF(sc, 2, ("%s: Failed to get new status in refresh %d != %d\n",
	    device_xname(sc->sc_dev), (*svalptr + 2), testcrc));
	    return EINVAL;
	}
	edata->value_cur = (uint32_t) current_value;
	edata->state = ENVSYS_SVALID;
	return 0;
}

static int
sht3x_refresh_periodic(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct sht3x_sc *sc = sme->sme_cookie;
	uint8_t rawdata[sizeof(sc->sc_pbuffer)];

	memcpy(rawdata, sc->sc_pbuffer, sizeof(rawdata));

	return sht3x_parse_data(sc, edata, rawdata);

}

static int
sht3x_refresh_oneshot(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct sht3x_sc *sc = sme->sme_cookie;
	uint16_t measurement_command_ss;
	uint8_t rawdata[sizeof(sc->sc_pbuffer)];
	int error;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		DPRINTF(sc, 2, ("%s: Could not acquire i2c bus: %x\n",
		    device_xname(sc->sc_dev), error));
		return error;
	}

	measurement_command_ss = sht3x_compute_measure_command_ss(
	    sc->sc_repeatability);
	error = sht3x_cmdr(sc, measurement_command_ss, rawdata, sizeof(rawdata));
	DPRINTF(sc, 2, ("%s: Status for single-shot measurement cmd %04x "
	    "Error %d\n", device_xname(sc->sc_dev), measurement_command_ss, error));
	if (error == 0) {
		error = sht3x_parse_data(sc, edata, rawdata);
	}

	uint16_t sbuf;
	int status_error = sht3x_get_status_register(sc, &sbuf, true);

	if (!status_error) {
		DPRINTF(sc, 2, ("%s: read status register single-shot: %04x\n",
		    device_xname(sc->sc_dev), sbuf));

		if (sbuf & SHT3X_RESET_DETECTED) {
			aprint_error_dev(sc->sc_dev,
			    "Reset detected in single shot mode. "
			    "Heater may have been reset\n");
			sht3x_clear_status_register(sc, true);
		}

		sc->sc_heateron = sbuf & SHT3X_HEATER_STATUS;
	}

	iic_release_bus(sc->sc_tag, 0);

	return error;
}

static void
sht3x_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct sht3x_sc *sc = sme->sme_cookie;

	edata->state = ENVSYS_SINVALID;

	mutex_enter(&sc->sc_mutex);

	if (sc->sc_isperiodic) {
		sht3x_refresh_periodic(sme, edata);
	} else {
		sht3x_refresh_oneshot(sme, edata);
	}

	mutex_exit(&sc->sc_mutex);
}

static int
sht3xopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct sht3x_sc *sc;

	sc = device_lookup_private(&sht3xtemp_cd, minor(dev));
	if (!sc)
		return ENXIO;

	if (sc->sc_opened)
		return EBUSY;

	mutex_enter(&sc->sc_mutex);
	sc->sc_opened = true;

	sc->sc_wassingleshot = false;
	if (!sc->sc_isperiodic) {
		sc->sc_stopping = false;
		sc->sc_initperiodic = true;
		sc->sc_isperiodic = true;
		sc->sc_wassingleshot = true;
		sht3x_start_thread(sc);
	}
	mutex_exit(&sc->sc_mutex);

	return 0;
}

static int
sht3xread(dev_t dev, struct uio *uio, int flags)
{
	struct sht3x_sc *sc;
	struct sht3x_read_q *pp;
	int error,any;

	sc = device_lookup_private(&sht3xtemp_cd, minor(dev));
	if (!sc)
		return ENXIO;

	while (uio->uio_resid) {
		any = 0;
		error = 0;
		mutex_enter(&sc->sc_read_mutex);

		while (any == 0) {
			pp = SIMPLEQ_FIRST(&sc->sc_read_queue);
			if (pp != NULL) {
				SIMPLEQ_REMOVE_HEAD(&sc->sc_read_queue, read_q);
				any = 1;
				break;
			}
			error = cv_wait_sig(&sc->sc_condreadready,
			    &sc->sc_read_mutex);
			if (sc->sc_dying)
				error = EIO;
			if (error == 0)
				continue;
			break;
		}

		if (any == 1 && error == 0) {
			uint8_t *p = pp->measurement;
			mutex_exit(&sc->sc_read_mutex);
			pool_cache_put(sc->sc_readpool,pp);

			DPRINTF(sc,2, ("%s: sending %02x%02x %02x -- %02x%02x "
			    "%02x -- %x\n", device_xname(sc->sc_dev), p[0],
			    p[1], p[2], p[3], p[4], p[5],
			    mutex_owned(&sc->sc_read_mutex)));
			if ((error = uiomove(pp->measurement,
			    sizeof(pp->measurement), uio)) != 0) {
				DPRINTF(sc,2, ("%s: send error %d\n",
				    device_xname(sc->sc_dev), error));
				break;
			}
		} else {
			mutex_exit(&sc->sc_read_mutex);
			if (error) {
				break;
			}
		}
	}

	DPRINTF(sc,2, ("%s: loop done: %d\n",device_xname(sc->sc_dev),error));
	if (sc->sc_dying) {
		DPRINTF(sc, 2, ("%s: Telling all we are almost dead\n",
		    device_xname(sc->sc_dev)));
		mutex_enter(&sc->sc_dying_mutex);
		cv_signal(&sc->sc_cond_dying);
		mutex_exit(&sc->sc_dying_mutex);
	}
	return error;
}

static int
sht3xclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct sht3x_sc *sc;
	struct sht3x_read_q *pp;

	sc = device_lookup_private(&sht3xtemp_cd, minor(dev));

	if (sc->sc_wassingleshot) {
		sht3x_stop_thread(sc);
		sc->sc_stopping = false;
		sc->sc_initperiodic = false;
		sc->sc_isperiodic = false;
	}

	mutex_enter(&sc->sc_mutex);
	/* Drain any read pools */
	while ((pp = SIMPLEQ_FIRST(&sc->sc_read_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_read_queue, read_q);
		pool_cache_put(sc->sc_readpool,pp);
	}

	/* Say that the device is now free */
	sc->sc_opened = false;
	mutex_exit(&sc->sc_mutex);

	return(0);
}

static int
sht3x_detach(device_t self, int flags)
{
	struct sht3x_sc *sc;
	struct sht3x_read_q *pp;

	sc = device_private(self);

	if (sc->sc_isperiodic) {
		sht3x_stop_thread(sc);
	}

	mutex_enter(&sc->sc_mutex);

	sc->sc_dying = true;

	/* If this is true we are still open, destroy the condvar */
	if (sc->sc_opened) {
		mutex_enter(&sc->sc_dying_mutex);
		mutex_enter(&sc->sc_read_mutex);
		cv_signal(&sc->sc_condreadready);
		mutex_exit(&sc->sc_read_mutex);
		DPRINTF(sc, 2, ("%s: Will wait for anything to exit\n",
		    device_xname(sc->sc_dev)));
		/* In the worst case this will time out after 5 seconds.
		 * It really should not take that long for the drain / whatever
		 * to happen
		 */
		cv_timedwait_sig(&sc->sc_cond_dying,
		    &sc->sc_dying_mutex, mstohz(5000));
		mutex_exit(&sc->sc_dying_mutex);
		cv_destroy(&sc->sc_condreadready);
		cv_destroy(&sc->sc_cond_dying);
	}

	/* Drain any read pools */
	while ((pp = SIMPLEQ_FIRST(&sc->sc_read_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_read_queue, read_q);
		pool_cache_put(sc->sc_readpool,pp);
	}

	/* Destroy the pool cache now that nothing is using it */
	pool_cache_destroy(sc->sc_readpool);

	/* Remove the sensors */
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}
	mutex_exit(&sc->sc_mutex);

	/* Remove the sysctl tree */
	sysctl_teardown(&sc->sc_sht3xlog);

	/* Remove the mutex */
	mutex_destroy(&sc->sc_mutex);
	mutex_destroy(&sc->sc_threadmutex);
	mutex_destroy(&sc->sc_read_mutex);
	mutex_destroy(&sc->sc_dying_mutex);

	/* Free the poolname string */
        if (sc->sc_readpoolname != NULL) {
                kmem_free(sc->sc_readpoolname,strlen(sc->sc_readpoolname) + 1);
        }

	return 0;
}

int
sht3x_activate(device_t self, enum devact act)
{
	struct sht3x_sc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

MODULE(MODULE_CLASS_DRIVER, sht3xtemp, "iic,sysmon_envsys");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
sht3xtemp_modcmd(modcmd_t cmd, void *opaque)
{
	int error;
#ifdef _MODULE
	int bmaj = -1, cmaj = -1;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = devsw_attach("sht3xtemp", NULL, &bmaj,
		    &sht3x_cdevsw, &cmaj);
		if (error) {
			aprint_error("%s: unable to attach devsw\n",
			    sht3xtemp_cd.cd_name);
			return error;
		}

		error = config_init_component(cfdriver_ioconf_sht3xtemp,
		    cfattach_ioconf_sht3xtemp, cfdata_ioconf_sht3xtemp);
		if (error) {
			aprint_error("%s: unable to init component\n",
			    sht3xtemp_cd.cd_name);
			devsw_detach(NULL, &sht3x_cdevsw);
		}
		return error;
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_sht3xtemp,
		      cfattach_ioconf_sht3xtemp, cfdata_ioconf_sht3xtemp);
		devsw_detach(NULL, &sht3x_cdevsw);
		return error;
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
