/*	$NetBSD: swsensor.c,v 1.3 2010/10/20 19:21:04 pooka Exp $ */
/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: swsensor.c,v 1.3 2010/10/20 19:21:04 pooka Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmonvar.h>

#include <prop/proplib.h>

int swsensorattach(int);

static struct sysctllog *swsensor_sysctllog = NULL;

static int sensor_value_sysctl = 0;

static struct sysmon_envsys *swsensor_sme;
static envsys_data_t swsensor_edata;

static int32_t sw_sensor_value;

MODULE(MODULE_CLASS_DRIVER, swsensor, NULL);

/*
 * Set-up the sysctl interface for setting the sensor's cur_value
 */

static
void
sysctl_swsensor_setup(void)
{
	int ret;
	int node_sysctl_num;
	const struct sysctlnode *me = NULL;

	KASSERT(swsensor_sysctllog == NULL);

	ret = sysctl_createv(&swsensor_sysctllog, 0, NULL, &me,
			     CTLFLAG_READWRITE,
			     CTLTYPE_NODE, "swsensor", NULL,
			     NULL, 0, NULL, 0,
			     CTL_HW, CTL_CREATE, CTL_EOL);
	if (ret != 0)
		return;

	node_sysctl_num = me->sysctl_num;
	ret = sysctl_createv(&swsensor_sysctllog, 0, NULL, &me,
			     CTLFLAG_READWRITE,
			     CTLTYPE_INT, "cur_value", NULL,
			     NULL, 0, &sw_sensor_value, 0,
			     CTL_HW, node_sysctl_num, CTL_CREATE, CTL_EOL);

	if (ret == 0)
		sensor_value_sysctl = me->sysctl_num;
}

/*
 * "Polling" routine to update sensor value
 */
static
void
swsensor_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{

	edata->value_cur = sw_sensor_value;
	edata->state = ENVSYS_SVALID;
}

/*
 * Module management
 */

static
int
swsensor_init(void *arg)
{
	int error;
	prop_dictionary_t pd = (prop_dictionary_t)arg;
	prop_object_t po = NULL;
	int pv;

	swsensor_sme = sysmon_envsys_create();
	if (swsensor_sme == NULL)
		return ENOTTY;

	swsensor_sme->sme_name = "swsensor";
	swsensor_sme->sme_cookie = &swsensor_edata;
	swsensor_sme->sme_refresh = swsensor_refresh;
	swsensor_sme->sme_set_limits = NULL;
	swsensor_sme->sme_get_limits = NULL;

	/* See if prop dictionary supplies a sensor type */
	if (pd != NULL)
		po = prop_dictionary_get(pd, "type");

	pv = -1;
	if (po != NULL && prop_object_type(po) == PROP_TYPE_NUMBER)
		swsensor_edata.units = prop_number_integer_value(po);
	else
		swsensor_edata.units = ENVSYS_INTEGER;

	/* See if prop dictionary supplies sensor flags */
	if (pd != NULL)
		po = prop_dictionary_get(pd, "flags");

	pv = -1;
	if (po != NULL && prop_object_type(po) == PROP_TYPE_NUMBER)
		swsensor_edata.flags = prop_number_integer_value(po);
	else
		swsensor_edata.flags = 0;

	swsensor_edata.value_cur = 0;
	strlcpy(swsensor_edata.desc, "sensor", ENVSYS_DESCLEN);

	error = sysmon_envsys_sensor_attach(swsensor_sme, &swsensor_edata);

	if (error == 0)
		error = sysmon_envsys_register(swsensor_sme);
	else {
		printf("sysmon_envsys_sensor_attach failed: %d\n", error);
		return error;
	}

	if (error == 0)
		sysctl_swsensor_setup();
	else
		printf("sysmon_envsys_register failed: %d\n", error);

	return error;
}

static
int
swsensor_fini(void *arg)
{

	sysmon_envsys_unregister(swsensor_sme);

	sysctl_teardown(&swsensor_sysctllog);

	return 0;
}

static
int
swsensor_modcmd(modcmd_t cmd, void *arg)
{
	int ret;

	switch (cmd) {
	case MODULE_CMD_INIT:
		ret = swsensor_init(arg);
		break;

	case MODULE_CMD_FINI:
		ret = swsensor_fini(arg);
		break;

	case MODULE_CMD_STAT:
	default:
		ret = ENOTTY;
	}

	return ret;
}
