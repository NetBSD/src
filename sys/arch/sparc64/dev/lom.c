/*	$NetBSD: lom.c,v 1.10.2.1 2013/02/25 00:28:59 tls Exp $	*/
/*	$OpenBSD: lom.c,v 1.21 2010/02/28 20:44:39 kettenis Exp $	*/
/*
 * Copyright (c) 2009 Mark Kettenis
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
__KERNEL_RCSID(0, "$NetBSD: lom.c,v 1.10.2.1 2013/02/25 00:28:59 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/envsys.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/sysctl.h>

#include <machine/autoconf.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <dev/sysmon/sysmonvar.h>

/*
 * LOMlite is a so far unidentified microcontroller.
 */
#define LOM1_STATUS		0x00	/* R */
#define  LOM1_STATUS_BUSY	0x80
#define LOM1_CMD		0x00	/* W */
#define LOM1_DATA		0x01	/* R/W */

/*
 * LOMlite2 is implemented as a H8/3437 microcontroller which has its
 * on-chip host interface hooked up to EBus.
 */
#define LOM2_DATA		0x00	/* R/W */
#define LOM2_CMD		0x01	/* W */
#define LOM2_STATUS		0x01	/* R */
#define  LOM2_STATUS_OBF	0x01	/* Output Buffer Full */
#define  LOM2_STATUS_IBF	0x02	/* Input Buffer Full  */

#define LOM_IDX_CMD		0x00
#define  LOM_IDX_CMD_GENERIC	0x00
#define  LOM_IDX_CMD_TEMP	0x04
#define  LOM_IDX_CMD_FAN	0x05

#define LOM_IDX_FW_REV		0x01	/* Firmware revision  */

#define LOM_IDX_FAN1		0x04	/* Fan speed */
#define LOM_IDX_FAN2		0x05
#define LOM_IDX_FAN3		0x06
#define LOM_IDX_FAN4		0x07
#define LOM_IDX_PSU1		0x08	/* PSU status */
#define LOM_IDX_PSU2		0x09
#define LOM_IDX_PSU3		0x0a
#define  LOM_PSU_INPUTA		0x01
#define  LOM_PSU_INPUTB		0x02
#define  LOM_PSU_OUTPUT		0x04
#define  LOM_PSU_PRESENT	0x08
#define  LOM_PSU_STANDBY	0x10

#define LOM_IDX_TEMP1		0x18	/* Temperature */
#define LOM_IDX_TEMP2		0x19
#define LOM_IDX_TEMP3		0x1a
#define LOM_IDX_TEMP4		0x1b
#define LOM_IDX_TEMP5		0x1c
#define LOM_IDX_TEMP6		0x1d
#define LOM_IDX_TEMP7		0x1e
#define LOM_IDX_TEMP8		0x1f

#define LOM_IDX_LED1		0x25

#define LOM_IDX_ALARM		0x30
#define  LOM_ALARM_1		0x01
#define  LOM_ALARM_2		0x02
#define  LOM_ALARM_3		0x04
#define  LOM_ALARM_FAULT	0xf0
#define LOM_IDX_WDOG_CTL	0x31
#define  LOM_WDOG_ENABLE	0x01
#define  LOM_WDOG_RESET		0x02
#define  LOM_WDOG_AL3_WDOG	0x04
#define  LOM_WDOG_AL3_FANPSU	0x08
#define LOM_IDX_WDOG_TIME	0x32
#define  LOM_WDOG_TIME_MAX	126

#define LOM1_IDX_HOSTNAME1	0x33
#define LOM1_IDX_HOSTNAME2	0x34
#define LOM1_IDX_HOSTNAME3	0x35
#define LOM1_IDX_HOSTNAME4	0x36
#define LOM1_IDX_HOSTNAME5	0x37
#define LOM1_IDX_HOSTNAME6	0x38
#define LOM1_IDX_HOSTNAME7	0x39
#define LOM1_IDX_HOSTNAME8	0x3a
#define LOM1_IDX_HOSTNAME9	0x3b
#define LOM1_IDX_HOSTNAME10	0x3c
#define LOM1_IDX_HOSTNAME11	0x3d
#define LOM1_IDX_HOSTNAME12	0x3e

#define LOM2_IDX_HOSTNAMELEN	0x38
#define LOM2_IDX_HOSTNAME	0x39

#define LOM_IDX_CONFIG		0x5d
#define LOM_IDX_FAN1_CAL	0x5e
#define LOM_IDX_FAN2_CAL	0x5f
#define LOM_IDX_FAN3_CAL	0x60
#define LOM_IDX_FAN4_CAL	0x61
#define LOM_IDX_FAN1_LOW	0x62
#define LOM_IDX_FAN2_LOW	0x63
#define LOM_IDX_FAN3_LOW	0x64
#define LOM_IDX_FAN4_LOW	0x65

#define LOM_IDX_CONFIG2		0x66
#define LOM_IDX_CONFIG3		0x67

#define LOM_IDX_PROBE55		0x7e	/* Always returns 0x55 */
#define LOM_IDX_PROBEAA		0x7f	/* Always returns 0xaa */

#define LOM_IDX_WRITE		0x80

#define LOM_IDX4_TEMP_NAME_START	0x40
#define LOM_IDX4_TEMP_NAME_END		0xff

#define LOM_IDX5_FAN_NAME_START		0x40
#define LOM_IDX5_FAN_NAME_END		0xff

#define LOM_MAX_ALARM	4
#define LOM_MAX_FAN	4
#define LOM_MAX_PSU	3
#define LOM_MAX_TEMP	8

struct lom_cmd {
	uint8_t			lc_cmd;
	uint8_t			lc_data;

	TAILQ_ENTRY(lom_cmd)	lc_next;
};

struct lom_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	int			sc_type;
#define LOM_LOMLITE		0
#define LOM_LOMLITE2		2
	int			sc_space;

	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_alarm[LOM_MAX_ALARM];
	envsys_data_t		sc_fan[LOM_MAX_FAN];
	envsys_data_t		sc_psu[LOM_MAX_PSU];
	envsys_data_t		sc_temp[LOM_MAX_TEMP];

	int			sc_num_alarm;
	int			sc_num_fan;
	int			sc_num_psu;
	int			sc_num_temp;

	int32_t			sc_sysctl_num[LOM_MAX_ALARM];

	struct timeval		sc_alarm_lastread;
	uint8_t			sc_alarm_lastval;
	struct timeval		sc_fan_lastread[LOM_MAX_FAN];
	struct timeval		sc_psu_lastread[LOM_MAX_PSU];
	struct timeval		sc_temp_lastread[LOM_MAX_TEMP];

	uint8_t			sc_fan_cal[LOM_MAX_FAN];
	uint8_t			sc_fan_low[LOM_MAX_FAN];

	char			sc_hostname[MAXHOSTNAMELEN];

	struct sysmon_wdog	sc_smw;
	int			sc_wdog_period;
	uint8_t			sc_wdog_ctl;
	struct lom_cmd		sc_wdog_pat;

	TAILQ_HEAD(, lom_cmd)	sc_queue;
	kmutex_t		sc_queue_mtx;
	struct callout		sc_state_to;
	int			sc_state;
#define LOM_STATE_IDLE		0
#define LOM_STATE_CMD		1
#define LOM_STATE_DATA		2
	int			sc_retry;
};

static int	lom_match(device_t, cfdata_t, void *);
static void	lom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lom, sizeof(struct lom_softc),
    lom_match, lom_attach, NULL, NULL);

static int	lom_read(struct lom_softc *, uint8_t, uint8_t *);
static int	lom_write(struct lom_softc *, uint8_t, uint8_t);
static void	lom_queue_cmd(struct lom_softc *, struct lom_cmd *);
static void	lom_dequeue_cmd(struct lom_softc *, struct lom_cmd *);
static int	lom1_read(struct lom_softc *, uint8_t, uint8_t *);
static int	lom1_write(struct lom_softc *, uint8_t, uint8_t);
static int	lom1_read_polled(struct lom_softc *, uint8_t, uint8_t *);
static int	lom1_write_polled(struct lom_softc *, uint8_t, uint8_t);
static void	lom1_queue_cmd(struct lom_softc *, struct lom_cmd *);
static void	lom1_process_queue(void *);
static void	lom1_process_queue_locked(struct lom_softc *);
static int	lom2_read(struct lom_softc *, uint8_t, uint8_t *);
static int	lom2_write(struct lom_softc *, uint8_t, uint8_t);
static int	lom2_read_polled(struct lom_softc *, uint8_t, uint8_t *);
static int	lom2_write_polled(struct lom_softc *, uint8_t, uint8_t);
static void	lom2_queue_cmd(struct lom_softc *, struct lom_cmd *);
static int	lom2_intr(void *);

static int	lom_init_desc(struct lom_softc *);
static void	lom_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	lom_refresh_alarm(struct lom_softc *, envsys_data_t *, uint32_t);
static void	lom_refresh_fan(struct lom_softc *, envsys_data_t *, uint32_t);
static void	lom_refresh_psu(struct lom_softc *, envsys_data_t *, uint32_t);
static void	lom_refresh_temp(struct lom_softc *, envsys_data_t *, uint32_t);
static void	lom1_write_hostname(struct lom_softc *);
static void	lom2_write_hostname(struct lom_softc *);

static int	lom_wdog_tickle(struct sysmon_wdog *);
static int	lom_wdog_setmode(struct sysmon_wdog *);

static bool	lom_shutdown(device_t, int);

SYSCTL_SETUP_PROTO(sysctl_lom_setup);
static int	lom_sysctl_alarm(SYSCTLFN_PROTO);

static int hw_node = CTL_EOL;
static const char *nodename[LOM_MAX_ALARM] =
    { "fault_led", "alarm1", "alarm2", "alarm3" };
#ifdef SYSCTL_INCLUDE_DESCR
static const char *nodedesc[LOM_MAX_ALARM] =
    { "Fault LED status", "Alarm1 status", "Alarm2 status ", "Alarm3 status" };
#endif
static const struct timeval refresh_interval = { 1, 0 };

static int
lom_match(device_t parent, cfdata_t match, void *aux)
{
	struct ebus_attach_args *ea = aux;

	if (strcmp(ea->ea_name, "SUNW,lom") == 0 ||
	    strcmp(ea->ea_name, "SUNW,lomh") == 0)
		return (1);

	return (0);
}

static void
lom_attach(device_t parent, device_t self, void *aux)
{
	struct lom_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;
	uint8_t reg, fw_rev, config, config2, config3;
	uint8_t cal, low;
	int i;
	const struct sysctlnode *node = NULL, *newnode;

	if (strcmp(ea->ea_name, "SUNW,lomh") == 0) {
		if (ea->ea_nintr < 1) {
			aprint_error(": no interrupt\n");
			return;
		}
		sc->sc_type = LOM_LOMLITE2;
	}

	sc->sc_dev = self;
	sc->sc_iot = ea->ea_bustag;
	if (bus_space_map(sc->sc_iot, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
	    ea->ea_reg[0].size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": can't map register space\n");
		return;
	}

	if (sc->sc_type < LOM_LOMLITE2) {
		/* XXX Magic */
		(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, 3, 0xca);
	}

	if (lom_read(sc, LOM_IDX_PROBE55, &reg) || reg != 0x55 ||
	    lom_read(sc, LOM_IDX_PROBEAA, &reg) || reg != 0xaa ||
	    lom_read(sc, LOM_IDX_FW_REV, &fw_rev) ||
	    lom_read(sc, LOM_IDX_CONFIG, &config))
	{
		aprint_error(": not responding\n");
		return;
	}

	aprint_normal(": %s: %s rev %d.%d\n", ea->ea_name,
	    sc->sc_type < LOM_LOMLITE2 ? "LOMlite" : "LOMlite2",
	    fw_rev >> 4, fw_rev & 0x0f);

	TAILQ_INIT(&sc->sc_queue);
	mutex_init(&sc->sc_queue_mtx, MUTEX_DEFAULT, IPL_BIO);

	config2 = config3 = 0;
	if (sc->sc_type < LOM_LOMLITE2) {
		/*
		 * LOMlite doesn't do interrupts so we limp along on
		 * timeouts.
		 */
		callout_init(&sc->sc_state_to, 0);
		callout_setfunc(&sc->sc_state_to, lom1_process_queue, sc);
	} else {
		lom_read(sc, LOM_IDX_CONFIG2, &config2);
		lom_read(sc, LOM_IDX_CONFIG3, &config3);

		bus_intr_establish(sc->sc_iot, ea->ea_intr[0],
		    IPL_BIO, lom2_intr, sc);
	}

	sc->sc_num_alarm = LOM_MAX_ALARM;
	sc->sc_num_fan = min((config >> 5) & 0x7, LOM_MAX_FAN);
	sc->sc_num_psu = min((config >> 3) & 0x3, LOM_MAX_PSU);
	sc->sc_num_temp = min((config2 >> 4) & 0xf, LOM_MAX_TEMP);

	aprint_verbose_dev(self, "%d fan(s), %d PSU(s), %d temp sensor(s)\n",
	    sc->sc_num_fan, sc->sc_num_psu, sc->sc_num_temp);

	for (i = 0; i < sc->sc_num_fan; i++) {
		if (lom_read(sc, LOM_IDX_FAN1_CAL + i, &cal) ||
		    lom_read(sc, LOM_IDX_FAN1_LOW + i, &low)) {
			aprint_error_dev(self, "can't read fan information\n");
			return;
		}
		sc->sc_fan_cal[i] = cal;
		sc->sc_fan_low[i] = low;
	}

	/* Setup our sysctl subtree, hw.lomN */
	if (hw_node != CTL_EOL)
		sysctl_createv(NULL, 0, NULL, &node,
		    0, CTLTYPE_NODE, device_xname(self), NULL,
		    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	/* Initialize sensor data. */
	sc->sc_sme = sysmon_envsys_create();
	for (i = 0; i < sc->sc_num_alarm; i++) {
		sc->sc_alarm[i].units = ENVSYS_INDICATOR;
		sc->sc_alarm[i].state = ENVSYS_SINVALID;
		snprintf(sc->sc_alarm[i].desc, sizeof(sc->sc_alarm[i].desc),
		    i == 0 ? "Fault LED" : "Alarm%d", i);
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_alarm[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			aprint_error_dev(self, "can't attach alarm sensor\n");
			return;
		}
		if (node != NULL) {
			sysctl_createv(NULL, 0, NULL, &newnode,
			    CTLFLAG_READWRITE, CTLTYPE_INT, nodename[i],
			    SYSCTL_DESCR(nodedesc[i]),
			    lom_sysctl_alarm, 0, (void *)sc, 0,
			    CTL_HW, node->sysctl_num, CTL_CREATE, CTL_EOL);
			if (newnode != NULL)
				sc->sc_sysctl_num[i] = newnode->sysctl_num;
			else
				sc->sc_sysctl_num[i] = 0;
		}
	}
	for (i = 0; i < sc->sc_num_fan; i++) {
		sc->sc_fan[i].units = ENVSYS_SFANRPM;
		sc->sc_fan[i].state = ENVSYS_SINVALID;
		snprintf(sc->sc_fan[i].desc, sizeof(sc->sc_fan[i].desc),
		    "fan%d", i + 1);
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_fan[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			aprint_error_dev(self, "can't attach fan sensor\n");
			return;
		}
	}
	for (i = 0; i < sc->sc_num_psu; i++) {
		sc->sc_psu[i].units = ENVSYS_INDICATOR;
		sc->sc_psu[i].state = ENVSYS_SINVALID;
		snprintf(sc->sc_psu[i].desc, sizeof(sc->sc_psu[i].desc),
		    "PSU%d", i + 1);
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_psu[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			aprint_error_dev(self, "can't attach PSU sensor\n");
			return;
		}
	}
	for (i = 0; i < sc->sc_num_temp; i++) {
		sc->sc_temp[i].units = ENVSYS_STEMP;
		sc->sc_temp[i].state = ENVSYS_SINVALID;
		snprintf(sc->sc_temp[i].desc, sizeof(sc->sc_temp[i].desc),
		    "temp%d", i + 1);
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_temp[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			aprint_error_dev(self, "can't attach temp sensor\n");
			return;
		}
	}
	if (lom_init_desc(sc)) {
		aprint_error_dev(self, "can't read sensor names\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = lom_refresh;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self,
		    "unable to register envsys with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* Initialize watchdog. */
	lom_write(sc, LOM_IDX_WDOG_TIME, LOM_WDOG_TIME_MAX);
	lom_read(sc, LOM_IDX_WDOG_CTL, &sc->sc_wdog_ctl);
	sc->sc_wdog_ctl &= ~(LOM_WDOG_ENABLE|LOM_WDOG_RESET);
	lom_write(sc, LOM_IDX_WDOG_CTL, sc->sc_wdog_ctl);

	sc->sc_wdog_period = LOM_WDOG_TIME_MAX;

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = lom_wdog_setmode;
	sc->sc_smw.smw_tickle = lom_wdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;
	if (sysmon_wdog_register(&sc->sc_smw)) {
		aprint_error_dev(self,
		    "unable to register wdog with sysmon\n");
		return;
	}

	aprint_verbose_dev(self, "Watchdog timer configured.\n");

	if (!pmf_device_register1(self, NULL, NULL, lom_shutdown))
		aprint_error_dev(self, "unable to register power handler\n");
}

static int
lom_read(struct lom_softc *sc, uint8_t reg, uint8_t *val)
{
	if (sc->sc_type < LOM_LOMLITE2)
		return lom1_read(sc, reg, val);
	else
		return lom2_read(sc, reg, val);
}

static int
lom_write(struct lom_softc *sc, uint8_t reg, uint8_t val)
{
	if (sc->sc_type < LOM_LOMLITE2)
		return lom1_write(sc, reg, val);
	else
		return lom2_write(sc, reg, val);
}

static void
lom_queue_cmd(struct lom_softc *sc, struct lom_cmd *lc)
{
	if (sc->sc_type < LOM_LOMLITE2)
		return lom1_queue_cmd(sc, lc);
	else
		return lom2_queue_cmd(sc, lc);
}

static void
lom_dequeue_cmd(struct lom_softc *sc, struct lom_cmd *lc)
{
	struct lom_cmd *lcp;

	mutex_enter(&sc->sc_queue_mtx);
	TAILQ_FOREACH(lcp, &sc->sc_queue, lc_next) {
		if (lcp == lc) {
			TAILQ_REMOVE(&sc->sc_queue, lc, lc_next);
			break;
		}
	}
	mutex_exit(&sc->sc_queue_mtx);
}

static int
lom1_read(struct lom_softc *sc, uint8_t reg, uint8_t *val)
{
	struct lom_cmd lc;
	int error;

	if (cold)
		return lom1_read_polled(sc, reg, val);

	lc.lc_cmd = reg;
	lc.lc_data = 0xff;
	lom1_queue_cmd(sc, &lc);

	error = tsleep(&lc, PZERO, "lomrd", hz);
	if (error)
		lom_dequeue_cmd(sc, &lc);

	*val = lc.lc_data;

	return (error);
}

static int
lom1_write(struct lom_softc *sc, uint8_t reg, uint8_t val)
{
	struct lom_cmd lc;
	int error;

	if (cold)
		return lom1_write_polled(sc, reg, val);

	lc.lc_cmd = reg | LOM_IDX_WRITE;
	lc.lc_data = val;
	lom1_queue_cmd(sc, &lc);

	error = tsleep(&lc, PZERO, "lomwr", 2 * hz);
	if (error)
		lom_dequeue_cmd(sc, &lc);

	return (error);
}

static int
lom1_read_polled(struct lom_softc *sc, uint8_t reg, uint8_t *val)
{
	uint8_t str;
	int i;

	/* Wait for input buffer to become available. */
	for (i = 30; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_STATUS);
		delay(1000);
		if ((str & LOM1_STATUS_BUSY) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM1_CMD, reg);

	/* Wait until the microcontroller fills output buffer. */
	for (i = 30; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_STATUS);
		delay(1000);
		if ((str & LOM1_STATUS_BUSY) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	*val = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_DATA);
	return (0);
}

static int
lom1_write_polled(struct lom_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t str;
	int i;

	/* Wait for input buffer to become available. */
	for (i = 30; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_STATUS);
		delay(1000);
		if ((str & LOM1_STATUS_BUSY) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	reg |= LOM_IDX_WRITE;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM1_CMD, reg);

	/* Wait until the microcontroller fills output buffer. */
	for (i = 30; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_STATUS);
		delay(1000);
		if ((str & LOM1_STATUS_BUSY) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM1_DATA, val);

	return (0);
}

static void
lom1_queue_cmd(struct lom_softc *sc, struct lom_cmd *lc)
{
	mutex_enter(&sc->sc_queue_mtx);
	TAILQ_INSERT_TAIL(&sc->sc_queue, lc, lc_next);
	if (sc->sc_state == LOM_STATE_IDLE) {
		sc->sc_state = LOM_STATE_CMD;
		lom1_process_queue_locked(sc);
	}
	mutex_exit(&sc->sc_queue_mtx);
}

static void
lom1_process_queue(void *arg)
{
	struct lom_softc *sc = arg;

	mutex_enter(&sc->sc_queue_mtx);
	lom1_process_queue_locked(sc);
	mutex_exit(&sc->sc_queue_mtx);
}

static void
lom1_process_queue_locked(struct lom_softc *sc)
{
	struct lom_cmd *lc;
	uint8_t str;

	lc = TAILQ_FIRST(&sc->sc_queue);
	if (lc == NULL) {
		sc->sc_state = LOM_STATE_IDLE;
		return;
	}

	str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_STATUS);
	if (str & LOM1_STATUS_BUSY) {
		if (sc->sc_retry++ < 30) {
			callout_schedule(&sc->sc_state_to, mstohz(1));
			return;
		}

		/*
		 * Looks like the microcontroller got wedged.  Unwedge
		 * it by writing this magic value.  Give it some time
		 * to recover.
		 */
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM1_DATA, 0xac);
		callout_schedule(&sc->sc_state_to, mstohz(1000));
		sc->sc_state = LOM_STATE_CMD;
		return;
	}

	sc->sc_retry = 0;

	if (sc->sc_state == LOM_STATE_CMD) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM1_CMD, lc->lc_cmd);
		sc->sc_state = LOM_STATE_DATA;
		callout_schedule(&sc->sc_state_to, mstohz(250));
		return;
	}

	KASSERT(sc->sc_state == LOM_STATE_DATA);
	if ((lc->lc_cmd & LOM_IDX_WRITE) == 0)
		lc->lc_data = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM1_DATA);
	else
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM1_DATA, lc->lc_data);

	TAILQ_REMOVE(&sc->sc_queue, lc, lc_next);

	wakeup(lc);

	if (!TAILQ_EMPTY(&sc->sc_queue)) {
		sc->sc_state = LOM_STATE_CMD;
		callout_schedule(&sc->sc_state_to, mstohz(1));
		return;
	}

	sc->sc_state = LOM_STATE_IDLE;
}

static int
lom2_read(struct lom_softc *sc, uint8_t reg, uint8_t *val)
{
	struct lom_cmd lc;
	int error;

	if (cold)
		return lom2_read_polled(sc, reg, val);

	lc.lc_cmd = reg;
	lc.lc_data = 0xff;
	lom2_queue_cmd(sc, &lc);

	error = tsleep(&lc, PZERO, "lom2rd", hz);
	if (error)
		lom_dequeue_cmd(sc, &lc);

	*val = lc.lc_data;

	return (error);
}

static int
lom2_read_polled(struct lom_softc *sc, uint8_t reg, uint8_t *val)
{
	uint8_t str;
	int i;

	/* Wait for input buffer to become available. */
	for (i = 1000; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		delay(10);
		if ((str & LOM2_STATUS_IBF) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM2_CMD, reg);

	/* Wait until the microcontroller fills output buffer. */
	for (i = 1000; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		delay(10);
		if (str & LOM2_STATUS_OBF)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	*val = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_DATA);
	return (0);
}

static int
lom2_write(struct lom_softc *sc, uint8_t reg, uint8_t val)
{
	struct lom_cmd lc;
	int error;

	if (cold)
		return lom2_write_polled(sc, reg, val);

	lc.lc_cmd = reg | LOM_IDX_WRITE;
	lc.lc_data = val;
	lom2_queue_cmd(sc, &lc);

	error = tsleep(&lc, PZERO, "lom2wr", hz);
	if (error)
		lom_dequeue_cmd(sc, &lc);

	return (error);
}

static int
lom2_write_polled(struct lom_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t str;
	int i;

	/* Wait for input buffer to become available. */
	for (i = 1000; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		delay(10);
		if ((str & LOM2_STATUS_IBF) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	if (sc->sc_space == LOM_IDX_CMD_GENERIC && reg != LOM_IDX_CMD)
		reg |= LOM_IDX_WRITE;

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM2_CMD, reg);

	/* Wait until the microcontroller fills output buffer. */
	for (i = 1000; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		delay(10);
		if (str & LOM2_STATUS_OBF)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_DATA);

	/* Wait for input buffer to become available. */
	for (i = 1000; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		delay(10);
		if ((str & LOM2_STATUS_IBF) == 0)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LOM2_DATA, val);

	/* Wait until the microcontroller fills output buffer. */
	for (i = 1000; i > 0; i--) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		delay(10);
		if (str & LOM2_STATUS_OBF)
			break;
	}
	if (i == 0)
		return (ETIMEDOUT);

	(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_DATA);

	/* If we switched spaces, remember the one we're in now. */
	if (reg == LOM_IDX_CMD)
		sc->sc_space = val;

	return (0);
}

static void
lom2_queue_cmd(struct lom_softc *sc, struct lom_cmd *lc)
{
	uint8_t str;

	mutex_enter(&sc->sc_queue_mtx);
	TAILQ_INSERT_TAIL(&sc->sc_queue, lc, lc_next);
	if (sc->sc_state == LOM_STATE_IDLE) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		if ((str & LOM2_STATUS_IBF) == 0) {
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    LOM2_CMD, lc->lc_cmd);
			sc->sc_state = LOM_STATE_DATA;
		}
	}
	mutex_exit(&sc->sc_queue_mtx);
}

static int
lom2_intr(void *arg)
{
	struct lom_softc *sc = arg;
	struct lom_cmd *lc;
	uint8_t str, obr;

	mutex_enter(&sc->sc_queue_mtx);

	str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
	obr = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_DATA);

	lc = TAILQ_FIRST(&sc->sc_queue);
	if (lc == NULL) {
		mutex_exit(&sc->sc_queue_mtx);
		return (0);
	}

	if (lc->lc_cmd & LOM_IDX_WRITE) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
		    LOM2_DATA, lc->lc_data);
		lc->lc_cmd &= ~LOM_IDX_WRITE;
		mutex_exit(&sc->sc_queue_mtx);
		return (1);
	}

	KASSERT(sc->sc_state == LOM_STATE_DATA);
	lc->lc_data = obr;

	TAILQ_REMOVE(&sc->sc_queue, lc, lc_next);

	wakeup(lc);

	sc->sc_state = LOM_STATE_IDLE;

	if (!TAILQ_EMPTY(&sc->sc_queue)) {
		str = bus_space_read_1(sc->sc_iot, sc->sc_ioh, LOM2_STATUS);
		if ((str & LOM2_STATUS_IBF) == 0) {
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    LOM2_CMD, lc->lc_cmd);
			sc->sc_state = LOM_STATE_DATA;
		}
	}

	mutex_exit(&sc->sc_queue_mtx);

	return (1);
}

static int
lom_init_desc(struct lom_softc *sc)
{
	uint8_t val;
	int i, j, k;
	int error;

	/* LOMlite doesn't provide sensor descriptions. */
	if (sc->sc_type < LOM_LOMLITE2)
		return (0);

	/*
	 * Read temperature sensor names.
	 */
	error = lom_write(sc, LOM_IDX_CMD, LOM_IDX_CMD_TEMP);
	if (error)
		return (error);

	i = 0;
	j = 0;
	k = LOM_IDX4_TEMP_NAME_START;
	while (k <= LOM_IDX4_TEMP_NAME_END) {
		error = lom_read(sc, k++, &val);
		if (error)
			goto fail;

		if (val == 0xff)
			break;

		if (j < sizeof (sc->sc_temp[i].desc) - 1)
			sc->sc_temp[i].desc[j++] = val;

		if (val == '\0') {
			i++;
			j = 0;
			if (i < sc->sc_num_temp)
				continue;

			break;
		}
	}

	/*
	 * Read fan names.
	 */
	error = lom_write(sc, LOM_IDX_CMD, LOM_IDX_CMD_FAN);
	if (error)
		return (error);

	i = 0;
	j = 0;
	k = LOM_IDX5_FAN_NAME_START;
	while (k <= LOM_IDX5_FAN_NAME_END) {
		error = lom_read(sc, k++, &val);
		if (error)
			goto fail;

		if (val == 0xff)
			break;

		if (j < sizeof (sc->sc_fan[i].desc) - 1)
			sc->sc_fan[i].desc[j++] = val;

		if (val == '\0') {
			i++;
			j = 0;
			if (i < sc->sc_num_fan)
				continue;

			break;
		}
	}

fail:
	lom_write(sc, LOM_IDX_CMD, LOM_IDX_CMD_GENERIC);
	return (error);
}

static void
lom_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct lom_softc *sc = sme->sme_cookie;
	uint32_t i;

	/* Sensor number */
	i = edata->sensor;

	/* Sensor type */
	switch (edata->units) {
	case ENVSYS_INDICATOR:
		if (i < sc->sc_num_alarm)
			lom_refresh_alarm(sc, edata, i);
		else
			lom_refresh_psu(sc, edata,
			    i - sc->sc_num_alarm - sc->sc_num_fan);
		break;
	case ENVSYS_SFANRPM:
		lom_refresh_fan(sc, edata, i - sc->sc_num_alarm);
		break;
	case ENVSYS_STEMP:
		lom_refresh_temp(sc, edata,
		    i - sc->sc_num_alarm - sc->sc_num_fan - sc->sc_num_psu);
		break;
	default:
		edata->state = ENVSYS_SINVALID;
		break;
	}

	/*
	 * If our hostname is set and differs from what's stored in
	 * the LOM, write the new hostname back to the LOM.  Note that
	 * we include the terminating NUL when writing the hostname
	 * back to the LOM, otherwise the LOM will print any trailing
	 * garbage.
	 */
	if (i == 0 && hostnamelen > 0 &&
	    strncmp(sc->sc_hostname, hostname, sizeof(hostname)) != 0) {
		if (sc->sc_type < LOM_LOMLITE2)
			lom1_write_hostname(sc);
		else
			lom2_write_hostname(sc);
		strlcpy(sc->sc_hostname, hostname, sizeof(hostname));
	}
}

static void
lom_refresh_alarm(struct lom_softc *sc, envsys_data_t *edata, uint32_t i)
{
	uint8_t val;

	/* Fault LED or Alarms */
	KASSERT(i < sc->sc_num_alarm);

	/* Read new value at most once every second. */
	if (ratecheck(&sc->sc_alarm_lastread, &refresh_interval)) {
		if (lom_read(sc, LOM_IDX_ALARM, &val)) {
			edata->state = ENVSYS_SINVALID;
			return;
		}
		sc->sc_alarm_lastval = val;
	} else {
		val = sc->sc_alarm_lastval;
	}

	if (i == 0) {
		/* Fault LED */
		if ((val & LOM_ALARM_FAULT) == LOM_ALARM_FAULT)
			edata->value_cur = 0;
		else
			edata->value_cur = 1;
	} else {
		/* Alarms */
		if ((val & (LOM_ALARM_1 << (i - 1))) == 0)
			edata->value_cur = 0;
		else
			edata->value_cur = 1;
	}
	edata->state = ENVSYS_SVALID;
}

static void
lom_refresh_fan(struct lom_softc *sc, envsys_data_t *edata, uint32_t i)
{
	uint8_t val;

	/* Fan speed */
	KASSERT(i < sc->sc_num_fan);

	/* Read new value at most once every second. */
	if (!ratecheck(&sc->sc_fan_lastread[i], &refresh_interval))
		return;

	if (lom_read(sc, LOM_IDX_FAN1 + i, &val)) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = (60 * sc->sc_fan_cal[i] * val) / 100;
		if (val < sc->sc_fan_low[i])
			edata->state = ENVSYS_SCRITICAL;
		else
			edata->state = ENVSYS_SVALID;
	}
}

static void
lom_refresh_psu(struct lom_softc *sc, envsys_data_t *edata, uint32_t i)
{
	uint8_t val;

	/* PSU status */
	KASSERT(i < sc->sc_num_psu);

	/* Read new value at most once every second. */
	if (!ratecheck(&sc->sc_psu_lastread[i], &refresh_interval))
		return;

	if (lom_read(sc, LOM_IDX_PSU1 + i, &val) ||
	    !ISSET(val, LOM_PSU_PRESENT)) {
		edata->state = ENVSYS_SINVALID;
	} else {
		if (val & LOM_PSU_STANDBY) {
			edata->value_cur = 0;
			edata->state = ENVSYS_SVALID;
		} else {
			edata->value_cur = 1;
			if (ISSET(val, LOM_PSU_INPUTA) &&
			    ISSET(val, LOM_PSU_INPUTB) &&
			    ISSET(val, LOM_PSU_OUTPUT))
				edata->state = ENVSYS_SVALID;
			else
				edata->state = ENVSYS_SCRITICAL;
		}
	}
}

static void
lom_refresh_temp(struct lom_softc *sc, envsys_data_t *edata, uint32_t i)
{
	uint8_t val;

	/* Temperature */
	KASSERT(i < sc->sc_num_temp);

	/* Read new value at most once every second. */
	if (!ratecheck(&sc->sc_temp_lastread[i], &refresh_interval))
		return;

	if (lom_read(sc, LOM_IDX_TEMP1 + i, &val)) {
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->value_cur = val * 1000000 + 273150000;
		edata->state = ENVSYS_SVALID;
	}
}

static void
lom1_write_hostname(struct lom_softc *sc)
{
	char name[(LOM1_IDX_HOSTNAME12 - LOM1_IDX_HOSTNAME1 + 1) + 1];
	char *p;
	int i;

	/*
	 * LOMlite generally doesn't have enough space to store the
	 * fully qualified hostname.  If the hostname is too long,
	 * strip off the domain name.
	 */
	strlcpy(name, hostname, sizeof(name));
	if (hostnamelen >= sizeof(name)) {
		p = strchr(name, '.');
		if (p)
			*p = '\0';
	}

	for (i = 0; i < strlen(name) + 1; i++)
		if (lom_write(sc, LOM1_IDX_HOSTNAME1 + i, name[i]))
			break;
}

static void
lom2_write_hostname(struct lom_softc *sc)
{
	int i;

	lom_write(sc, LOM2_IDX_HOSTNAMELEN, hostnamelen + 1);
	for (i = 0; i < hostnamelen + 1; i++)
		lom_write(sc, LOM2_IDX_HOSTNAME, hostname[i]);
}

static int
lom_wdog_tickle(struct sysmon_wdog *smw)
{
	struct lom_softc *sc = smw->smw_cookie;

	/* Pat the dog. */
	sc->sc_wdog_pat.lc_cmd = LOM_IDX_WDOG_CTL | LOM_IDX_WRITE;
	sc->sc_wdog_pat.lc_data = sc->sc_wdog_ctl;
	lom_queue_cmd(sc, &sc->sc_wdog_pat);

	return 0;
}

static int
lom_wdog_setmode(struct sysmon_wdog *smw)
{
	struct lom_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* disable watchdog */
		sc->sc_wdog_ctl &= ~(LOM_WDOG_ENABLE|LOM_WDOG_RESET);
		lom_write(sc, LOM_IDX_WDOG_CTL, sc->sc_wdog_ctl);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = sc->sc_wdog_period;
		else if (smw->smw_period == 0 ||
		    smw->smw_period > LOM_WDOG_TIME_MAX)
			return EINVAL;
		lom_write(sc, LOM_IDX_WDOG_TIME, smw->smw_period);

		/* enable watchdog */
		lom_dequeue_cmd(sc, &sc->sc_wdog_pat);
		sc->sc_wdog_ctl |= LOM_WDOG_ENABLE|LOM_WDOG_RESET;
		sc->sc_wdog_pat.lc_cmd = LOM_IDX_WDOG_CTL | LOM_IDX_WRITE;
		sc->sc_wdog_pat.lc_data = sc->sc_wdog_ctl;
		lom_queue_cmd(sc, &sc->sc_wdog_pat);
	}

	return 0;
}

static bool
lom_shutdown(device_t dev, int how)
{
	struct lom_softc *sc = device_private(dev);

	sc->sc_wdog_ctl &= ~LOM_WDOG_ENABLE;
	lom_write(sc, LOM_IDX_WDOG_CTL, sc->sc_wdog_ctl);
	return true;
}

SYSCTL_SETUP(sysctl_lom_setup, "sysctl hw.lom subtree setup")
{
	const struct sysctlnode *node;

	if (sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL) != 0)
		return;

	hw_node = node->sysctl_num;
}

static int
lom_sysctl_alarm(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct lom_softc *sc;
	int i, tmp, error;
	uint8_t val;

	node = *rnode;
	sc = node.sysctl_data;

	for (i = 0; i < sc->sc_num_alarm; i++) {
		if (node.sysctl_num == sc->sc_sysctl_num[i]) {
			lom_refresh_alarm(sc, &sc->sc_alarm[i], i);
			tmp = sc->sc_alarm[i].value_cur;
			node.sysctl_data = &tmp;
			error = sysctl_lookup(SYSCTLFN_CALL(&node));
			if (error || newp == NULL)
				return error;
			if (tmp < 0 || tmp > 1)
				return EINVAL;

			if (lom_read(sc, LOM_IDX_ALARM, &val))
				return EINVAL;
			if (i == 0) {
				/* Fault LED */
				if (tmp != 0)
					val &= ~LOM_ALARM_FAULT;
				else
					val |= LOM_ALARM_FAULT;
			} else {
				/* Alarms */
				if (tmp != 0)
					val |= LOM_ALARM_1 << (i - 1);
				else
					val &= ~(LOM_ALARM_1 << (i - 1));
			}
			if (lom_write(sc, LOM_IDX_ALARM, val))
				return EINVAL;

			sc->sc_alarm[i].value_cur = tmp;
			return 0;
		}
	}

	return ENOENT;
}
