/*/* $NetBSD: tadpmu.c,v 1.4 2018/10/14 05:08:39 macallan Exp $ */

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

/* a driver for the PMU found in Tadpole Wiper and possibly SPARCle laptops */

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
static struct sysmon_envsys *tadpmu_sme;
static envsys_data_t tadpmu_sensors[5];
static uint8_t idata = 0xff;
static uint8_t ivalid = 0;
static wchan_t tadpmu, tadpmuev;
static struct sysmon_pswitch tadpmu_pbutton, tadpmu_lidswitch;
static kmutex_t tadpmu_lock;
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
		} else {
			edata->value_cur = res;
		}
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}
}

static void
tadpmu_events(void *cookie)
{
	uint8_t res, ores = 0;
	while (!tadpmu_dying) {
		mutex_enter(&tadpmu_lock);
		tadpmu_flush();
		tadpmu_send_cmd(CMD_READ_GENSTAT);
		res = tadpmu_recv();
		mutex_exit(&tadpmu_lock);
		res &= GENSTAT_LID_CLOSED;
		if (res != ores) {
			ores = res;
			sysmon_pswitch_event(&tadpmu_lidswitch, 
				    (res & GENSTAT_LID_CLOSED) ? 
				        PSWITCH_EVENT_PRESSED :
				        PSWITCH_EVENT_RELEASED);
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
		DPRINTF("status change %02x\n", d);
		switch (d) {
			case TADPMU_POWERBUTTON:
				sysmon_pswitch_event(&tadpmu_pbutton, 
				    PSWITCH_EVENT_PRESSED);
				break;
			case TADPMU_LID:
				/* read genstat and report lid */
				wakeup(tadpmuev);
				break;
		}
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
	int ver;

	tadpmu_iot = t;
	tadpmu_hcmd = hcmd;
	tadpmu_hdata = hdata;

	tadpmu_flush();
	delay(1000);

	tadpmu_send_cmd(CMD_READ_VERSION);
	ver = tadpmu_recv();
	printf("Tadpole PMU Version 1.%d\n", ver);	

	tadpmu_send_cmd(CMD_SET_OPMODE);
	tadpmu_send(0x75);

	tadpmu_send_cmd(CMD_READ_SYSTEMP);
	ver = tadpmu_recv();
	printf("Temperature %d\n", ver);	

	tadpmu_send_cmd(CMD_READ_VBATT);
	ver = tadpmu_recv();
	printf("Battery voltage %d\n", ver);	

	tadpmu_send_cmd(CMD_READ_GENSTAT);
	ver = tadpmu_recv();
	printf("status %02x\n", ver);

	mutex_init(&tadpmu_lock, MUTEX_DEFAULT, IPL_NONE);

	tadpmu_sme = sysmon_envsys_create();
	tadpmu_sme->sme_name = "tadpmu";
	tadpmu_sme->sme_cookie = NULL;
	tadpmu_sme->sme_refresh = tadpmu_sensors_refresh;

	tadpmu_sensors[0].units = ENVSYS_STEMP;
	tadpmu_sensors[0].private = CMD_READ_SYSTEMP;
	strcpy(tadpmu_sensors[0].desc, "systemp");
	sysmon_envsys_sensor_attach(tadpmu_sme, &tadpmu_sensors[0]);
#ifdef TADPMU_DEBUG
	tadpmu_sensors[1].units = ENVSYS_INTEGER;
	tadpmu_sensors[1].private = 0x17;
	strcpy(tadpmu_sensors[1].desc, "reg 17");
	sysmon_envsys_sensor_attach(tadpmu_sme, &tadpmu_sensors[1]);
	tadpmu_sensors[2].units = ENVSYS_INTEGER;
	tadpmu_sensors[2].private = 0x18;
	strcpy(tadpmu_sensors[2].desc, "reg 18");
	sysmon_envsys_sensor_attach(tadpmu_sme, &tadpmu_sensors[2]);
	tadpmu_sensors[3].units = ENVSYS_INTEGER;
	tadpmu_sensors[3].private = CMD_READ_GENSTAT;
	strcpy(tadpmu_sensors[3].desc, "genstat");
	sysmon_envsys_sensor_attach(tadpmu_sme, &tadpmu_sensors[3]);
#if 0
	tadpmu_sensors[4].units = ENVSYS_INTEGER;
	tadpmu_sensors[4].private = CMD_READ_VBATT;
	strcpy(tadpmu_sensors[4].desc, "Vbatt");
	sysmon_envsys_sensor_attach(tadpmu_sme, &tadpmu_sensors[4]);
#endif
#endif
	sysmon_envsys_register(tadpmu_sme);

	sysmon_task_queue_init();
	memset(&tadpmu_pbutton, 0, sizeof(struct sysmon_pswitch));
	tadpmu_pbutton.smpsw_name = "power";
	tadpmu_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&tadpmu_pbutton) != 0)
		aprint_error(
		    "unable to register power button with sysmon\n");

	tadpmu_lidswitch.smpsw_name = "lid";
	tadpmu_lidswitch.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&tadpmu_lidswitch) != 0)
		aprint_error(
		    "unable to register lid switch with sysmon\n");

	kthread_create(PRI_NONE, 0, curcpu(), tadpmu_events, NULL,
	    &tadpmu_thread, "tadpmu_events"); 

	return 0;
}
#endif /* HAVE_TADPMU */
