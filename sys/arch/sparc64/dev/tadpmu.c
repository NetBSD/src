/*/* $NetBSD: tadpmu.c,v 1.5 2020/05/16 07:16:14 jdc Exp $ */

/*-
 * Copyright (c) 2018 Michael Lorenz <macallan@netbsd.org>
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

/* a driver for the PMU found in Tadpole Viper and SPARCle laptops */

#include "opt_tadpmu.h"
#ifdef HAVE_TADPMU
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kthread.h>
#include <sys/mutex.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <sparc64/dev/tadpmureg.h>
#include <sparc64/dev/tadpmuvar.h>

#ifdef TADPMU_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

static bus_space_tag_t tadpmu_iot;
static bus_space_handle_t tadpmu_hcmd;
static bus_space_handle_t tadpmu_hdata;
static struct sysmon_envsys *tadpmu_sens_sme;
static struct sysmon_envsys *tadpmu_acad_sme;
static struct sysmon_envsys *tadpmu_batt_sme;
static envsys_data_t tadpmu_sensors[8];
static uint8_t idata = 0xff;
static uint8_t ivalid = 0;
static uint8_t ev_data = 0;
static wchan_t tadpmu, tadpmuev;
static struct sysmon_pswitch tadpmu_pbutton, tadpmu_lidswitch, tadpmu_dcpower;
static kmutex_t tadpmu_lock, data_lock;
static lwp_t *tadpmu_thread;
static int tadpmu_dying = 0;

static inline void
tadpmu_cmd(uint8_t d)
{
	bus_space_write_1(tadpmu_iot, tadpmu_hcmd, 0, d);
}

static inline void
tadpmu_wdata(uint8_t d)
{
	bus_space_write_1(tadpmu_iot, tadpmu_hdata, 0, d);
}

static inline uint8_t
tadpmu_status(void)
{
	return bus_space_read_1(tadpmu_iot, tadpmu_hcmd, 0);
}

static inline uint8_t
tadpmu_data(void)
{
	return bus_space_read_1(tadpmu_iot, tadpmu_hdata, 0);
}

static void
tadpmu_flush(void)
{
	volatile uint8_t junk, d;
	int bail = 0;

	d = tadpmu_status();
	while (d & STATUS_HAVE_DATA) {
		junk = tadpmu_data();
		__USE(junk);
		delay(10);
		bail++;
		if (bail > 100) {
			printf("%s: timeout waiting for data out to clear %2x\n",
			    __func__, d);
			break;
		}
		d = tadpmu_status();
	}
	bail = 0;
	d = tadpmu_status();
	while (d & STATUS_SEND_DATA) {
		bus_space_write_1(tadpmu_iot, tadpmu_hdata, 0, 0);
		delay(10);
		bail++;
		if (bail > 100) {
			printf("%s: timeout waiting for data in to clear %02x\n",
			    __func__, d);
			break;
		}
		d = tadpmu_status();
	}
}

static void
tadpmu_send_cmd(uint8_t cmd)
{
	int bail = 0;
	uint8_t d;

	ivalid = 0;
	tadpmu_cmd(cmd);

	d = tadpmu_status();
	while ((d & STATUS_CMD_IN_PROGRESS) == 0) {
		delay(10);
		bail++;
		if (bail > 100) {
			printf("%s: timeout waiting for command to start\n",
			    __func__);
			break;
		}
		d = tadpmu_status();
	}
}

static uint8_t
tadpmu_recv(void)
{
	int bail = 0;
	uint8_t d;

	if (cold) {
		d = tadpmu_status();	
		while ((d & STATUS_HAVE_DATA) == 0) {
			delay(10);
			bail++;
			if (bail > 1000) {
				printf("%s: timeout waiting for data %02x\n",
				    __func__, d);
				break;
			}
			d = tadpmu_status();
		}
		return bus_space_read_1(tadpmu_iot, tadpmu_hdata, 0);
	} else {
		while (ivalid == 0)
			tsleep(tadpmu, 0, "pmucmd", 1);
		return idata;
	}
}

static void
tadpmu_send(uint8_t v)
{
	int bail = 0;
	uint8_t d;

	d = tadpmu_status();	
	while ((d & STATUS_SEND_DATA) == 0) {
		delay(10);
		bail++;
		if (bail > 1000) {
			printf("%s: timeout waiting for PMU ready %02x\n", __func__, d);
			break;
		}
		d = tadpmu_status();
	}

	tadpmu_wdata(v);

	while ((d & STATUS_SEND_DATA) != 0) {
		delay(10);
		bail++;
		if (bail > 1000) {
			printf("%s: timeout waiting for accept data %02x\n", __func__, d);
			break;
		}
		d = tadpmu_status();
	}
}

static uint32_t
tadpmu_battery_capacity(uint8_t gstat)
{
	uint8_t res;

	if (gstat == GENSTAT_STATE_BATTERY_FULL) {
		return ENVSYS_BATTERY_CAPACITY_NORMAL;
	}

	mutex_enter(&tadpmu_lock);
	tadpmu_flush();
	tadpmu_send_cmd(CMD_READ_VBATT);
	res = tadpmu_recv();
	mutex_exit(&tadpmu_lock);

	if (gstat & GENSTAT_STATE_BATTERY_DISCHARGE) {
		if (res < TADPMU_BATT_DIS_CAP_CRIT)
			return ENVSYS_BATTERY_CAPACITY_CRITICAL;
		if (res < TADPMU_BATT_DIS_CAP_WARN)
			return ENVSYS_BATTERY_CAPACITY_WARNING;
		if (res < TADPMU_BATT_DIS_CAP_LOW)
			return ENVSYS_BATTERY_CAPACITY_LOW;
		else
			return ENVSYS_BATTERY_CAPACITY_NORMAL;
	} else if (gstat == GENSTAT_STATE_BATTERY_CHARGE) {
		if (res < TADPMU_BATT_CHG_CAP_CRIT)
			return ENVSYS_BATTERY_CAPACITY_CRITICAL;
		else if (res < TADPMU_BATT_CHG_CAP_WARN)
			return ENVSYS_BATTERY_CAPACITY_WARNING;
		else if (res < TADPMU_BATT_CHG_CAP_LOW)
			return ENVSYS_BATTERY_CAPACITY_LOW;
		else
			return ENVSYS_BATTERY_CAPACITY_NORMAL;
	} else {
		DPRINTF("%s unknown battery state %02x\n",
		    __func__, gstat);
		return ENVSYS_BATTERY_CAPACITY_NORMAL;
	}
}

/* The data to read is calculated from the command and the units */
static void
tadpmu_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	int res;

	if (edata->private > 0) {
		mutex_enter(&tadpmu_lock);
		tadpmu_flush();
		tadpmu_send_cmd(edata->private);
		res = tadpmu_recv();
		mutex_exit(&tadpmu_lock);
		if (edata->units == ENVSYS_STEMP) {
			edata->value_cur = res * 1000000 + 273150000;
		} else if (edata->units == ENVSYS_SVOLTS_DC) {
			edata->value_cur = res * 100000;
		} else if (edata->units == ENVSYS_BATTERY_CHARGE) {
			if (res & GENSTAT_BATTERY_CHARGING)
				edata->value_cur = ENVSYS_INDICATOR_TRUE;
			else
				edata->value_cur = ENVSYS_INDICATOR_FALSE;
		} else if (edata->units == ENVSYS_BATTERY_CAPACITY) {
			edata->value_cur = tadpmu_battery_capacity(res);
		} else {
			if (edata->units == ENVSYS_INDICATOR &&
			    edata->private == CMD_READ_GENSTAT) {
				if (res & GENSTAT_DC_PRESENT)
					edata->value_cur =
					    ENVSYS_INDICATOR_TRUE;
				else
					edata->value_cur =
					    ENVSYS_INDICATOR_FALSE;
			} else {
				edata->value_cur = res;
			}
		}
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}
}

static void
tadpmu_events(void *cookie)
{
	uint8_t events, gs, vb;

	while (!tadpmu_dying) {
		mutex_enter(&tadpmu_lock);
		tadpmu_flush();
		tadpmu_send_cmd(CMD_READ_GENSTAT);
		gs = tadpmu_recv();
		tadpmu_send_cmd(CMD_READ_VBATT);
		vb = tadpmu_recv();
		mutex_exit(&tadpmu_lock);

		mutex_enter(&data_lock);
		events = ev_data;
		mutex_exit(&data_lock);
		DPRINTF("%s event %02x, status %02x/%02x\n", __func__,
		    events, gs, vb);

		if (events & TADPMU_EV_PWRBUTT) {
			mutex_enter(&data_lock);
			ev_data &= ~TADPMU_EV_PWRBUTT;
			mutex_exit(&data_lock);
			sysmon_pswitch_event(&tadpmu_pbutton,
			    PSWITCH_EVENT_PRESSED);
		}

		if (events & TADPMU_EV_LID) {
			mutex_enter(&data_lock);
			ev_data &= ~TADPMU_EV_LID;
			mutex_exit(&data_lock);
			sysmon_pswitch_event(&tadpmu_lidswitch, 
			    gs & GENSTAT_LID_CLOSED ? 
		            PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
		}

		if (events & TADPMU_EV_DCPOWER) {
			mutex_enter(&data_lock);
			ev_data &= ~TADPMU_EV_DCPOWER;
			mutex_exit(&data_lock);
			sysmon_pswitch_event(&tadpmu_dcpower, 
			    gs & GENSTAT_DC_PRESENT ? 
		            PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
		}

		if (events & TADPMU_EV_BATTCHANGE) {
			mutex_enter(&data_lock);
			ev_data &= ~TADPMU_EV_BATTCHANGE;
			mutex_exit(&data_lock);
			if (gs == GENSTAT_STATE_BATTERY_DISCHARGE) {
				if (vb < TADPMU_BATT_DIS_CAP_CRIT)
					printf("Battery critical!\n");
				else if (vb < TADPMU_BATT_DIS_CAP_WARN)
					printf("Battery warning!\n");
			}
		}

		if (events & TADPMU_EV_BATTCHARGED) {
			mutex_enter(&data_lock);
			ev_data &= ~TADPMU_EV_BATTCHARGED;
			mutex_exit(&data_lock);
			printf("Battery charged\n");
		}

		tsleep(tadpmuev, 0, "tadpmuev", hz); 		
	}
	kthread_exit(0);
}

int
tadpmu_intr(void *cookie)
{
	uint8_t s = tadpmu_status(), d;
	if (s & STATUS_INTR) {
		/* interrupt message */
		d = tadpmu_data();
		DPRINTF("%s status change %02x\n", __func__, d);

		switch (d) {
			case TADPMU_INTR_POWERBUTTON:
				mutex_enter(&data_lock);
				ev_data |= TADPMU_EV_PWRBUTT;;
				mutex_exit(&data_lock);
				break;
			case TADPMU_INTR_LID:
				mutex_enter(&data_lock);
				ev_data |= TADPMU_EV_LID;
				mutex_exit(&data_lock);
				break;
			case TADPMU_INTR_DCPOWER:
				mutex_enter(&data_lock);
				ev_data |= TADPMU_EV_DCPOWER;
				mutex_exit(&data_lock);
				break;
			case TADPMU_INTR_BATTERY_STATE:
				mutex_enter(&data_lock);
				ev_data |= TADPMU_EV_BATTCHANGE;
				mutex_exit(&data_lock);
				break;
			case TADPMU_INTR_BATTERY_CHARGED:
				mutex_enter(&data_lock);
				ev_data |= TADPMU_EV_BATTCHARGED;
				mutex_exit(&data_lock);
				break;
		}
		/* Report events */
		if (ev_data)
			wakeup(tadpmuev);
	}
	s = tadpmu_status();
	if (s & STATUS_HAVE_DATA) {
		idata = tadpmu_data();
		ivalid = 1;
		wakeup(tadpmu);
		DPRINTF("%s data %02x\n", __func__, idata);
	}
	
	return 1;
}

int 
tadpmu_init(bus_space_tag_t t, bus_space_handle_t hcmd, bus_space_handle_t hdata)
{
	uint8_t ver;

	tadpmu_iot = t;
	tadpmu_hcmd = hcmd;
	tadpmu_hdata = hdata;

	tadpmu_flush();
	delay(1000);

	tadpmu_send_cmd(CMD_READ_VERSION);
	ver = tadpmu_recv();
	printf("Tadpole PMU Version 1.%d\n", ver);	

	tadpmu_send_cmd(CMD_SET_OPMODE);
	tadpmu_send(OPMODE_UNIX);

#ifdef TADPMU_DEBUG
	tadpmu_send_cmd(CMD_READ_SYSTEMP);
	ver = tadpmu_recv();
	printf("Temperature 0x%02x\n", ver);	

	tadpmu_send_cmd(CMD_READ_VBATT);
	ver = tadpmu_recv();
	printf("Battery voltage 0x%02x\n", ver);	

	tadpmu_send_cmd(CMD_READ_GENSTAT);
	ver = tadpmu_recv();
	printf("status 0x%02x\n", ver);
#endif

	mutex_init(&tadpmu_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&data_lock, MUTEX_DEFAULT, IPL_HIGH);

	tadpmu_sens_sme = sysmon_envsys_create();
	tadpmu_sens_sme->sme_name = "tadpmu";
	tadpmu_sens_sme->sme_cookie = NULL;
	tadpmu_sens_sme->sme_refresh = tadpmu_sensors_refresh;

	tadpmu_acad_sme = sysmon_envsys_create();
	tadpmu_acad_sme->sme_name = "ac adapter";
	tadpmu_acad_sme->sme_cookie = NULL;
	tadpmu_acad_sme->sme_refresh = tadpmu_sensors_refresh;
	tadpmu_acad_sme->sme_class = SME_CLASS_ACADAPTER;

	tadpmu_batt_sme = sysmon_envsys_create();
	tadpmu_batt_sme->sme_name = "battery";
	tadpmu_batt_sme->sme_cookie = NULL;
	tadpmu_batt_sme->sme_refresh = tadpmu_sensors_refresh;
	tadpmu_batt_sme->sme_class = SME_CLASS_BATTERY;

	tadpmu_sensors[0].state = ENVSYS_SINVALID;
	tadpmu_sensors[0].units = ENVSYS_STEMP;
	tadpmu_sensors[0].private = CMD_READ_SYSTEMP;
	strcpy(tadpmu_sensors[0].desc, "systemp");
	sysmon_envsys_sensor_attach(tadpmu_sens_sme, &tadpmu_sensors[0]);

	tadpmu_sensors[1].state = ENVSYS_SINVALID;
	tadpmu_sensors[1].units = ENVSYS_INDICATOR;
	tadpmu_sensors[1].private = CMD_READ_FAN_EN;
	strcpy(tadpmu_sensors[1].desc, "fan on");
	sysmon_envsys_sensor_attach(tadpmu_sens_sme, &tadpmu_sensors[1]);

	tadpmu_sensors[2].state = ENVSYS_SINVALID;
	tadpmu_sensors[2].units = ENVSYS_INDICATOR;
	tadpmu_sensors[2].private = CMD_READ_GENSTAT;
	strcpy(tadpmu_sensors[2].desc, "DC power");
	sysmon_envsys_sensor_attach(tadpmu_acad_sme, &tadpmu_sensors[2]);

	tadpmu_sensors[3].state = ENVSYS_SINVALID;
	tadpmu_sensors[3].units = ENVSYS_SVOLTS_DC;
	tadpmu_sensors[3].private = CMD_READ_VBATT;
	strcpy(tadpmu_sensors[3].desc, "Vbatt");
	sysmon_envsys_sensor_attach(tadpmu_batt_sme, &tadpmu_sensors[3]);

	tadpmu_sensors[4].state = ENVSYS_SINVALID;
	tadpmu_sensors[4].units = ENVSYS_BATTERY_CAPACITY;
	tadpmu_sensors[4].private = CMD_READ_GENSTAT;
	/* We must provide an initial value for battery capacity */
	tadpmu_sensors[4].value_cur = ENVSYS_BATTERY_CAPACITY_NORMAL;
	strcpy(tadpmu_sensors[4].desc, "capacity");
	sysmon_envsys_sensor_attach(tadpmu_batt_sme, &tadpmu_sensors[4]);

	tadpmu_sensors[5].state = ENVSYS_SINVALID;
	tadpmu_sensors[5].units = ENVSYS_BATTERY_CHARGE;
	tadpmu_sensors[5].private = CMD_READ_GENSTAT;
	strcpy(tadpmu_sensors[5].desc, "charging");
	sysmon_envsys_sensor_attach(tadpmu_batt_sme, &tadpmu_sensors[5]);

#ifdef TADPMU_DEBUG
	tadpmu_sensors[6].state = ENVSYS_SINVALID;
	tadpmu_sensors[6].units = ENVSYS_INTEGER;
	tadpmu_sensors[6].private = CMD_READ_GENSTAT;
	strcpy(tadpmu_sensors[6].desc, "genstat");
	sysmon_envsys_sensor_attach(tadpmu_sens_sme, &tadpmu_sensors[6]);
#endif

	sysmon_envsys_register(tadpmu_sens_sme);
	sysmon_envsys_register(tadpmu_acad_sme);
	sysmon_envsys_register(tadpmu_batt_sme);

	sysmon_task_queue_init();
	memset(&tadpmu_pbutton, 0, sizeof(struct sysmon_pswitch));
	tadpmu_pbutton.smpsw_name = "power";
	tadpmu_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&tadpmu_pbutton) != 0)
		aprint_error(
		    "unable to register power button with sysmon\n");

	memset(&tadpmu_lidswitch, 0, sizeof(struct sysmon_pswitch));
	tadpmu_lidswitch.smpsw_name = "lid";
	tadpmu_lidswitch.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&tadpmu_lidswitch) != 0)
		aprint_error(
		    "unable to register lid switch with sysmon\n");

	memset(&tadpmu_dcpower, 0, sizeof(struct sysmon_pswitch));
	tadpmu_dcpower.smpsw_name = "AC adapter";
	tadpmu_dcpower.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&tadpmu_dcpower) != 0)
		aprint_error(
		    "unable to register AC adapter with sysmon\n");

	kthread_create(PRI_NONE, 0, curcpu(), tadpmu_events, NULL,
	    &tadpmu_thread, "tadpmu_events"); 

	return 0;
}
#endif /* HAVE_TADPMU */
