/* $NetBSD: power.c,v 1.6 2007/07/11 21:57:29 dsl Exp $ */
/*
 * Copyright (c) 2004 Jochen Kunz.
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
 * 3. The name of Jochen Kunz may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOCHEN KUNZ
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL JOCHEN KUNZ
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support for soft power switch found on most hp700 machines.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: power.c,v 1.6 2007/07/11 21:57:29 dsl Exp $");


/*
 * There is no software power switch control available at all when 
 * PDC_SOFT_POWER / PDC_SOFT_POWER_INFO fails.
 *
 * There are two ways of controling power depending on the machine type:
 * - 712 style: We have to use the DR25_PCXL_POWFAIL bit in CPU Diagnose
 *   Register 25 of the PCX-L / PA7100LC CPU to get the status of the power
 *   switch and the LASI power register to control the switch. After a
 *   successful call to PDC_SOFT_POWER / PDC_SOFT_POWER_INFO
 *   pdc_power_info.addr is == 0 on this machines. This was introduced with
 *   the 712, hence 712 style.
 * - B/C-Class style: If PDC_SOFT_POWER / PDC_SOFT_POWER_INFO is successful
 *   pdc_power_info.addr contains the HPA of the power switch status 
 *   register and the power switch is controled via PDC_SOFT_POWER / 
 *   PDC_SOFT_POWER_ENABLE.
 *
 * There is no way of asynchronous notification when the power switch was 
 * switched. Thus we have to poll the status of the switch. (There are rumors
 * about a power interrupt facility on B/C-Class style machines, but I found
 * no documentation about it.)
 *
 * The DR25_PCXL_POWFAIL bit on 712 style machines is a straight line from
 * the hardware, so we have to do dampening in software. This is accomplished
 * by polling it multiple times a second (default 0.2 Hz) and alter the state
 * variable if the state of the DR25_PCXL_POWFAIL bit is constant over a 
 * period of one second.
 * The PWR_SW_REG_POWFAIL bit in the power switch status register is dampened
 * in hardware. So we poll this bit once a second.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/power.h>
#include <sys/sysctl.h>

#include <dev/sysmon/sysmon_taskq.h>
#include <dev/sysmon/sysmonvar.h>

#include <machine/reg.h>
#include <machine/pdc.h>
#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <hp700/hp700/power.h>

#define PWR_SW_REG_POWFAIL		1
#define LASI_PWR_SW_REG_CTRL_DISABLE	1
#define LASI_PWR_SW_REG_CTRL_ENABLE	0

volatile uint32_t *lasi_pwr_sw_reg;
static bus_space_tag_t pwr_sw_reg_bst;
static bus_space_handle_t pwr_sw_reg_bsh;
int pwr_sw_state;
static int pwr_sw_control;
static const char *pwr_sw_state_str[] = {"off", "on"};
static const char *pwr_sw_control_str[] = {"disabled", "enabled", "locked"};
static int pwr_sw_poll_interval; 
static int pwr_sw_count;
static struct sysmon_pswitch *pwr_sw_sysmon;
static struct callout pwr_sw_callout;


static int pwr_sw_sysctl_state(SYSCTLFN_PROTO);
static int pwr_sw_sysctl_ctrl(SYSCTLFN_PROTO);
static void pwr_sw_sysmon_cb(void *);
static void pwr_sw_poll(void *);


void
pwr_sw_init(bus_space_tag_t bst)
{
	struct pdc_power_info pdc_power_info PDC_ALIGNMENT;
	struct sysctllog *sysctl_log;
	const struct sysctlnode *pwr_sw_node;
	int error, stage;

	pwr_sw_state = 1;
	/*
	 * Ensure that we have a valid lasi_pwr_sw_reg pointer and that we 
	 * are on a PCX-L / PA7100LC CPU if it is a 712 style machine.
	 */
	if (pdc_call((iodcio_t)pdc, 0, PDC_SOFT_POWER, PDC_SOFT_POWER_INFO,
	    &pdc_power_info, 0) < 0 ||
	    (pdc_power_info.addr == 0 &&
	     (strcmp(hppa_cpu_info->hppa_cpu_info_chip_type, "PCX-L") != 0 ||
	      lasi_pwr_sw_reg == NULL))) {
		printf("No soft power available.\n");
		pwr_sw_reg_bst = NULL;
		lasi_pwr_sw_reg = NULL;
		return;
	}
	if (pdc_power_info.addr != 0) {
		pwr_sw_reg_bst = bst;
		if (bus_space_map(pwr_sw_reg_bst, pdc_power_info.addr, 4, 0,
		    &pwr_sw_reg_bsh) != 0) {
			printf("Can't map power switch status register.\n");
			return;
		}
	}

#ifdef DEBUG
	if (pwr_sw_reg_bst != NULL)
		printf("pwr_sw_init: pdc_power_info.addr=0x%x\n",
		    pdc_power_info.addr);
	else
		printf("pwr_sw_init: lasi_pwr_sw_reg=%p\n", lasi_pwr_sw_reg);
#endif /* DEBUG */

	stage = 0;
	error = sysctl_createv(&sysctl_log, 0, NULL, NULL, 0, 
	    CTLTYPE_NODE, "machdep", NULL, NULL, 0, NULL, 0, 
	    CTL_MACHDEP, CTL_EOL);
	if (error == 0) {
		stage = 1;
		error = sysctl_createv(&sysctl_log, 0, NULL, &pwr_sw_node, 0, 
		    CTLTYPE_NODE, "power_switch", NULL, NULL, 0, NULL, 0,
		    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	}
	if (error == 0)
		error = sysctl_createv(&sysctl_log, 0, NULL, NULL, 
		    CTLFLAG_READONLY, CTLTYPE_STRING, "state", NULL, 
		    pwr_sw_sysctl_state, 0, NULL, 16, 
		    CTL_MACHDEP, pwr_sw_node->sysctl_num, CTL_CREATE, CTL_EOL);
	if (error == 0)
		error = sysctl_createv(&sysctl_log, 0, NULL, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_STRING, "control", NULL,
		    pwr_sw_sysctl_ctrl, 0, NULL, 16,
		    CTL_MACHDEP, pwr_sw_node->sysctl_num, CTL_CREATE, CTL_EOL);

	if (error == 0) {
		if ((pwr_sw_sysmon = malloc(sizeof(struct sysmon_pswitch),
		    M_DEVBUF, 0)) == NULL)
			error = 1;
	} else
		printf("Can't create sysctl machdep.power_switch\n");

	if (error == 0) {
		stage = 2;
		memset(pwr_sw_sysmon, 0, sizeof(struct sysmon_pswitch));
		pwr_sw_sysmon->smpsw_name = "power switch";
		pwr_sw_sysmon->smpsw_type = PSWITCH_TYPE_POWER;
		error = sysmon_pswitch_register(pwr_sw_sysmon);
	} else 
		printf("Can't malloc sysmon power switch.\n");
	if (error == 0) {
		sysmon_task_queue_init();
		if (pwr_sw_reg_bst == NULL)
			/* Diag Reg. needs software dampening, poll at 0.2 Hz.*/
			pwr_sw_poll_interval = hz / 5;
		else
			/* Power Reg. is hardware dampened, poll at 1 Hz. */
			pwr_sw_poll_interval = hz;
		callout_init(&pwr_sw_callout, 0);
		callout_reset(&pwr_sw_callout, pwr_sw_poll_interval, 
		    pwr_sw_poll, NULL);
		return;
	} else
		printf("Can't register power switch with sysmon.\n");

	switch (stage) {
	case 2:
		free(pwr_sw_sysmon, M_DEVBUF);
		/* FALL THROUGH */
	case 1:
		sysctl_teardown(&sysctl_log);
		/* FALL THROUGH */
	}
}


static void
pwr_sw_sysmon_cb(void *not_used) 
{
	sysmon_pswitch_event(pwr_sw_sysmon, PSWITCH_EVENT_PRESSED);
}



static void
pwr_sw_poll(void *not_used)
{
	uint32_t reg;

	if (pwr_sw_reg_bst == NULL) {
		/* Get Power Fail status from CPU Diagnose Register 25 */
		mfcpu(25, reg);
		reg &= 1 << DR25_PCXL_POWFAIL;
		if (reg == (pwr_sw_state != 0 ? 0 : 1 << DR25_PCXL_POWFAIL))
			pwr_sw_count++;
		else
			pwr_sw_count = 0;
		if (pwr_sw_count == hz / pwr_sw_poll_interval)
			pwr_sw_state = reg == 0 ? 0 : 1;
	} else {
		if ((bus_space_read_4(pwr_sw_reg_bst, pwr_sw_reg_bsh, 0) & 
		    PWR_SW_REG_POWFAIL) == 0)
			pwr_sw_state = 0;
		else
			pwr_sw_state = 1;
	}

	if (pwr_sw_state == 0 && pwr_sw_control != PWR_SW_CTRL_LOCK)
		sysmon_task_queue_sched(0, pwr_sw_sysmon_cb, NULL);
	else
		callout_reset(&pwr_sw_callout, pwr_sw_poll_interval, 
		    pwr_sw_poll, NULL);
}


void
pwr_sw_ctrl(int enable)
{
	struct pdc_power_info pdc_power_info PDC_ALIGNMENT;
	int error;

#ifdef DEBUG
	printf("pwr_sw_control=%d enable=%d\n", pwr_sw_control, enable);
#endif /* DEBUG */
	if (cold != 0) {
		if (panicstr)
			return;
		panic("pwr_sw_ctrl can only be called when machine is warm!");
	}
	if (pwr_sw_reg_bst == NULL && lasi_pwr_sw_reg == NULL)
		return;
	if (enable < PWR_SW_CTRL_MIN || enable > PWR_SW_CTRL_MAX)
		panic("invalid power state in pwr_sw_control: %d", enable);
	pwr_sw_control = enable;
	if (pwr_sw_reg_bst == NULL) {
		if (enable == PWR_SW_CTRL_DISABLE)
			*lasi_pwr_sw_reg = LASI_PWR_SW_REG_CTRL_DISABLE;
		else
			*lasi_pwr_sw_reg = LASI_PWR_SW_REG_CTRL_ENABLE;
	} else {
		error = pdc_call((iodcio_t)pdc, 0, PDC_SOFT_POWER,
		    PDC_SOFT_POWER_ENABLE, &pdc_power_info, 
		    enable == 0 ? 0 : 1);
		if (error != 0)
			printf("pwr_sw_control: failed errorcode=%d\n", error);
	}
}


int
pwr_sw_sysctl_state(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = __UNCONST(pwr_sw_state_str[pwr_sw_state]);
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}


int
pwr_sw_sysctl_ctrl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int i, error;
	char val[16];

	node = *rnode;
	strcpy(val, pwr_sw_control_str[pwr_sw_control]);
	node.sysctl_data = val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;
	for (i = 0; i <= PWR_SW_CTRL_MAX; i++)
		if (strcmp(val, pwr_sw_control_str[i]) == 0) {
			pwr_sw_ctrl(i);
			return 0;
		}
	return EINVAL;
}

