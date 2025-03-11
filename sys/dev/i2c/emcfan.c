/*	$NetBSD: emcfan.c,v 1.1 2025/03/11 13:56:46 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: emcfan.c,v 1.1 2025/03/11 13:56:46 brad Exp $");

/*
 * Driver for the EMC-210x and EMC-230x fan controllers on a
 * I2C bus.
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
#include <dev/i2c/emcfanreg.h>
#include <dev/i2c/emcfanvar.h>
#include <dev/i2c/emcfaninfo.h>

static int 	emcfan_poke(i2c_tag_t, i2c_addr_t, bool);
static int 	emcfan_match(device_t, cfdata_t, void *);
static void 	emcfan_attach(device_t, device_t, void *);
static int 	emcfan_detach(device_t, int);
static void 	emcfan_refresh(struct sysmon_envsys *, envsys_data_t *);
static int	emcfan_activate(device_t, enum devact);
static int 	emcfan_verify_sysctl(SYSCTLFN_ARGS);
static void	emcfan_attach_gpio(struct emcfan_sc *, uint8_t);

#define EMCFAN_DEBUG
#ifdef EMCFAN_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_emcfandebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(emcfan, sizeof(struct emcfan_sc),
    emcfan_match, emcfan_attach, emcfan_detach, emcfan_activate);

extern struct cfdriver emcfan_cd;

static dev_type_open(emcfanopen);
static dev_type_read(emcfanread);
static dev_type_write(emcfanwrite);
static dev_type_close(emcfanclose);
const struct cdevsw emcfan_cdevsw = {
	.d_open = emcfanopen,
	.d_close = emcfanclose,
	.d_read = emcfanread,
	.d_write = emcfanwrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static bool
emcfan_reg_is_real(struct emcfan_sc *sc, uint8_t reg)
{
	int segment;
	uint64_t index;

	segment = reg / 64;
	index = reg % 64;

	DPRINTF(sc, 10, ("%s: void check 1: reg=%02x, segment=%d, index=%jd, sc_info_info=%d\n", __func__, reg,
	    segment, index, sc->sc_info_index));
	DPRINTF(sc, 10, ("%s: void check 2: register_void=%jx, shift=%jx\n", __func__,
	    emcfan_chip_infos[sc->sc_info_index].register_void[segment], ((uint64_t)1 << index)));

	return(emcfan_chip_infos[sc->sc_info_index].register_void[segment] & ((uint64_t)1 << index));
}

static int
emcfan_read_registerr(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    uint8_t *res)
{
	int error = 0;

	error = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, &reg, 1, res, 1, 0);

	return error;
}

static int
emcfan_read_register(struct emcfan_sc *sc, uint8_t reg, uint8_t *res)
{
	if (emcfan_reg_is_real(sc,reg))
		return(emcfan_read_registerr(sc->sc_tag, sc->sc_addr, reg, res));
	else
		*res = EMCFAN_VOID_READ;
	return 0;
}

static int
emcfan_write_registerr(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    uint8_t value)
{
	int error = 0;

	error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &reg, 1, &value, 1, 0);

	return error;
}

static int
emcfan_write_register(struct emcfan_sc *sc, uint8_t reg, uint8_t value)
{
	if (emcfan_reg_is_real(sc,reg))
		return(emcfan_write_registerr(sc->sc_tag, sc->sc_addr, reg, value));
	else
		return EACCES;
}

static int
emcfan_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	int error;
	uint8_t res;

	error = emcfan_read_registerr(tag, addr, EMCFAN_MANUFACTURER_ID, &res);
	if (matchdebug) {
		printf("poke X 1: %d %d\n", addr, error);
	}

	/* Ok..  something was there, but the ID did not match what was expected.
	 * We get away with doing this because the poke is just getting the Manufacturer
	 * ID, which is a fixed value.
	 */

	if (!error) {
		if (res != EMCFAN_VALID_MANUFACTURER_ID)
			error = EIO;
	}

	return error;
}

static bool
emcfan_check_i2c_addr(i2c_addr_t addr)
{
	bool r = false;

	for(int i = 0;i < __arraycount(emcfan_typical_addrs); i++)
		if (addr == emcfan_typical_addrs[i]) {
			r = true;
			break;
		}

	return(r);
}

static int
emcfan_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;

	if (iic_use_direct_match(ia, match, NULL, &match_result))
		return match_result;

	if (matchdebug) {
		printf("Looking at ia_addr: %x\n",ia->ia_addr);
	}

	/* Look to see if there is a device indirectly */

	if (! emcfan_check_i2c_addr(ia->ia_addr))
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

	error = emcfan_poke(ia->ia_tag, ia->ia_addr, matchdebug);
	iic_release_bus(ia->ia_tag, 0);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static int
emcfan_find_info(uint8_t product)
{
	for(int i = 0;i < __arraycount(emcfan_chip_infos); i++)
		if (product == emcfan_chip_infos[i].product_id)
			return(i);

	return(-1);
}

static const char *
emcfan_product_to_name(uint8_t info_index)
{
	KASSERT(info_index >= 0);

	return(emcfan_chip_infos[info_index].name);
}

int
emcfan_verify_sysctl(SYSCTLFN_ARGS)
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
emcfan_sysctl_init(struct emcfan_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;
	char pole_name[8];

	if ((error = sysctl_createv(&sc->sc_emcfanlog, 0, NULL, &cnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("emcfan controls"), NULL, 0, NULL, 0, CTL_HW,
	    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

#ifdef EMCFAN_DEBUG
	if ((error = sysctl_createv(&sc->sc_emcfanlog, 0, NULL, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Debug level"), emcfan_verify_sysctl, 0,
	    &sc->sc_emcfandebug, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
	    CTL_EOL)) != 0)
		return error;

#endif

	if (emcfan_chip_infos[sc->sc_info_index].family == EMCFAN_FAMILY_230X ||
	    emcfan_chip_infos[sc->sc_info_index].product_id == EMCFAN_PRODUCT_2103_1 ||
	    emcfan_chip_infos[sc->sc_info_index].product_id == EMCFAN_PRODUCT_2103_24 ||
	    emcfan_chip_infos[sc->sc_info_index].product_id == EMCFAN_PRODUCT_2104 ||
	    emcfan_chip_infos[sc->sc_info_index].product_id == EMCFAN_PRODUCT_2106) {
		for(int i=0;i < emcfan_chip_infos[sc->sc_info_index].num_tachs;i++) {
			snprintf(pole_name,sizeof(pole_name),"poles%d",i+1);
			if ((error = sysctl_createv(&sc->sc_emcfanlog, 0, NULL, &cnode,
			    CTLFLAG_READWRITE, CTLTYPE_INT, pole_name,
			    SYSCTL_DESCR("Number of poles"), emcfan_verify_sysctl, 0,
			    &sc->sc_num_poles[i], 0, CTL_HW, sysctlroot_num, CTL_CREATE,
			    CTL_EOL)) != 0)
				return error;
		}
	}
	if (emcfan_chip_infos[sc->sc_info_index].product_id == EMCFAN_PRODUCT_2103_1 ||
	    emcfan_chip_infos[sc->sc_info_index].product_id == EMCFAN_PRODUCT_2103_24) {
		if ((error = sysctl_createv(&sc->sc_emcfanlog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "ftach",
		    SYSCTL_DESCR("ftach frequency"), emcfan_verify_sysctl, 0,
		    &sc->sc_ftach, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
		    CTL_EOL)) != 0)
			return error;
	}
	if (emcfan_chip_infos[sc->sc_info_index].vin4_temp_zone) {
		if ((error = sysctl_createv(&sc->sc_emcfanlog, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_BOOL, "vin4",
		    SYSCTL_DESCR("Use VIN4 pin as a temperature sensor input"), NULL, 0,
		    &sc->sc_vin4_temp, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
		    CTL_EOL)) != 0)
			return error;
	}

	return 0;
}

static void
emcfan_attach(device_t parent, device_t self, void *aux)
{
	struct emcfan_sc *sc;
	struct i2c_attach_args *ia;
	uint8_t product_id, revision;
	int error;

	ia = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_opened = false;
	sc->sc_dying = false;
	sc->sc_ftach = 32000;
	sc->sc_vin4_temp = false;
	for(int i=0;i < EMCFAN_NUM_FANS;i++)
		sc->sc_num_poles[i] = 2;
	sc->sc_emcfandebug = 0;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);

	aprint_normal("\n");

	if ((sc->sc_sme = sysmon_envsys_create()) == NULL) {
		aprint_error_dev(self,
		    "Unable to create sysmon structure\n");
		sc->sc_sme = NULL;
		return;
	}

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		aprint_error_dev(self, "Could not acquire iic bus: %d\n",
		    error);
		goto out;
	}

	error = emcfan_read_registerr(sc->sc_tag, sc->sc_addr, EMCFAN_PRODUCT_ID, &product_id);
	if (error != 0) {
		aprint_error_dev(self, "Could not get the product id\n");
	} else {
		error = emcfan_read_registerr(sc->sc_tag, sc->sc_addr, EMCFAN_REVISION, &revision);
		if (error != 0) {
			aprint_error_dev(self, "Could not get the revision of the chip\n");
		}
	}

	iic_release_bus(sc->sc_tag, 0);
	if (error != 0) {
		aprint_error_dev(self, "Unable to setup device\n");
		goto out;
	}

	sc->sc_info_index = emcfan_find_info(product_id);
	if (sc->sc_info_index < 0) {
		aprint_error_dev(self, "Unknown product id: %02x\n",product_id);
		goto out;
	}

	if ((error = emcfan_sysctl_init(sc)) != 0) {
		sc->sc_emcfanlog = NULL;
		aprint_error_dev(self, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	for(int i=0;i < EMCFAN_NUM_SENSORS;i++)
		sc->sc_sensor_instances[i].sc_i_member = -1;

	int sensor_instance = 0;
	/* Set up the tachs */
	for(int i = 0;i < emcfan_chip_infos[sc->sc_info_index].num_tachs &&
		sensor_instance < EMCFAN_NUM_SENSORS;
	    i++) {
		snprintf(sc->sc_sensors[sensor_instance].desc,
		    sizeof(sc->sc_sensors[sensor_instance].desc),
		    "FAN %d",i+1);

		DPRINTF(sc, 2, ("%s: TACH registering fan sensor %d (%s)\n", __func__,
		    sensor_instance, sc->sc_sensors[sensor_instance].desc));

		sc->sc_sensor_instances[sensor_instance].sc_i_flags = 0;
		sc->sc_sensor_instances[sensor_instance].sc_i_member = i + 1;
		sc->sc_sensors[sensor_instance].units = ENVSYS_SFANRPM;
		sc->sc_sensors[sensor_instance].state = ENVSYS_SINVALID;

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[sensor_instance]);
		if (error) {
			aprint_error_dev(self,
			    "Unable to attach sensor %d: %d\n", i, error);
			goto out;
		}

		sc->sc_sensor_instances[sensor_instance].sc_i_envnum =
		    sc->sc_sensors[sensor_instance].sensor;

		DPRINTF(sc, 2, ("%s: TACH recorded sensor instance number %d->%d\n", __func__,
		    sensor_instance, sc->sc_sensor_instances[sensor_instance].sc_i_envnum));

		sensor_instance++;
	}

	/* Set up internal temperature sensor */
	if (emcfan_chip_infos[sc->sc_info_index].internal_temp_zone) {
		snprintf(sc->sc_sensors[sensor_instance].desc,
		    sizeof(sc->sc_sensors[sensor_instance].desc),
		    "internal temperature");

		DPRINTF(sc, 2, ("%s: IT registering internal temperature sensor %d (%s)\n", __func__,
		    sensor_instance, sc->sc_sensors[sensor_instance].desc));

		sc->sc_sensor_instances[sensor_instance].sc_i_flags = EMCFAN_INTERNAL_TEMP;
		sc->sc_sensor_instances[sensor_instance].sc_i_member = 1;
		sc->sc_sensors[sensor_instance].units = ENVSYS_STEMP;
		sc->sc_sensors[sensor_instance].state = ENVSYS_SINVALID;

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[sensor_instance]);
		if (error) {
			aprint_error_dev(self,
			    "Unable to attach internal sensor: %d\n", error);
			goto out;
		}

		sc->sc_sensor_instances[sensor_instance].sc_i_envnum =
		    sc->sc_sensors[sensor_instance].sensor;

		DPRINTF(sc, 2, ("%s: IT recorded sensor instance number %d->%d\n", __func__,
		    sensor_instance, sc->sc_sensor_instances[sensor_instance].sc_i_envnum));

		sensor_instance++;
	}

	/* Set up VIN4 temperature sensor */
	if (emcfan_chip_infos[sc->sc_info_index].vin4_temp_zone) {
		snprintf(sc->sc_sensors[sensor_instance].desc,
		    sizeof(sc->sc_sensors[sensor_instance].desc),
		    "VIN4 temperature");

		DPRINTF(sc, 2, ("%s: registering VIN4 temperature sensor %d (%s)\n", __func__,
		    sensor_instance, sc->sc_sensors[sensor_instance].desc));

		sc->sc_sensor_instances[sensor_instance].sc_i_flags = EMCFAN_VIN4_TEMP;
		sc->sc_sensor_instances[sensor_instance].sc_i_member = 1;
		sc->sc_sensors[sensor_instance].units = ENVSYS_STEMP;
		sc->sc_sensors[sensor_instance].state = ENVSYS_SINVALID;

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[sensor_instance]);
		if (error) {
			aprint_error_dev(self,
			    "Unable to attach VIN4 sensor: %d\n", error);
			goto out;
		}

		sc->sc_sensor_instances[sensor_instance].sc_i_envnum =
		    sc->sc_sensors[sensor_instance].sensor;

		DPRINTF(sc, 2, ("%s: VIN4 recorded sensor instance number %d->%d\n", __func__,
		    sensor_instance, sc->sc_sensor_instances[sensor_instance].sc_i_envnum));

		sensor_instance++;
	}

	/* Set up external temperature sensors */
	for(int i = 0;i < emcfan_chip_infos[sc->sc_info_index].num_external_temp_zones &&
		sensor_instance < EMCFAN_NUM_SENSORS;
	    i++) {
		snprintf(sc->sc_sensors[sensor_instance].desc,
		    sizeof(sc->sc_sensors[sensor_instance].desc),
		    "temperature zone %d",i+1);

		DPRINTF(sc, 2, ("%s: ET registering fan sensor %d (%s)\n", __func__,
		    sensor_instance, sc->sc_sensors[sensor_instance].desc));

		sc->sc_sensor_instances[sensor_instance].sc_i_flags = 0;
		sc->sc_sensor_instances[sensor_instance].sc_i_member = i + 1;
		sc->sc_sensors[sensor_instance].units = ENVSYS_STEMP;
		sc->sc_sensors[sensor_instance].state = ENVSYS_SINVALID;

		error = sysmon_envsys_sensor_attach(sc->sc_sme,
		    &sc->sc_sensors[sensor_instance]);
		if (error) {
			aprint_error_dev(self,
			    "Unable to attach sensor %d: %d\n", i, error);
			goto out;
		}

		sc->sc_sensor_instances[sensor_instance].sc_i_envnum =
		    sc->sc_sensors[sensor_instance].sensor;

		DPRINTF(sc, 2, ("%s: ET recorded sensor instance number %d->%d\n", __func__,
		    sensor_instance, sc->sc_sensor_instances[sensor_instance].sc_i_envnum));

		sensor_instance++;
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = emcfan_refresh;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
		return;
	}

	aprint_normal_dev(self, "Microchip Technology %s fan controller, "
	    "Revision: %02x\n",
	    emcfan_product_to_name(sc->sc_info_index),
	    revision);
	int e = emcfan_chip_infos[sc->sc_info_index].num_external_temp_zones;
	if (emcfan_chip_infos[sc->sc_info_index].vin4_temp_zone)
		e++;
	aprint_normal_dev(self, "Fans: %d, Tachometers: %d, Internal Temperature: %s, External Sensors: %d, GPIO: %s\n",
	    emcfan_chip_infos[sc->sc_info_index].num_fans,
	    emcfan_chip_infos[sc->sc_info_index].num_tachs,
	    (emcfan_chip_infos[sc->sc_info_index].internal_temp_zone) ? "Yes" : "No",
	    e,
	    (emcfan_chip_infos[sc->sc_info_index].num_gpio_pins > 0) ? "Yes" : "No");

	    if (emcfan_chip_infos[sc->sc_info_index].num_gpio_pins > 0)
		    emcfan_attach_gpio(sc, product_id);
	return;
out:
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}

/* The EMC-2101 is quite a bit different than the other EMC fan controllers.
 * Handle it differently.
 */

static void
emcfan_refresh_2101_tach(struct sysmon_envsys *sme, envsys_data_t *edata, int instance)
{
	struct emcfan_sc *sc = sme->sme_cookie;

	int error;
	uint8_t tach_high_reg;
	uint8_t tach_low_reg;
	uint8_t tach_high;
	uint8_t tach_low;

	switch(sc->sc_sensor_instances[instance].sc_i_member) {
	case 1:
		tach_high_reg = EMCFAN_2101_TACH_HIGH;
		tach_low_reg = EMCFAN_2101_TACH_LOW;
		break;
	default:
		panic("A 2101 can not have more than one tach\n");
		break;
	};

	DPRINTF(sc, 2, ("%s: dev=%s, instance=%d, sc_i_member=%d, tach_high_reg=0x%02X, tach_low_reg=0x%02X\n", __func__,
	    device_xname(sc->sc_dev), instance,
	    sc->sc_sensor_instances[instance].sc_i_member,
	    tach_high_reg, tach_low_reg));

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not acquire I2C bus: %d\n",__func__, error);
		return;
	}

	/* There is a interlock thing with the low and high bytes.  Read the
	 * low byte first.
	 */

	error = emcfan_read_register(sc, tach_low_reg, &tach_low);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not read tach low register: %d\n",__func__, error);
		iic_release_bus(sc->sc_tag, 0);
		return;
	}
	error = emcfan_read_register(sc, tach_high_reg, &tach_high);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not read tach high register: %d\n",__func__, error);
		iic_release_bus(sc->sc_tag, 0);
		return;
	}

	iic_release_bus(sc->sc_tag, 0);

	uint16_t count;
	count = tach_high << 8;
	count |= tach_low;

	DPRINTF(sc, 2, ("%s: instance=%d, tach_high=%d 0x%02X, tach_low=%d 0x%02X, count=%d\n", __func__,
	    instance, tach_high, tach_high, tach_low, tach_low, count));

	/* 0xffff indicates that the fan is not present, stopped / stalled
	 * or below the RPM that can be measured or the chip is not configured
	 * to read tach signals on the pin, but is being used for an alert
	 */

	if (count == 0xffff)
		return;

	/* The formula is:
	 *
	 * rpm = 5400000 / count
	 *
	 */

	uint64_t irpm;

	irpm = 5400000 / count;

	edata->value_cur = (uint32_t) irpm;
	edata->state = ENVSYS_SVALID;
}

static void
emcfan_refresh_210_346_230x_tach(int product_family, uint8_t product_id,
    struct sysmon_envsys *sme, envsys_data_t *edata, int instance)
{
	struct emcfan_sc *sc = sme->sme_cookie;

	int error;
	uint8_t tach_high_reg;
	uint8_t tach_low_reg;
	uint8_t fan_config_reg;
	uint8_t chip_config;
	uint8_t fan_config;
	uint8_t tach_high;
	uint8_t tach_low;
	int ftach = 32000;
	int edges;
	int poles;
	int m;

	if (product_family == EMCFAN_FAMILY_210X) {
		switch(sc->sc_sensor_instances[instance].sc_i_member) {
		case 1:
			fan_config_reg = EMCFAN_210_346_CONFIG_1;
			tach_high_reg = EMCFAN_210_346_TACH_1_HIGH;
			tach_low_reg = EMCFAN_210_346_TACH_1_LOW;
			break;
		case 2:
			fan_config_reg = EMCFAN_210_346_CONFIG_2;
			tach_high_reg = EMCFAN_210_346_TACH_2_HIGH;
			tach_low_reg = EMCFAN_210_346_TACH_2_LOW;
			break;
		default:
			panic("210X family do not know how to deal with member: %d\n",
			    sc->sc_sensor_instances[instance].sc_i_member);
			break;
		};
	} else {
		switch(sc->sc_sensor_instances[instance].sc_i_member) {
		case 1:
			fan_config_reg = EMCFAN_230X_CONFIG_1;
			tach_high_reg = EMCFAN_230X_TACH_1_HIGH;
			tach_low_reg = EMCFAN_230X_TACH_1_LOW;
			break;
		case 2:
			fan_config_reg = EMCFAN_230X_CONFIG_2;
			tach_high_reg = EMCFAN_230X_TACH_2_HIGH;
			tach_low_reg = EMCFAN_230X_TACH_2_LOW;
			break;
		case 3:
			fan_config_reg = EMCFAN_230X_CONFIG_3;
			tach_high_reg = EMCFAN_230X_TACH_3_HIGH;
			tach_low_reg = EMCFAN_230X_TACH_3_LOW;
			break;
		case 4:
			fan_config_reg = EMCFAN_230X_CONFIG_4;
			tach_high_reg = EMCFAN_230X_TACH_4_HIGH;
			tach_low_reg = EMCFAN_230X_TACH_4_LOW;
			break;
		case 5:
			fan_config_reg = EMCFAN_230X_CONFIG_5;
			tach_high_reg = EMCFAN_230X_TACH_5_HIGH;
			tach_low_reg = EMCFAN_230X_TACH_5_LOW;
			break;
		default:
			panic("230X family do not know how to deal with member: %d\n",
			    sc->sc_sensor_instances[instance].sc_i_member);
			break;
		};
	}

	DPRINTF(sc, 2, ("%s: dev=%s, instance=%d, sc_i_member=%d, fan_config_reg=0x%02X, tach_high_reg=0x%02X, tach_low_reg=0x%02X\n", __func__,
	    device_xname(sc->sc_dev), instance,
	    sc->sc_sensor_instances[instance].sc_i_member,
	    fan_config_reg, tach_high_reg, tach_low_reg));

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not acquire I2C bus: %d\n",__func__, error);
		return;
	}

	if (product_id == EMCFAN_PRODUCT_2103_1 ||
	    product_id == EMCFAN_PRODUCT_2103_24) {
		ftach = sc->sc_ftach;
	} else {
		chip_config = 0x00;
		if (product_family == EMCFAN_FAMILY_230X) {
			error = emcfan_read_register(sc, EMCFAN_CHIP_CONFIG, &chip_config);
		} else {
			if (product_id == EMCFAN_PRODUCT_2104 ||
			    product_id == EMCFAN_PRODUCT_2106) {
				error = emcfan_read_register(sc, EMCFAN_MUX_PINS, &chip_config);
			}
		}
		if (error) {
			device_printf(sc->sc_dev,"%s: could not read chip config: %d\n",__func__, error);
			iic_release_bus(sc->sc_tag, 0);
			return;
		}

		/* Figure out if there is an external clock involved */
		if (product_family == EMCFAN_FAMILY_230X) {
			if (chip_config & 0x02)
				ftach = 32000;
			else
				if (chip_config & 0x01)
					ftach = 32768;
				else
					ftach = 32000;
		} else {
			if (product_id == EMCFAN_PRODUCT_2104 ||
			    product_id == EMCFAN_PRODUCT_2106) {
				if (chip_config & 0x01)
					ftach = 32768;
				else
					ftach = 32000;
			}
		}

	}

	error = emcfan_read_register(sc, fan_config_reg, &fan_config);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not read fan config: %d\n",__func__, error);
		iic_release_bus(sc->sc_tag, 0);
		return;
	}

	/* There is a interlock thing with the low and high bytes.  Read the
	 * low byte first.
	 */

	error = emcfan_read_register(sc, tach_low_reg, &tach_low);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not read tach low register: %d\n",__func__, error);
		iic_release_bus(sc->sc_tag, 0);
		return;
	}
	error = emcfan_read_register(sc, tach_high_reg, &tach_high);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not read tach high register: %d\n",__func__, error);
		iic_release_bus(sc->sc_tag, 0);
		return;
	}

	iic_release_bus(sc->sc_tag, 0);

	/* Return early if the fan is stalled or not hooked up.  It might be better to look at
	 * the stalled fan status register, but that works differently depending on which chip
	 * you are looking at.
	 */

	if (product_family == EMCFAN_FAMILY_210X) {
		/* The datasheet is not at all clear as to what will be set in the low byte of the tach
		 * 0xc0, 0xe0 and 0xf0 all seem to depend on the minimum expected rpm and 0xf8 appears
		 * to mean that the fan is stalled in some way.
		 *
		 * Further to confuse matters, some chips may be able to adjust what invalid means.
		 * See the fan config register (0x4A) on the EMC2101 for an example of that.  We check
		 * tach_low here just in case these chips can do that too.
		 */
		if (tach_high == 0xff &&
		    (tach_low == 0xc0 || tach_low == 0xe0 ||
		    tach_low == 0xf0 || tach_low == 0xf8 ||
		    tach_low == 0xff))
			return;
	} else {
		/* The datasheet for the 230X family was a little clearer.  In that one, if the high byte is
		 * 0xff the tach reading is invalid.
		 */
		if (tach_high == 0xff)
			return;
	}

	/* Extract the M value, also known as the tach multiplier */
	m = fan_config & 0b01100000;
	m = m >> 5;

	DPRINTF(sc, 2, ("%s: fan_config=%d 0x%02X, raw m=%d 0x%02X\n",
	    __func__, fan_config, fan_config, m, m));

	m = (1 << m);

	/* Extract the number of configured edges */
	edges = fan_config & 0b00011000;
	edges = edges >> 3;

	DPRINTF(sc, 2, ("%s: fan_config=%d 0x%02X, raw edges=%d 0x%02X\n",
	    __func__, fan_config, fan_config, edges, edges));

	edges = ((edges + 1) * 2) + 1;

	/* Calculate the tach count, which needs to use bit weights */
        int count = 0;
	count = (tach_high << 5) | tach_low;

	/* The number of poles is a sysctl setting */
	poles = sc->sc_num_poles[sc->sc_sensor_instances[instance].sc_i_member - 1];

	DPRINTF(sc, 2, ("%s: instance=%d, ftach=%d, m=%d, edges=%d, poles=%d, tach_high=%d 0x%02X, tach_low=%d 0x%02X, count=%d\n", __func__,
	    instance, ftach, m, edges, poles, tach_high, tach_high, tach_low, tach_low, count));

	/* The formula is:
	 *
	 * rpm = 1/poles * ((edges - 1) / count * 1/m) * ftach * 60
	 *
	 * ftach is either 32.000khz or 32.768khz
	 *
	 */

	int64_t irpm;
	int ip1, ip2;
	int64_t ip3;

	ip1 = 10000 / poles;
	/*
	printf("poles: %d ; ip1: %d\n",poles,ip1);
	*/
	ip2 = 10000 / m;
	/*
	printf("m: %d ; ip2: %d\n",m,ip2);
	*/
	ip2 = count * ip2;
	/*
	printf("count: %d ; ip2: %d\n",count,ip2);
	*/
	ip3 = (int64_t)((edges - 1) * (int64_t)100000000000) / (int64_t)ip2;
	/*
	printf("edges: %d ; ip3: %d\n",edges,ip3);
	*/

	irpm = (ip1 * ip3 * ftach * 60) / 100000000000;

	edata->value_cur = (uint32_t) irpm;
	edata->state = ENVSYS_SVALID;
}

/* These two tables are taken from Appendix A in the 2104 and 2106 datasheet.
 * The index into the array is the ADC value and the value of the array is a
 * precomputed kelvin1000 (i.e celcius to kelvin * 1000) temperature.
 *
 * There are unusual holes as not all of the ADC values are present in the
 * *center* of the table these were made into xx.5 temperature values.
 *
 * Another quirk is that the table in the datasheets have multiple temperatures
 * for a particular ADC.  This behavior seems more common on the edges of the
 * table and may make sense.  What these tables do, is just take the first
 * temperature for any ADC value.
 *
 */

#define EMCFAN_VIN_NO_TEMP -1

static const int32_t emcfan_vin_temps[] = {
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	463150,
	461150,
	459150,
	457150,
	455150,
	453150,
	451150,
	450150,
	448150,
	446150,
	445150,
	443150,
	441150,
	440150,
	438150,
	437150,
	435150,
	434150,
	433150,
	431150,
	430150,
	429150,
	427150,
	426150,
	425150,
	424150,
	423150,
	421150,
	420150,
	419150,
	418150,
	417150,
	416150,
	415150,
	414150,
	413150,
	412150,
	411150,
	410150,
	409150,
	408150,
	407150,
	406150,
	405150,
	404150,
	403150,
	402150,
	398150,
	397150,
	396150,
	395150,
	394650,
	394150,
	393150,
	392150,
	391650,
	391150,
	390150,
	389150,
	388650,
	388150,
	387150,
	386650,
	386150,
	385150,
	384150,
	383650,
	383150,
	382150,
	381650,
	381150,
	380150,
	379650,
	379150,
	378150,
	377650,
	377150,
	376650,
	376150,
	375150,
	374650,
	374150,
	373150,
	372650,
	372150,
	371650,
	371150,
	370150,
	369650,
	369150,
	368650,
	368150,
	367150,
	366650,
	366150,
	365650,
	365150,
	364150,
	363650,
	363150,
	362650,
	362150,
	361650,
	361150,
	360150,
	359650,
	359150,
	358150,
	357650,
	357150,
	356650,
	356150,
	355650,
	355150,
	354150,
	353650,
	353150,
	352650,
	352150,
	351650,
	351150,
	350650,
	350150,
	349150,
	348650,
	348150,
	347650,
	347150,
	346150,
	345650,
	345150,
	344650,
	344150,
	343650,
	343150,
	342150,
	341650,
	341150,
	340650,
	340150,
	339150,
	338650,
	338150,
	337650,
	337150,
	336150,
	335650,
	335150,
	334650,
	334150,
	333150,
	332650,
	332150,
	331150,
	330650,
	330150,
	329650,
	329150,
	328150,
	327650,
	327150,
	326150,
	325650,
	325150,
	324150,
	323650,
	323150,
	322150,
	321150,
	320650,
	320150,
	319150,
	318650,
	318150,
	317150,
	316150,
	315150,
	314650,
	314150,
	313150,
	312150,
	311150,
	310650,
	310150,
	309150,
	308150,
	307150,
	306150,
	305150,
	304150,
	303150,
	302150,
	301150,
	300150,
	299150,
	298150,
	297150,
	296150,
	295150,
	293150,
	292150,
	291150,
	290150,
	288150,
	287150,
	285150,
	283150,
	282150,
	280150,
	278150,
	276150,
	273150,
	271150,
	268150,
	265150,
	262150,
	259150,
	255150,
	250150,
	244150,
	236150,
	229150,
	228150,
	EMCFAN_VIN_NO_TEMP
};

static const int32_t emcfan_vin_temps_i[] = {
	228150,
	229150,
	236150,
	244150,
	250150,
	255150,
	259150,
	262150,
	265150,
	268150,
	271150,
	273150,
	276150,
	278150,
	280150,
	281150,
	283150,
	285150,
	286150,
	288150,
	289150,
	291150,
	292150,
	293150,
	295150,
	296150,
	297150,
	298150,
	299150,
	300150,
	301150,
	302150,
	303150,
	304150,
	305150,
	306150,
	307150,
	308150,
	309150,
	310150,
	310650,
	311150,
	312150,
	313150,
	314150,
	314650,
	315150,
	316150,
	317150,
	317650,
	318150,
	319150,
	320150,
	320650,
	321150,
	322150,
	323150,
	323650,
	324150,
	325150,
	325650,
	326150,
	327150,
	327650,
	328150,
	329150,
	329650,
	330150,
	330650,
	331150,
	332150,
	332650,
	333150,
	334150,
	334650,
	335150,
	335650,
	336150,
	337150,
	337650,
	338150,
	338650,
	339150,
	340150,
	340650,
	341150,
	341650,
	342150,
	343150,
	343650,
	344150,
	344650,
	345150,
	345650,
	346150,
	347150,
	347650,
	348150,
	348650,
	349150,
	350150,
	350650,
	351150,
	351650,
	352150,
	352650,
	353150,
	353650,
	354150,
	355150,
	355650,
	356150,
	356650,
	357150,
	357650,
	358150,
	359150,
	359650,
	360150,
	360650,
	361150,
	362150,
	362650,
	363150,
	363650,
	364150,
	365150,
	365650,
	366150,
	366650,
	367150,
	368150,
	368650,
	369150,
	369650,
	370150,
	371150,
	371650,
	372150,
	372650,
	373150,
	374150,
	374650,
	375150,
	376150,
	376650,
	377150,
	377650,
	378150,
	379150,
	379650,
	380150,
	381150,
	381650,
	382150,
	383150,
	383650,
	384150,
	385150,
	386150,
	386650,
	387150,
	388150,
	388650,
	389150,
	390150,
	391150,
	391650,
	392150,
	393150,
	394150,
	394650,
	395150,
	396150,
	397150,
	398150,
	402150,
	403150,
	404150,
	405150,
	406150,
	407150,
	408150,
	409150,
	410150,
	411150,
	412150,
	413150,
	414150,
	415150,
	416150,
	417150,
	418150,
	419150,
	420150,
	421150,
	423150,
	424150,
	425150,
	426150,
	427150,
	429150,
	430150,
	431150,
	433150,
	434150,
	435150,
	437150,
	438150,
	440150,
	441150,
	443150,
	445150,
	446150,
	448150,
	450150,
	451150,
	453150,
	455150,
	457150,
	459150,
	461150,
	463150,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP,
	EMCFAN_VIN_NO_TEMP
};

static void
emcfan_refresh_temp(int product_family, uint8_t product_id,
    struct sysmon_envsys *sme, envsys_data_t *edata, int instance)
{
	struct emcfan_sc *sc = sme->sme_cookie;

	int error;
	uint8_t temp_config;
	uint8_t raw_temp_config_3;
	uint8_t temp_config_3;
	uint8_t temp_high;
	uint8_t temp_low;
	uint8_t external_temp_high_reg;
	uint8_t external_temp_low_reg;
	bool is_internal = false;
	bool is_vin4 = false;
	bool using_apd = false;
	bool using_vin = false;
	bool inverted = false;

	is_internal = sc->sc_sensor_instances[instance].sc_i_flags & EMCFAN_INTERNAL_TEMP;
	is_vin4 = sc->sc_sensor_instances[instance].sc_i_flags & EMCFAN_VIN4_TEMP;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		device_printf(sc->sc_dev,"%s: could not acquire I2C bus: %d\n",__func__, error);
		return;
	}

	if (is_internal) {
		/* There might be a data interlock thing going on with the high and low
		 * registers, to make sure, read the high one first.  This works in the
		 * opposite of the tach.
		 */
		error = emcfan_read_register(sc, EMCFAN_INTERNAL_TEMP_HIGH, &temp_high);
		if (error) {
			device_printf(sc->sc_dev,"%s: could not read internal temp high: %d\n",__func__, error);
			iic_release_bus(sc->sc_tag, 0);
			return;
		}
		/* The 2101 does not have fractions on the internal temperature sensor */
		if (product_id == EMCFAN_PRODUCT_2101) {
			temp_low = 0;
		} else {
			error = emcfan_read_register(sc, EMCFAN_INTERNAL_TEMP_LOW, &temp_low);
			if (error) {
				device_printf(sc->sc_dev,"%s: could not read internal temp low: %d\n",__func__, error);
				iic_release_bus(sc->sc_tag, 0);
				return;
			}
		}
	} else {
		if (is_vin4) {
			if (sc->sc_vin4_temp) {
				using_vin = true;

				error = emcfan_read_register(sc, EMCFAN_CHIP_CONFIG, &temp_config);
				if (error) {
					device_printf(sc->sc_dev,"%s: could not read chip config register: %d\n",__func__, error);
					iic_release_bus(sc->sc_tag, 0);
					return;
				}
				inverted = temp_config & 0x80;

				error = emcfan_read_register(sc, EMCFAN_VIN4_VOLTAGE, &temp_high);
				if (error) {
					device_printf(sc->sc_dev,"%s: could not read external temp high: %d\n",__func__, error);
					iic_release_bus(sc->sc_tag, 0);
					return;
				}
			} else {
				iic_release_bus(sc->sc_tag, 0);
				return;
			}
		} else {
			/* The 2101 has its external sensor on a different set of registers
			 * than the rest.
			 */
			if (product_id == EMCFAN_PRODUCT_2101) {
				error = emcfan_read_register(sc, EMCFAN_2101_EXTERNAL_TEMP_LOW, &temp_low);
				if (error) {
					device_printf(sc->sc_dev,"%s: could not read external temp low: %d\n",__func__, error);
					iic_release_bus(sc->sc_tag, 0);
					return;
				}
				error = emcfan_read_register(sc, EMCFAN_2101_EXTERNAL_TEMP_HIGH, &temp_high);
				if (error) {
					device_printf(sc->sc_dev,"%s: could not read external temp high: %d\n",__func__, error);
					iic_release_bus(sc->sc_tag, 0);
					return;
				}
			} else {
				switch(sc->sc_sensor_instances[instance].sc_i_member) {
				case 1:
					external_temp_high_reg = EMCFAN_EXTERNAL_1_TEMP_HIGH;
					external_temp_low_reg = EMCFAN_EXTERNAL_1_TEMP_LOW;
					break;
				case 2:
					external_temp_high_reg = EMCFAN_EXTERNAL_2_TEMP_HIGH;
					external_temp_low_reg = EMCFAN_EXTERNAL_2_TEMP_LOW;
					break;
				case 3:
					external_temp_high_reg = EMCFAN_EXTERNAL_3_TEMP_HIGH;
					external_temp_low_reg = EMCFAN_EXTERNAL_3_TEMP_LOW;
					break;
				case 4:
					external_temp_high_reg = EMCFAN_EXTERNAL_4_TEMP_HIGH;
					external_temp_low_reg = EMCFAN_EXTERNAL_4_TEMP_LOW;
					break;
				default:
					panic("Unknown member: %d\n",
					    sc->sc_sensor_instances[instance].sc_i_member);
					break;
				};

				/* The 2103-2, 2103-4, 2104 and 2106 can use APD mode.  This is a method
				 * of using two sensors in parallel on a single set of pins.  The way one
				 * wires this up is in the datasheets for the chip.
				 */

				if (product_id == EMCFAN_PRODUCT_2103_24 ||
				    product_id == EMCFAN_PRODUCT_2104 ||
				    product_id == EMCFAN_PRODUCT_2106) {
					error = emcfan_read_register(sc, EMCFAN_CHIP_CONFIG, &temp_config);
					if (error) {
						device_printf(sc->sc_dev,"%s: could not read chip config register: %d\n",__func__, error);
						iic_release_bus(sc->sc_tag, 0);
						return;
					}

					using_apd = temp_config & 0x01;
				}

				/* The 2104, 2105 and 2106 has some other special abilities, such as being
				 * able to use thermistors.
				 */

				if (product_id == EMCFAN_PRODUCT_2104 ||
				    product_id == EMCFAN_PRODUCT_2106) {
					error = emcfan_read_register(sc, EMCFAN_TEMP_CONFIG_3, &raw_temp_config_3);
					if (error) {
						device_printf(sc->sc_dev,"%s: could not read temperature config register: %d\n",__func__, error);
						iic_release_bus(sc->sc_tag, 0);
						return;
					}
					switch(sc->sc_sensor_instances[instance].sc_i_member) {
					case 1:
						temp_config_3 = raw_temp_config_3 & 0x03;
						break;
					case 2:
						temp_config_3 = raw_temp_config_3 >> 2;
						temp_config_3 = temp_config_3 & 0x03;
						break;
					case 3:
						temp_config_3 = raw_temp_config_3 >> 4;
						temp_config_3 = temp_config_3 & 0x03;
						break;
					default:
						temp_config_3 = 0;
						break;
					};

					using_vin = temp_config_3 & 0x02;
					inverted = temp_config_3 & 0x01;


					/* there is a strange situation if sensor 3 is being used as a VIN
					 * sensor, then sensor 4 is not available at all.  Note that this
					 * sensor 4 is *NOT* the sensor that might be attached to the VIN4
					 * pin.
					 */

					if (sc->sc_sensor_instances[instance].sc_i_member == 4 &&
					    raw_temp_config_3 & 0x20) {
						iic_release_bus(sc->sc_tag, 0);
						return;
					}
				}

				if (product_id == EMCFAN_PRODUCT_2103_24) {
					/* The anti-parallel mode, apd, must be enabled before sensor 3 will
					 * be available.
					 */
					if (!using_apd &&
					    sc->sc_sensor_instances[instance].sc_i_member == 3) {
						iic_release_bus(sc->sc_tag, 0);
						return;
					}
				}

				if (product_id == EMCFAN_PRODUCT_2104 ||
				    product_id == EMCFAN_PRODUCT_2106) {
					/* The anti-parallel mode, apd, must be enabled before sensor 4 will
					 * be available.  This, of course, might conflict if sensor 3 is a VIN
					 * sensor.  Don't do that....
					 */
					if (!using_apd &&
					    sc->sc_sensor_instances[instance].sc_i_member == 4) {
						iic_release_bus(sc->sc_tag, 0);
						return;
					}
				}

				/* There is a data interlock thing going on with the high and low
				 * registers, but it works the opposite from the tach.
				 */

				error = emcfan_read_register(sc, external_temp_high_reg, &temp_high);
				if (error) {
					device_printf(sc->sc_dev,"%s: could not read external temp high: %d\n",__func__, error);
					iic_release_bus(sc->sc_tag, 0);
					return;
				}
				if (using_vin) {
					temp_low = 0;
				} else {
					error = emcfan_read_register(sc, external_temp_low_reg, &temp_low);
					if (error) {
						device_printf(sc->sc_dev,"%s: could not read external temp low: %d\n",__func__, error);
						iic_release_bus(sc->sc_tag, 0);
						return;
					}
				}
			}
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	/* It appears that on the 2101, if the high byte is 0x7f and the low byte is 0,\
	 * then there is a fault or no sensor.
	 */

	if (product_id == EMCFAN_PRODUCT_2101 &&
	    !is_internal) {
		if (temp_high == 0x7f &&
		    temp_low == 0) {
			return;
		}
	}

	/* For everyone else, if the external sensor read 0x80 on the high byte and
	 * the fraction is 0, then there is a fault or no sensor.
	 */

	if (!is_internal && !using_vin) {
		if (temp_high == 0x80 &&
		    (temp_low >> 5) == 0x00) {
			return;
		    }
	}

	int32_t kelvin1000 = 0;
	int32_t frac = 0;
	uint8_t tl;

	if (!using_vin) {
		kelvin1000 = (int8_t)temp_high * 1000;
		tl = temp_low >> 5;
		if (temp_high & 0x80) {
			tl = (~tl) & 0x07;
			tl++;
		}
		frac = 125 * tl;
		if (temp_high & 0x80) {
			kelvin1000 -= frac;
		} else {
			kelvin1000 += frac;
		}
		kelvin1000 += 273150;
	} else {
		int32_t vin1000 = EMCFAN_VIN_NO_TEMP;

		if (inverted) {
			if (emcfan_vin_temps_i[temp_high] != EMCFAN_VIN_NO_TEMP) {
				vin1000 = emcfan_vin_temps_i[temp_high];
			}
		} else {
			if (emcfan_vin_temps[temp_high] != EMCFAN_VIN_NO_TEMP) {
				vin1000 = emcfan_vin_temps[temp_high];
			}
		}

		if (vin1000 != EMCFAN_VIN_NO_TEMP)
			kelvin1000 = vin1000;
		else
			return;
	}

	edata->value_cur = (uint32_t) kelvin1000 * 1000;
	edata->state = ENVSYS_SVALID;
}

static void
emcfan_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct emcfan_sc *sc = sme->sme_cookie;
	int instance = -1;

	/* Hunt down the instance for this sensor.
	 *
	 * The type is held in sc_sensors, but the actual hardware
	 * instance is held in sc_sensor_instances in the member
	 * field.  It would be nice if the edata structure had a
	 * field that could be used as an opaque value.
	 */
	for(int i = 0;i < EMCFAN_NUM_SENSORS;i++)
		if (sc->sc_sensor_instances[i].sc_i_envnum == edata->sensor) {
			instance = i;
			break;
		}

	KASSERT(instance > -1);

	DPRINTF(sc, 2, ("%s: using sensor instance %d\n", __func__,
	    instance));

	edata->state = ENVSYS_SINVALID;

	/* Unlike manor of the refresh functions in other drivers, this
	 * one will select the sensor based upon the type and instance.
	 *
	 * Due to the fact that the order will vary depending on which
	 * chip you are using.
	 */

	switch(edata->units) {
	case ENVSYS_SFANRPM:
		switch(emcfan_chip_infos[sc->sc_info_index].family) {
		case EMCFAN_FAMILY_210X:
			switch(emcfan_chip_infos[sc->sc_info_index].product_id) {
			case EMCFAN_PRODUCT_2101:
				emcfan_refresh_2101_tach(sme, edata, instance);
				break;
				/* 2103, 2104 and 2106 use nearly the same algorithm as the 230x family */
			default:
				emcfan_refresh_210_346_230x_tach(emcfan_chip_infos[sc->sc_info_index].family,
				    emcfan_chip_infos[sc->sc_info_index].product_id,
				    sme, edata, instance);
				break;
			};
			break;
		case EMCFAN_FAMILY_230X:
			emcfan_refresh_210_346_230x_tach(emcfan_chip_infos[sc->sc_info_index].family,
			    emcfan_chip_infos[sc->sc_info_index].product_id,
			    sme, edata, instance);
			break;
		default:
			panic("Unknown family: %d\n",emcfan_chip_infos[sc->sc_info_index].family);
			break;
		}
		break;
	case ENVSYS_STEMP:
		emcfan_refresh_temp(emcfan_chip_infos[sc->sc_info_index].family,
		    emcfan_chip_infos[sc->sc_info_index].product_id,
		    sme, edata, instance);
		break;
	default:
		panic("Unknown edata units value: %d\n",edata->units);
		break;
	};
}

static int
emcfanopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct emcfan_sc *sc;

	sc = device_lookup_private(&emcfan_cd, minor(dev));
	if (!sc)
		return ENXIO;

	if (sc->sc_opened)
		return EBUSY;

	mutex_enter(&sc->sc_mutex);
	sc->sc_opened = true;
	mutex_exit(&sc->sc_mutex);

	return 0;
}

static int
emcfanread(dev_t dev, struct uio *uio, int flags)
{
	struct emcfan_sc *sc;
	int error;

	if ((sc = device_lookup_private(&emcfan_cd, minor(dev))) == NULL)
		return ENXIO;

	/* We do not make this an error.  There is nothing wrong with running
	 * off the end here, just return EOF.
	 */
	if (uio->uio_offset > 0xff)
		return 0;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	while (uio->uio_resid &&
	    uio->uio_offset <= 0xff &&
	    !sc->sc_dying) {
		uint8_t buf;
		int reg_addr = uio->uio_offset;

		if ((error = emcfan_read_register(sc, reg_addr, &buf)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(sc->sc_dev,
			    "%s: read failed at 0x%02x: %d\n",
			    __func__, reg_addr, error);
			return error;
		}

		if (sc->sc_dying)
			break;

		if ((error = uiomove(&buf, 1, uio)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			return error;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	if (sc->sc_dying) {
		return EIO;
	}

	return 0;
}

static int
emcfanwrite(dev_t dev, struct uio *uio, int flags)
{
	struct emcfan_sc *sc;
	int error;

	if ((sc = device_lookup_private(&emcfan_cd, minor(dev))) == NULL)
		return ENXIO;

	/* Same thing as read, this is not considered an error */
	if (uio->uio_offset > 0xff)
		return 0;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	while (uio->uio_resid &&
	    uio->uio_offset <= 0xff &&
	    !sc->sc_dying) {
		uint8_t buf;
		int reg_addr = uio->uio_offset;

		if ((error = uiomove(&buf, 1, uio)) != 0)
			break;

		if (sc->sc_dying)
			break;

		if ((error = emcfan_write_register(sc, (uint8_t)reg_addr, buf)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(sc->sc_dev,
			    "%s: write failed at 0x%02x: %d\n",
			    __func__, reg_addr, error);
			return error;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	if (sc->sc_dying) {
		return EIO;
	}

	return error;
}

static int
emcfanclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct emcfan_sc *sc;

	sc = device_lookup_private(&emcfan_cd, minor(dev));

	mutex_enter(&sc->sc_mutex);
	sc->sc_opened = false;
	mutex_exit(&sc->sc_mutex);

	return(0);
}

static int
emcfan_detach(device_t self, int flags)
{
	int err;
	struct emcfan_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);
	sc->sc_dying = true;

	err = config_detach_children(self, flags);
	if (err)
		return err;

	/* Remove the sysctl tree */
	if (sc->sc_emcfanlog != NULL)
		sysctl_teardown(&sc->sc_emcfanlog);

	/* Remove the sensors */
	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}

	mutex_exit(&sc->sc_mutex);

	mutex_destroy(&sc->sc_mutex);

	return 0;
}

int
emcfan_activate(device_t self, enum devact act)
{
	struct emcfan_sc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

/* --- GPIO --- */

static int
emcfan_current_gpio_flags(struct emcfan_sc *sc,
    uint8_t product_id,
    int pin)
{
	int error = 0;
	int f = 0;
	uint8_t mux_reg = 0;
	uint8_t dir_reg;
	uint8_t out_config;
	uint8_t pin_mask;
	uint8_t pin_maska1;
	uint8_t pin_maska2;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		return 0;
	}

	if (product_id != EMCFAN_PRODUCT_2103_24) {
		error = emcfan_read_register(sc, EMCFAN_MUX_PINS, &mux_reg);
		if (error != 0) {
			return 0;
		}
	}
	error = emcfan_read_register(sc, EMCFAN_DIR_PINS, &dir_reg);
	if (error != 0) {
		return 0;
	}
	error = emcfan_read_register(sc, EMCFAN_OUTPUT_PIN_CONFIG, &out_config);
	if (error != 0) {
		return 0;
	}
	iic_release_bus(sc->sc_tag, 0);

	pin_mask = 1 << pin;

	if (product_id != EMCFAN_PRODUCT_2103_24) {
		if (pin <= 4) {
			if (pin <= 2) {
				if ((mux_reg & pin_mask) == 0)
					f = GPIO_PIN_ALT0;
			} else {
				if (pin == 3) {
					pin_maska1 = 0x08;
					pin_maska2 = 0x10;
				} else {
					pin_maska1 = 0x20;
					pin_maska2 = 0x40;
				}
				if (mux_reg & pin_maska1 &&
				    mux_reg & pin_maska2) {
					f = GPIO_PIN_ALT1;
				} else {
					if (((mux_reg & pin_maska1) == 0) &&
					    ((mux_reg & pin_maska2) == 0)) {
						f = GPIO_PIN_ALT0;
					}
				}
			}
		}
	}

	if (f == 0) {
		if (dir_reg & pin_mask) {
			f = GPIO_PIN_OUTPUT;
		} else {
			f = GPIO_PIN_INPUT;
		}

		if (out_config & pin_mask) {
			f |= GPIO_PIN_PUSHPULL;
		} else {
			f |= GPIO_PIN_OPENDRAIN;
		}
	}

	return f;
}

static int
emcfan_gpio_pin_read(void *arg, int pin)
{
	struct emcfan_sc *sc = arg;
	int error = 0;
	int r = GPIO_PIN_LOW;
	uint8_t input_reg;
	uint8_t pin_mask;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (!error) {
		error = emcfan_read_register(sc, EMCFAN_PINS_INPUT, &input_reg);
		if (!error) {
			pin_mask = 1 << pin;
			if (input_reg & pin_mask)
				r = GPIO_PIN_HIGH;
		}

	}
	iic_release_bus(sc->sc_tag, 0);

	return r;
}

static void
emcfan_gpio_pin_write(void *arg, int pin, int value)
{
	struct emcfan_sc *sc = arg;
	int error = 0;
	uint8_t output_reg;
	uint8_t pin_mask;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (!error) {
		error = emcfan_read_register(sc, EMCFAN_PINS_OUTPUT, &output_reg);
		if (!error) {
			pin_mask = 1 << pin;

			if (value == 0) {
				pin_mask = ~pin_mask;
				output_reg &= pin_mask;
			} else {
				output_reg |= pin_mask;
			}
			emcfan_write_register(sc, EMCFAN_PINS_OUTPUT, output_reg);
		}

	}
	iic_release_bus(sc->sc_tag, 0);
}

static void
emcfan_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct emcfan_sc *sc = arg;
	int error = 0;
	uint8_t product_id = emcfan_chip_infos[sc->sc_info_index].product_id;
	uint8_t pin_mask = 1 << pin;
	uint8_t pin_maska1;
	uint8_t pin_maska2;
	uint8_t mux_reg = 0;
	uint8_t dir_reg;
	uint8_t out_config;

	error = iic_acquire_bus(sc->sc_tag, 0);
	if (error) {
		return;
	}

	if (product_id != EMCFAN_PRODUCT_2103_24) {
		error = emcfan_read_register(sc, EMCFAN_MUX_PINS, &mux_reg);
		if (error != 0) {
			return;
		}
	}
	error = emcfan_read_register(sc, EMCFAN_DIR_PINS, &dir_reg);
	if (error != 0) {
		return;
	}
	error = emcfan_read_register(sc, EMCFAN_OUTPUT_PIN_CONFIG, &out_config);
	if (error != 0) {
		return;
	}
	iic_release_bus(sc->sc_tag, 0);

	if (flags & GPIO_PIN_ALT0 ||
	    flags & GPIO_PIN_ALT1) {
		if (product_id != EMCFAN_PRODUCT_2103_24) {
			if (pin <= 4) {
				if (pin <= 2) {
					mux_reg &= ~pin_mask;
				} else {
					if (pin == 3) {
						pin_maska2 = 0x18;
					} else {
						pin_maska2 = 0x60;
					}
					if (flags & GPIO_PIN_ALT0) {
						mux_reg &= ~pin_maska2;
					} else {
						if (flags & GPIO_PIN_ALT1) {
							mux_reg |= pin_maska2;
						}
					}
				}
				emcfan_write_register(sc, EMCFAN_MUX_PINS, mux_reg);
			}
		}
	} else {
		if (product_id != EMCFAN_PRODUCT_2103_24) {
			if (pin <= 4) {
				if (pin <= 2) {
					mux_reg |= pin_mask;
				} else {
					if (pin == 3) {
						pin_maska1 = 0x08;
						pin_maska2 = 0x18;
					} else {
						pin_maska1 = 0x20;
						pin_maska2 = 0x60;
					}
					mux_reg &= ~pin_maska2;
					mux_reg |= pin_maska1;
				}
				emcfan_write_register(sc, EMCFAN_MUX_PINS, mux_reg);
			}
		}

		if (flags & GPIO_PIN_OPENDRAIN) {
			out_config &= ~pin_mask;
		} else {
			if (flags & GPIO_PIN_PUSHPULL)
				out_config |= pin_mask;
		}
		emcfan_write_register(sc, EMCFAN_OUTPUT_PIN_CONFIG, out_config);

		if (flags & GPIO_PIN_INPUT) {
			dir_reg &= ~pin_mask;
		} else {
			if (flags & GPIO_PIN_OUTPUT)
				dir_reg |= pin_mask;
		}
		emcfan_write_register(sc, EMCFAN_DIR_PINS, dir_reg);
	}
}

static void
emcfan_attach_gpio(struct emcfan_sc *sc, uint8_t product_id)
{
	struct gpiobus_attach_args gba;

	for(int i = 0; i < emcfan_chip_infos[sc->sc_info_index].num_gpio_pins;i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = emcfan_chip_infos[sc->sc_info_index].gpio_pin_ability[i];
		sc->sc_gpio_pins[i].pin_flags = emcfan_current_gpio_flags(sc, emcfan_chip_infos[sc->sc_info_index].product_id, i);
		sc->sc_gpio_pins[i].pin_intrcaps = 0;
		strncpy(sc->sc_gpio_pins[i].pin_defname,
		    emcfan_chip_infos[sc->sc_info_index].gpio_names[i],
		    strlen(emcfan_chip_infos[sc->sc_info_index].gpio_names[i]) + 1);
	}

	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = emcfan_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = emcfan_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = emcfan_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = emcfan_chip_infos[sc->sc_info_index].num_gpio_pins;

	sc->sc_gpio_dev = config_found(sc->sc_dev, &gba, gpiobus_print, CFARGS(.iattr = "gpiobus"));

	return;
}

MODULE(MODULE_CLASS_DRIVER, emcfan, "iic,sysmon_envsys,gpio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
emcfan_modcmd(modcmd_t cmd, void *opaque)
{
	int error;
#ifdef _MODULE
	int bmaj = -1, cmaj = -1;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = devsw_attach("emcfan", NULL, &bmaj,
		    &emcfan_cdevsw, &cmaj);
		if (error) {
			aprint_error("%s: unable to attach devsw\n",
			    emcfan_cd.cd_name);
			return error;
		}

		error = config_init_component(cfdriver_ioconf_emcfan,
		    cfattach_ioconf_emcfan, cfdata_ioconf_emcfan);
		if (error) {
			aprint_error("%s: unable to init component\n",
			    emcfan_cd.cd_name);
			devsw_detach(NULL, &emcfan_cdevsw);
		}
		return error;
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_emcfan,
		      cfattach_ioconf_emcfan, cfdata_ioconf_emcfan);
		devsw_detach(NULL, &emcfan_cdevsw);
		return error;
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
