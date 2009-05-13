/*	$NetBSD: power.c,v 1.4.2.2 2009/05/13 17:17:43 jym Exp $	*/

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

/*	$OpenBSD: power.c,v 1.5 2004/06/11 12:53:09 mickey Exp $	*/

/*
 * Copyright (c) 2003 Michael Shalayeff
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>

#include <machine/reg.h>
#include <machine/pdc.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/hp700/intr.h>

#include <dev/sysmon/sysmon_taskq.h>
#include <dev/sysmon/sysmonvar.h>

/* Enable / disable control over the power switch. */
#define	PWR_SW_CTRL_DISABLE	0
#define	PWR_SW_CTRL_ENABLE	1
#define	PWR_SW_CTRL_LOCK	2
#define	PWR_SW_CTRL_MAX		PWR_SW_CTRL_LOCK

struct power_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	void (*sc_kicker)(void *);

	struct callout sc_callout;
	int sc_timeout;

	int sc_dr_cnt;
};

int	powermatch(device_t, cfdata_t, void *);
void	powerattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(power, sizeof(struct power_softc),
    powermatch, powerattach, NULL, NULL);

static struct pdc_power_info pdc_power_info PDC_ALIGNMENT;
static bool pswitch_on;			/* power switch */
static int pwr_sw_control;
static const char *pwr_sw_control_str[] = {"disabled", "enabled", "locked"};
static struct sysmon_pswitch *pwr_sw_sysmon;

static int pwr_sw_sysctl_state(SYSCTLFN_PROTO);
static int pwr_sw_sysctl_ctrl(SYSCTLFN_PROTO);
static void pwr_sw_sysmon_cb(void *);
static void pwr_sw_ctrl(int);
static int pwr_sw_init(struct power_softc *);

void power_thread_dr(void *v);
void power_thread_reg(void *v);
void power_cold_hook_reg(int);

int
powermatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (cf->cf_unit > 0 && !strcmp(ca->ca_name, "power"))
		return (0);

	return (1);
}

void
powerattach(device_t parent, device_t self, void *aux)
{
	struct power_softc *sc = device_private(self);
	struct confargs *ca = aux;

	sc->sc_dev = self;
	sc->sc_kicker = NULL;

	if (!pdc_call((iodcio_t)pdc, 0, PDC_SOFT_POWER,
	    PDC_SOFT_POWER_INFO, &pdc_power_info, 0)) {
		ca->ca_hpa = pdc_power_info.addr;
	}

	switch (cpu_hvers) {
	case HPPA_BOARD_HP712_60:
	case HPPA_BOARD_HP712_80:
	case HPPA_BOARD_HP712_100:
	case HPPA_BOARD_HP712_120:
		sc->sc_kicker = power_thread_dr;

		/* Diag Reg. needs software dampening, poll at 0.2 Hz.*/
		sc->sc_timeout = hz / 5;

		aprint_normal(": DR25\n");
		break;

	default:
		if (ca->ca_hpa) {
			sc->sc_bst = ca->ca_iot;
			if (bus_space_map(sc->sc_bst, ca->ca_hpa, 4, 0,
			    &sc->sc_bsh) != 0)
				aprint_error_dev(self,
				    "Can't map power switch status reg.\n");

			cold_hook = power_cold_hook_reg;
			sc->sc_kicker = power_thread_reg;

			/* Power Reg. is hardware dampened, poll at 1 Hz. */
			sc->sc_timeout = hz;

			aprint_normal("\n");
		} else
			aprint_normal(": not available\n");
		break;
	}

	if (sc->sc_kicker) {
		if (pwr_sw_init(sc))
			return;

		pswitch_on = true;
		pwr_sw_control = PWR_SW_CTRL_ENABLE;
	}
}

/*
 * If the power switch is turned off we schedule a sysmon task
 * to register that event for this power switch device.
 */
static void
check_pwr_state(struct power_softc *sc)
{
	if (pswitch_on == false && pwr_sw_control != PWR_SW_CTRL_LOCK)
		sysmon_task_queue_sched(0, pwr_sw_sysmon_cb, NULL);
	else
		callout_reset(&sc->sc_callout, sc->sc_timeout,
		    sc->sc_kicker, sc);
}

void
power_thread_dr(void *v)
{
	struct power_softc *sc = v;
	uint32_t r;

	/* Get Power Fail status from CPU Diagnose Register 25 */
	mfcpu(25, r);

	/* 
	 * On power failure, the hardware clears bit DR25_PCXL_POWFAIL
	 * in CPU Diagnose Register 25.
	 */
	if (r & (1 << DR25_PCXL_POWFAIL))
		sc->sc_dr_cnt = 0;
	else
		sc->sc_dr_cnt++;

	/*
	 * the bit is undampened straight wire from the power
	 * switch and thus we have do dampen it ourselves.
	 */
	if (sc->sc_dr_cnt == sc->sc_timeout)
		pswitch_on = false;
	else
		pswitch_on = true;

	check_pwr_state(sc);
}

void
power_thread_reg(void *v)
{
	struct power_softc *sc = v;
	uint32_t r;

	r = bus_space_read_4(sc->sc_bst, sc->sc_bsh, 0);

	if (!(r & 1))
		pswitch_on = false;
	else
		pswitch_on = true;

	check_pwr_state(sc);
}

void
power_cold_hook_reg(int on)
{
	int error;

	if ((error = pdc_call((iodcio_t)pdc, 0, PDC_SOFT_POWER,
	    PDC_SOFT_POWER_ENABLE, &pdc_power_info,
	    on == HPPA_COLD_HOT)))
		aprint_error("PDC_SOFT_POWER_ENABLE failed (%d)\n", error);
}

static int
pwr_sw_init(struct power_softc *sc)
{
	struct sysctllog *sysctl_log = NULL;
	const struct sysctlnode *pwr_sw_node;
	const char *errmsg;
	int error = EINVAL;

	/*
	 * Ensure that we are on a PCX-L / PA7100LC CPU if it is a 
	 * 712 style machine.
	 */
	if (pdc_power_info.addr == 0 && hppa_cpu_info->hci_type != hpcxl) {
		aprint_error_dev(sc->sc_dev, "No soft power available.\n");
		return error;
	}

	errmsg = "Can't create sysctl machdep.power_switch (or children)\n";
	error = sysctl_createv(&sysctl_log, 0, NULL, NULL, 0, 
	    CTLTYPE_NODE, "machdep", NULL, NULL, 0, NULL, 0, 
	    CTL_MACHDEP, CTL_EOL);

	if (error)
		goto err_sysctl;
	
	error = sysctl_createv(&sysctl_log, 0, NULL, &pwr_sw_node, 0, 
	    CTLTYPE_NODE, "power_switch", NULL, NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	if (error)
		goto err_sysctl;

	error = sysctl_createv(&sysctl_log, 0, NULL, NULL, 
	    CTLFLAG_READONLY, CTLTYPE_STRING, "state", NULL, 
	    pwr_sw_sysctl_state, 0, NULL, 16, 
	    CTL_MACHDEP, pwr_sw_node->sysctl_num, CTL_CREATE, CTL_EOL);

	if (error)
		goto err_sysctl;

	error = sysctl_createv(&sysctl_log, 0, NULL, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_STRING, "control", NULL,
	    pwr_sw_sysctl_ctrl, 0, NULL, 16,
	    CTL_MACHDEP, pwr_sw_node->sysctl_num, CTL_CREATE, CTL_EOL);

	if (error)
		goto err_sysctl;

	errmsg = "Can't alloc sysmon power switch.\n";
	pwr_sw_sysmon = kmem_zalloc(sizeof(*pwr_sw_sysmon), KM_SLEEP);
	if (pwr_sw_sysmon == NULL) {
		error = ENOMEM;
		goto err_kmem;
	}

	errmsg = "Can't register power switch with sysmon.\n";
	sysmon_task_queue_init();
	pwr_sw_sysmon->smpsw_name = "power switch";
	pwr_sw_sysmon->smpsw_type = PSWITCH_TYPE_POWER;
	error = sysmon_pswitch_register(pwr_sw_sysmon);

	if (error)
		goto err_sysmon;

	callout_init(&sc->sc_callout, 0);
	callout_reset(&sc->sc_callout, sc->sc_timeout, sc->sc_kicker, sc);

	return error;

err_sysmon:
	/* Nothing to do */

err_kmem:
	kmem_free(pwr_sw_sysmon, sizeof(*pwr_sw_sysmon));

err_sysctl:
	sysctl_teardown(&sysctl_log);

	aprint_error_dev(sc->sc_dev, errmsg);

	return error;
}

static void
pwr_sw_sysmon_cb(void *not_used) 
{
	sysmon_pswitch_event(pwr_sw_sysmon, PSWITCH_EVENT_PRESSED);
}

static void
pwr_sw_ctrl(int enable)
{
	int on;

#ifdef DEBUG
	printf("pwr_sw_control=%d enable=%d\n", pwr_sw_control, enable);
#endif /* DEBUG */

	if (cold_hook == NULL)
		return;

	switch(enable) {
	case PWR_SW_CTRL_DISABLE:
		on = HPPA_COLD_OFF;
		break;
	case PWR_SW_CTRL_ENABLE:
	case PWR_SW_CTRL_LOCK:
		on = HPPA_COLD_HOT;
		break;
	default:
		panic("invalid power state in pwr_sw_control: %d", enable);
	}

	pwr_sw_control = enable;

	if (cold_hook)
		(*cold_hook)(on);
}

int
pwr_sw_sysctl_state(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	const char *status;

	if (pswitch_on == true)
		status = "on";
	else
		status = "off";

	node = *rnode;
	node.sysctl_data = __UNCONST(status);
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

	if (error || newp == NULL)
		return error;

	for (i = 0; i <= PWR_SW_CTRL_MAX; i++)
		if (strcmp(val, pwr_sw_control_str[i]) == 0) {
			pwr_sw_ctrl(i);
			return 0;
		}

	return EINVAL;
}
