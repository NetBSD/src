/*	$NetBSD: zapm.c,v 1.4.2.1 2009/05/13 17:18:51 jym Exp $	*/
/*	$OpenBSD: zaurus_apm.c,v 1.13 2006/12/12 23:14:28 dim Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
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
__KERNEL_RCSID(0, "$NetBSD: zapm.c,v 1.4.2.1 2009/05/13 17:18:51 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/selinfo.h> /* XXX: for apm_softc that is exposed here */

#include <dev/hpc/apm/apmvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <machine/config_hook.h>

#include <zaurus/dev/scoopvar.h>
#include <zaurus/dev/zsspvar.h>
#include <zaurus/zaurus/zaurus_reg.h>
#include <zaurus/zaurus/zaurus_var.h>

#ifdef APMDEBUG
#define DPRINTF(x)	printf x
#else
#define	DPRINTF(x)	do { } while (/*CONSTCOND*/0)
#endif

struct zapm_softc {
	device_t sc_dev;
	void *sc_apmdev;

	struct callout sc_cyclic_poll;
	struct callout sc_discharge_poll;
	struct timeval sc_lastbattchk;
	volatile int suspended;
	volatile int charging;
	volatile int discharging;
	int battery_volt;
	int battery_full_cnt;

	/* GPIO pin */
	int sc_ac_detect_pin;
	int sc_batt_cover_pin;
	int sc_charge_comp_pin;

	/* machine-independent part */
	volatile u_int events;
	volatile int power_state;
	volatile int battery_state;
	volatile int ac_state;
	config_hook_tag sc_standby_hook;
	config_hook_tag sc_suspend_hook;
	config_hook_tag sc_battery_hook;
	config_hook_tag sc_ac_hook;
	int battery_life;
	int minutes_left;
};

static int	zapm_match(device_t, cfdata_t, void *);
static void	zapm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zapm, sizeof(struct zapm_softc),
    zapm_match, zapm_attach, NULL, NULL);

static int	zapm_hook(void *, int, long, void *);
static void     zapm_disconnect(void *);
static void     zapm_enable(void *, int);
static int      zapm_set_powstate(void *, u_int, u_int);
static int      zapm_get_powstat(void *, u_int, struct apm_power_info *);
static int      zapm_get_event(void *, u_int *, u_int *);
static void     zapm_cpu_busy(void *);
static void     zapm_cpu_idle(void *);
static void     zapm_get_capabilities(void *, u_int *, u_int *);

static struct apm_accessops zapm_accessops = {
	zapm_disconnect,
	zapm_enable,
	zapm_set_powstate,
	zapm_get_powstat,
	zapm_get_event,
	zapm_cpu_busy,
	zapm_cpu_idle,
	zapm_get_capabilities,
};

static int	zapm_acintr(void *);
static int	zapm_bcintr(void *);
static void	zapm_cyclic(void *);
static void	zapm_poll(void *);
static void	zapm_poll1(void *, int);

/* battery-related GPIO pins */
#define GPIO_AC_IN_C3000	115	/* 0=AC connected */
#define GPIO_CHRG_CO_C3000	101	/* 1=battery full */
#define GPIO_BATT_COVER_C3000	90	/* 0=unlocked */

/* Cyclic timer value */
#define	CYCLIC_TIME	(60 * hz)	/* 60s */

static int
zapm_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!ZAURUS_ISC3000)
		return 0;
	return 1;
}

static void
zapm_attach(device_t parent, device_t self, void *aux)
{
	struct zapm_softc *sc = device_private(self);
	struct apmdev_attach_args aaa;

	sc->sc_dev = self;

	aprint_normal(": pseudo power management module\n");
	aprint_naive("\n");

	/* machine-depent part */
	callout_init(&sc->sc_cyclic_poll, 0);
	callout_setfunc(&sc->sc_cyclic_poll, zapm_cyclic, sc);
	callout_init(&sc->sc_discharge_poll, 0);
	callout_setfunc(&sc->sc_discharge_poll, zapm_poll, sc);

	if (ZAURUS_ISC3000) {
		sc->sc_ac_detect_pin = GPIO_AC_IN_C3000;
		sc->sc_batt_cover_pin = GPIO_BATT_COVER_C3000;
		sc->sc_charge_comp_pin = GPIO_CHRG_CO_C3000;
	} else {
		/* XXX */
		return;
	}

	pxa2x0_gpio_set_function(sc->sc_ac_detect_pin, GPIO_IN);
	pxa2x0_gpio_set_function(sc->sc_charge_comp_pin, GPIO_IN);
	pxa2x0_gpio_set_function(sc->sc_batt_cover_pin, GPIO_IN);

	(void)pxa2x0_gpio_intr_establish(sc->sc_ac_detect_pin,
	    IST_EDGE_BOTH, IPL_BIO, zapm_acintr, sc);
	(void)pxa2x0_gpio_intr_establish(sc->sc_charge_comp_pin,
	    IST_EDGE_BOTH, IPL_BIO, zapm_bcintr, sc);

	/* machine-independent part */
	sc->events = 0;
	sc->power_state = APM_SYS_READY;
	sc->battery_state = APM_BATT_FLAG_UNKNOWN;
	sc->ac_state = APM_AC_UNKNOWN;
	sc->battery_life = APM_BATT_LIFE_UNKNOWN;
	sc->minutes_left = 0;
	sc->sc_standby_hook = config_hook(CONFIG_HOOK_PMEVENT,
					  CONFIG_HOOK_PMEVENT_STANDBYREQ,
					  CONFIG_HOOK_EXCLUSIVE,
					  zapm_hook, sc);
	sc->sc_suspend_hook = config_hook(CONFIG_HOOK_PMEVENT,
					  CONFIG_HOOK_PMEVENT_SUSPENDREQ,
					  CONFIG_HOOK_EXCLUSIVE,
					  zapm_hook, sc);

	sc->sc_battery_hook = config_hook(CONFIG_HOOK_PMEVENT,
					  CONFIG_HOOK_PMEVENT_BATTERY,
					  CONFIG_HOOK_SHARE,
					  zapm_hook, sc);

	sc->sc_ac_hook = config_hook(CONFIG_HOOK_PMEVENT,
				     CONFIG_HOOK_PMEVENT_AC,
				     CONFIG_HOOK_SHARE,
				     zapm_hook, sc);

	aaa.accessops = &zapm_accessops;
	aaa.accesscookie = sc;
	aaa.apm_detail = 0x0102;

	sc->sc_apmdev = config_found_ia(self, "apmdevif", &aaa, apmprint);
	if (sc->sc_apmdev != NULL) {
		zapm_poll1(sc, 0);
		callout_schedule(&sc->sc_cyclic_poll, CYCLIC_TIME);
	}
}

static int
zapm_hook(void *v, int type, long id, void *msg)
{
	struct zapm_softc *sc = (struct zapm_softc *)v;
	int charge;
	int message;
	int s;

	if (type != CONFIG_HOOK_PMEVENT)
		return 1;

	if (CONFIG_HOOK_VALUEP(msg))
		message = (int)msg;
	else
		message = *(int *)msg;

	s = splhigh();

	switch (id) {
	case CONFIG_HOOK_PMEVENT_STANDBYREQ:
		if (sc->power_state != APM_SYS_STANDBY) {
			sc->events |= (1 << APM_USER_STANDBY_REQ);
		} else {
			sc->events |= (1 << APM_NORMAL_RESUME);
		}
		break;
	case CONFIG_HOOK_PMEVENT_SUSPENDREQ:
		if (sc->power_state != APM_SYS_SUSPEND) {
			DPRINTF(("zapm: suspend request\n"));
			sc->events |= (1 << APM_USER_SUSPEND_REQ);
		} else {
			sc->events |= (1 << APM_NORMAL_RESUME);
		}
		break;
	case CONFIG_HOOK_PMEVENT_BATTERY:
		switch (message) {
		case CONFIG_HOOK_BATT_CRITICAL:
			DPRINTF(("zapm: battery state critical\n"));
			charge = sc->battery_state & APM_BATT_FLAG_CHARGING;
			sc->battery_state = APM_BATT_FLAG_CRITICAL;
			sc->battery_state |= charge;
			sc->battery_life = 0;
			break;
		case CONFIG_HOOK_BATT_LOW:
			DPRINTF(("zapm: battery state low\n"));
			charge = sc->battery_state & APM_BATT_FLAG_CHARGING;
			sc->battery_state = APM_BATT_FLAG_LOW;
			sc->battery_state |= charge;
			break;
		case CONFIG_HOOK_BATT_HIGH:
			DPRINTF(("zapm: battery state high\n"));
			charge = sc->battery_state & APM_BATT_FLAG_CHARGING;
			sc->battery_state = APM_BATT_FLAG_HIGH;
			sc->battery_state |= charge;
			break;
		case CONFIG_HOOK_BATT_10P:
			DPRINTF(("zapm: battery life 10%%\n"));
			sc->battery_life = 10;
			break;
		case CONFIG_HOOK_BATT_20P:
			DPRINTF(("zapm: battery life 20%%\n"));
			sc->battery_life = 20;
			break;
		case CONFIG_HOOK_BATT_30P:
			DPRINTF(("zapm: battery life 30%%\n"));
			sc->battery_life = 30;
			break;
		case CONFIG_HOOK_BATT_40P:
			DPRINTF(("zapm: battery life 40%%\n"));
			sc->battery_life = 40;
			break;
		case CONFIG_HOOK_BATT_50P:
			DPRINTF(("zapm: battery life 50%%\n"));
			sc->battery_life = 50;
			break;
		case CONFIG_HOOK_BATT_60P:
			DPRINTF(("zapm: battery life 60%%\n"));
			sc->battery_life = 60;
			break;
		case CONFIG_HOOK_BATT_70P:
			DPRINTF(("zapm: battery life 70%%\n"));
			sc->battery_life = 70;
			break;
		case CONFIG_HOOK_BATT_80P:
			DPRINTF(("zapm: battery life 80%%\n"));
			sc->battery_life = 80;
			break;
		case CONFIG_HOOK_BATT_90P:
			DPRINTF(("zapm: battery life 90%%\n"));
			sc->battery_life = 90;
			break;
		case CONFIG_HOOK_BATT_100P:
			DPRINTF(("zapm: battery life 100%%\n"));
			sc->battery_life = 100;
			break;
		case CONFIG_HOOK_BATT_UNKNOWN:
			DPRINTF(("zapm: battery state unknown\n"));
			sc->battery_state = APM_BATT_FLAG_UNKNOWN;
			sc->battery_life = APM_BATT_LIFE_UNKNOWN;
			break;
		case CONFIG_HOOK_BATT_NO_SYSTEM_BATTERY:
			DPRINTF(("zapm: battery state no system battery?\n"));
			sc->battery_state = APM_BATT_FLAG_NO_SYSTEM_BATTERY;
			sc->battery_life = APM_BATT_LIFE_UNKNOWN;
			break;
		}
		break;
	case CONFIG_HOOK_PMEVENT_AC:
		switch (message) {
		case CONFIG_HOOK_AC_OFF:
			DPRINTF(("zapm: ac not connected\n"));
			sc->battery_state &= ~APM_BATT_FLAG_CHARGING;
			sc->ac_state = APM_AC_OFF;
			break;
		case CONFIG_HOOK_AC_ON_CHARGE:
			DPRINTF(("zapm: charging\n"));
			sc->battery_state |= APM_BATT_FLAG_CHARGING;
			sc->ac_state = APM_AC_ON;
			break;
		case CONFIG_HOOK_AC_ON_NOCHARGE:
			DPRINTF(("zapm: ac connected\n"));
			sc->battery_state &= ~APM_BATT_FLAG_CHARGING;
			sc->ac_state = APM_AC_ON;
			break;
		case CONFIG_HOOK_AC_UNKNOWN:
			sc->ac_state = APM_AC_UNKNOWN;
			break;
		}
		break;
	}

	splx(s);

	return 0;
}

static void
zapm_disconnect(void *v)
{
#if 0
	struct zapm_softc *sc = (struct zapm_softc *)v;
#endif
}

static void
zapm_enable(void *v, int onoff)
{
#if 0
	struct zapm_softc *sc = (struct zapm_softc *)v;
#endif
}

static int
zapm_set_powstate(void *v, u_int devid, u_int powstat)
{
	struct zapm_softc *sc = (struct zapm_softc *)v;

	if (devid != APM_DEV_ALLDEVS)
		return APM_ERR_UNRECOG_DEV;

	switch (powstat) {
	case APM_SYS_READY:
		DPRINTF(("zapm: set power state READY\n"));
		sc->power_state = APM_SYS_READY;
		break;
	case APM_SYS_STANDBY:
		DPRINTF(("zapm: set power state STANDBY\n"));
		/* XXX */
		DPRINTF(("zapm: resume\n"));
		break;
	case APM_SYS_SUSPEND:
		DPRINTF(("zapm: set power state SUSPEND...\n"));
		/* XXX */
		DPRINTF(("zapm: resume\n"));
		break;
	case APM_SYS_OFF:
		DPRINTF(("zapm: set power state OFF\n"));
		sc->power_state = APM_SYS_OFF;
		break;
	case APM_LASTREQ_INPROG:
		/*DPRINTF(("zapm: set power state INPROG\n"));*/
		break;
	case APM_LASTREQ_REJECTED:
		DPRINTF(("zapm: set power state REJECTED\n"));
		break;
	}

	return 0;
}

static int
zapm_get_powstat(void *v, u_int batteryid, struct apm_power_info *pinfo)
{
	struct zapm_softc *sc = (struct zapm_softc *)v;
	int val;

	if (config_hook_call(CONFIG_HOOK_GET,
			     CONFIG_HOOK_ACADAPTER, &val) != -1)
		pinfo->ac_state = val;
	else
		pinfo->ac_state = sc->ac_state;
	if (config_hook_call(CONFIG_HOOK_GET,
			     CONFIG_HOOK_CHARGE, &val) != -1)
		pinfo->battery_state = val;
	else
		pinfo->battery_state = sc->battery_state;
	if (config_hook_call(CONFIG_HOOK_GET,
			     CONFIG_HOOK_BATTERYVAL, &val) != -1)
		pinfo->battery_life = val;
	else
		pinfo->battery_life = sc->battery_life;

	return 0;
}

static int
zapm_get_event(void *v, u_int *event_type, u_int *event_info)
{
	struct zapm_softc *sc = (struct zapm_softc *)v;
	u_int ev;
	int s;

	s = splhigh();
	for (ev = APM_STANDBY_REQ; ev <= APM_CAP_CHANGE; ev++) {
		if (sc->events & (1 << ev)) {
			sc->events &= ~(1 << ev);
			*event_type = ev;
			if (*event_type == APM_NORMAL_RESUME ||
			    *event_type == APM_CRIT_RESUME) {
				/* pccard power off in the suspend state */
				*event_info = 1;
				sc->power_state = APM_SYS_READY;
			} else {
				*event_info = 0;
			}
			splx(s);

			return 0;
		}
	}
	splx(s);

	return APM_ERR_NOEVENTS;
}

static void
zapm_cpu_busy(void *v)
{
#if 0
	struct zapm_softc *sc = (struct zapm_softc *)v;
#endif
}

static void
zapm_cpu_idle(void *v)
{
#if 0
	struct zapm_softc *sc = (struct zapm_softc *)v;
#endif
}

static void
zapm_get_capabilities(void *v, u_int *numbatts, u_int *capflags)
{
#if 0
	struct zapm_softc *sc = (struct zapm_softc *)v;
#endif

	*numbatts = 1;
	*capflags = 0 /* | APM_GLOBAL_STANDBY | APM_GLOBAL_SUSPEND */;
}

/*-----------------------------------------------------------------------------
 * zaurus depent part
 */
/* MAX1111 command word */
#define MAXCTRL_PD0		(1<<0)
#define MAXCTRL_PD1		(1<<1)
#define MAXCTRL_SGL		(1<<2)
#define MAXCTRL_UNI		(1<<3)
#define MAXCTRL_SEL_SHIFT	4
#define MAXCTRL_STR		(1<<7)

/* MAX1111 ADC channels */
#define	BATT_THM		2
#define	BATT_AD			4
#define JK_VAD			6

/*
 * Battery-specific information
 */
struct battery_threshold {
	int	percent;
	int	value;
	int	state;
};

struct battery_info {
	const struct battery_threshold *bi_thres;
};

static const struct battery_threshold zaurus_battery_life_c3000[] = {
	{ 100,	212,	CONFIG_HOOK_BATT_HIGH		},
	{  98,	212,	CONFIG_HOOK_BATT_HIGH		},
	{  95,	211,	CONFIG_HOOK_BATT_HIGH		},
	{  93,	210,	CONFIG_HOOK_BATT_HIGH		},
	{  90,	209,	CONFIG_HOOK_BATT_HIGH		},
	{  88,	208,	CONFIG_HOOK_BATT_HIGH		},
	{  85,	207,	CONFIG_HOOK_BATT_HIGH		},
	{  83,	206,	CONFIG_HOOK_BATT_HIGH		},
	{  80,	205,	CONFIG_HOOK_BATT_HIGH		},
	{  78,	204,	CONFIG_HOOK_BATT_HIGH		},
	{  75,	203,	CONFIG_HOOK_BATT_HIGH		},
	{  73,	202,	CONFIG_HOOK_BATT_HIGH		},
	{  70,	201,	CONFIG_HOOK_BATT_HIGH		},
	{  68,	200,	CONFIG_HOOK_BATT_HIGH		},
	{  65,	199,	CONFIG_HOOK_BATT_HIGH		},
	{  63,	198,	CONFIG_HOOK_BATT_HIGH		},
	{  60,	197,	CONFIG_HOOK_BATT_HIGH		},
	{  58,	196,	CONFIG_HOOK_BATT_HIGH		},
	{  55,	195,	CONFIG_HOOK_BATT_HIGH		},
	{  53,	194,	CONFIG_HOOK_BATT_HIGH		},
	{  50,	193,	CONFIG_HOOK_BATT_HIGH		},
	{  48,	192,	CONFIG_HOOK_BATT_HIGH		},
	{  45,	192,	CONFIG_HOOK_BATT_HIGH		},
	{  43,	191,	CONFIG_HOOK_BATT_HIGH		},
	{  40,	191,	CONFIG_HOOK_BATT_HIGH		},
	{  38,	190,	CONFIG_HOOK_BATT_HIGH		},
	{  35,	190,	CONFIG_HOOK_BATT_HIGH		},
	{  33,	189,	CONFIG_HOOK_BATT_HIGH		},
	{  30,	188,	CONFIG_HOOK_BATT_HIGH		},
	{  28,	187,	CONFIG_HOOK_BATT_LOW		},
	{  25,	186,	CONFIG_HOOK_BATT_LOW		},
	{  23,	185,	CONFIG_HOOK_BATT_LOW		},
	{  20,	184,	CONFIG_HOOK_BATT_LOW		},
	{  18,	183,	CONFIG_HOOK_BATT_LOW		},
	{  15,	182,	CONFIG_HOOK_BATT_LOW		},
	{  13,	181,	CONFIG_HOOK_BATT_LOW		},
	{  10,	180,	CONFIG_HOOK_BATT_LOW		},
	{   8,	179,	CONFIG_HOOK_BATT_LOW		},
	{   5,	178,	CONFIG_HOOK_BATT_LOW		},
	{   0,	  0,	CONFIG_HOOK_BATT_CRITICAL	}
};

static const struct battery_info zaurus_battery_c3000 = {
	zaurus_battery_life_c3000
};

static const struct battery_info *zaurus_main_battery = &zaurus_battery_c3000;

/* Restart charging this many times before accepting BATT_FULL. */
#define	MIN_BATT_FULL		2

/* Discharge 100 ms before reading the voltage if AC is connected. */
#define	DISCHARGE_TIMEOUT	(hz / 10)

/* Check battery voltage and "kick charging" every minute. */
static const struct timeval zapm_battchkrate = { 60, 0 };

static int	zapm_get_ac_state(struct zapm_softc *);
static int	zapm_get_battery_compartment_state(struct zapm_softc *);
static int	zapm_get_charge_complete_state(struct zapm_softc *);
static void	zapm_set_charging(struct zapm_softc *, int);
static int	zapm_charge_complete(struct zapm_softc *);
static int	max1111_adc_value_avg(int chan, int pause);
static int	zapm_get_battery_volt(void);
static int	zapm_battery_state(int volt);
static int	zapm_battery_life(int volt);

static int
zapm_acintr(void *v)
{

	zapm_poll1(v, 1);

	return 1;
}

static int
zapm_bcintr(void *v)
{

	zapm_poll1(v, 1);

	return 1;
}

static void
zapm_cyclic(void *v)
{
	struct zapm_softc *sc = (struct zapm_softc *)v;

	zapm_poll1(sc, 1);

	callout_schedule(&sc->sc_cyclic_poll, CYCLIC_TIME);
}

static void
zapm_poll(void *v)
{

	zapm_poll1(v, 1);
}

static int
zapm_get_ac_state(struct zapm_softc *sc)
{

	if (!pxa2x0_gpio_get_bit(sc->sc_ac_detect_pin))
		return APM_AC_ON;
	return APM_AC_OFF;
}

static int
zapm_get_battery_compartment_state(struct zapm_softc *sc)
{

	return pxa2x0_gpio_get_bit(sc->sc_batt_cover_pin);
}

static int
zapm_get_charge_complete_state(struct zapm_softc *sc)
{

	return pxa2x0_gpio_get_bit(sc->sc_charge_comp_pin);
}

static void
zapm_set_charging(struct zapm_softc *sc, int enable)
{

	scoop_discharge_battery(0);
	scoop_charge_battery(enable, 0);
	scoop_led_set(SCOOP_LED_ORANGE, enable);
}

/*
 * Return non-zero if the charge complete signal indicates that the
 * battery is fully charged.  Restart charging to clear this signal.
 */
static int
zapm_charge_complete(struct zapm_softc *sc)
{

	if (sc->charging && sc->battery_full_cnt < MIN_BATT_FULL) {
		if (zapm_get_charge_complete_state(sc)) {
			sc->battery_full_cnt++;
			if (sc->battery_full_cnt < MIN_BATT_FULL) {
				DPRINTF(("battery almost full\n"));
				zapm_set_charging(sc, 0);
				delay(15000);
				zapm_set_charging(sc, 1);
			}
		} else if (sc->battery_full_cnt > 0) {
			/* false alarm */
			sc->battery_full_cnt = 0;
			zapm_set_charging(sc, 0);
			delay(15000);
			zapm_set_charging(sc, 1);
		}
	}

	return (sc->battery_full_cnt >= MIN_BATT_FULL);
}

static int
max1111_adc_value(int chan)
{

	return ((int)zssp_ic_send(ZSSP_IC_MAX1111, MAXCTRL_PD0 |
	    MAXCTRL_PD1 | MAXCTRL_SGL | MAXCTRL_UNI |
	    (chan << MAXCTRL_SEL_SHIFT) | MAXCTRL_STR));
}

/* XXX simplify */
static int
max1111_adc_value_avg(int chan, int pause)
{
	int val[5];
	int sum;
	int minv, maxv, v;
	int i;

	DPRINTF(("max1111_adc_value_avg: chan = %d, pause = %d\n",
	    chan, pause));

	for (i = 0; i < 5; i++) {
		val[i] = max1111_adc_value(chan);
		if (i != 4)
			delay(pause * 1000);
		DPRINTF(("max1111_adc_value_avg: chan[%d] = %d\n", i, val[i]));
	}

	/* get max value */
	v = val[0];
	minv = 0;
	for (i = 1; i < 5; i++) {
		if (v < val[i]) {
			v = val[i];
			minv = i;
		}
	}

	/* get min value */
	v = val[4];
	maxv = 4;
	for (i = 3; i >= 0; i--) {
		if (v > val[i]) {
			v = val[i];
			maxv = i;
		}
	}

	DPRINTF(("max1111_adc_value_avg: minv = %d, maxv = %d\n", minv, maxv));
	sum = 0;
	for (i = 0; i < 5; i++) {
		if (i == minv || i == maxv)
			continue;
		sum += val[i];
	}

	DPRINTF(("max1111_adc_value_avg: sum = %d, sum / 3 = %d\n",
	    sum, sum / 3));

	return sum / 3;
}

static int
zapm_get_battery_volt(void)
{

	return max1111_adc_value_avg(BATT_AD, 10);
}

static int
zapm_battery_state(int volt)
{
	const struct battery_threshold *bthr;
	int i;

	bthr = zaurus_main_battery->bi_thres;

	for (i = 0; bthr[i].value > 0; i++)
		if (bthr[i].value <= volt)
			break;

	return bthr[i].state;
}

static int
zapm_battery_life(int volt)
{
	const struct battery_threshold *bthr;
	int i;

	bthr = zaurus_main_battery->bi_thres;

	for (i = 0; bthr[i].value > 0; i++)
		if (bthr[i].value <= volt)
			break;

	if (i == 0)
		return bthr[0].percent;

	return (bthr[i].percent +
	    ((volt - bthr[i].value) * 100) /
	    (bthr[i-1].value - bthr[i].value) *
	    (bthr[i-1].percent - bthr[i].percent) / 100);
}

/*
 * Poll power-management related GPIO inputs, update battery life
 * in softc, and/or control battery charging.
 */
static void
zapm_poll1(void *v, int do_suspend)
{
	struct zapm_softc *sc = (struct zapm_softc *)v;
	int ac_state;
	int bc_lock;
	int charging;
	int volt;
	int s;

	s = splhigh();

	ac_state = zapm_get_ac_state(sc);
	bc_lock = zapm_get_battery_compartment_state(sc);

	/* Stop discharging. */
	if (sc->discharging) {
		sc->discharging = 0;
		charging = 0;
		volt = zapm_get_battery_volt();
		DPRINTF(("zapm_poll: discharge off volt %d\n", volt));
	} else {
		charging = sc->battery_state & APM_BATT_FLAG_CHARGING;
		volt = sc->battery_volt;
	}

	/* Start or stop charging as necessary. */
	if (ac_state && bc_lock) {
		int charge_completed = zapm_charge_complete(sc);
		if (charging) {
			if (charge_completed) {
				DPRINTF(("zapm_poll: battery is full\n"));
				charging = 0;
				zapm_set_charging(sc, 0);
			}
		} else if (!charge_completed) {
			charging = 1;
			volt = zapm_get_battery_volt();
			zapm_set_charging(sc, 1);
			DPRINTF(("zapm_poll: start charging volt %d\n", volt));
		}
	} else {
		if (charging) {
			charging = 0;
			zapm_set_charging(sc, 0);
			timerclear(&sc->sc_lastbattchk);
			DPRINTF(("zapm_poll: stop charging\n"));
		}
		sc->battery_full_cnt = 0;
	}

	/*
	 * Restart charging once in a while.  Discharge a few milliseconds
	 * before updating the voltage in our softc if A/C is connected.
	 */
	if (bc_lock && ratecheck(&sc->sc_lastbattchk, &zapm_battchkrate)) {
		if (do_suspend && sc->suspended) {
			/* XXX */
#if 0
			DPRINTF(("zapm_poll: suspended %lu %lu\n",
			    sc->lastbattchk.tv_sec,
			    pxa2x0_rtc_getsecs()));
			if (charging) {
				zapm_set_charging(sc, 0);
				delay(15000);
				zapm_set_charging(sc, 1);
				pxa2x0_rtc_setalarm(pxa2x0_rtc_getsecs() +
				    zapm_battchkrate.tv_sec + 1);
			}
#endif
		} else if (ac_state && sc->battery_full_cnt == 0) {
			DPRINTF(("zapm_poll: discharge on\n"));
			if (charging)
				zapm_set_charging(sc, 0);
			sc->discharging = 1;
			scoop_discharge_battery(1);
			callout_schedule(&sc->sc_discharge_poll,
			    DISCHARGE_TIMEOUT);
		} else if (!ac_state) {
			volt = zapm_get_battery_volt();
			DPRINTF(("zapm_poll: volt %d\n", volt));
		}
	}

	/* Update the cached power state in our softc. */
	if ((ac_state != sc->ac_state)
	 || (charging != (sc->battery_state & APM_BATT_FLAG_CHARGING))) {
		config_hook_call(CONFIG_HOOK_PMEVENT,
		    CONFIG_HOOK_PMEVENT_AC,
		    (void *)((ac_state == APM_AC_OFF)
		        ? CONFIG_HOOK_AC_OFF
		        : (charging ? CONFIG_HOOK_AC_ON_CHARGE
		                    : CONFIG_HOOK_AC_ON_NOCHARGE)));
	}
	if (volt != sc->battery_volt) {
		sc->battery_volt = volt;
		sc->battery_life = zapm_battery_life(volt);
		config_hook_call(CONFIG_HOOK_PMEVENT,
		    CONFIG_HOOK_PMEVENT_BATTERY,
		    (void *)zapm_battery_state(volt));
	}

	splx(s);
}
