/* $NetBSD: resetbtn.c,v 1.2.2.2 2024/02/03 11:47:04 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: resetbtn.c,v 1.2.2.2 2024/02/03 11:47:04 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <powerpc/pio.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include "hollywood.h"

#define	PI_INTERRUPT_CAUSE	0x0c003000
#define	 RESET_SWITCH_STATE	__BIT(16)

static int	resetbtn_match(device_t, cfdata_t, void *);
static void	resetbtn_attach(device_t, device_t, void *);

static int	resetbtn_intr(void *);
static void	resetbtn_task(void *);

CFATTACH_DECL_NEW(resetbtn, sizeof(struct sysmon_pswitch),
	resetbtn_match, resetbtn_attach, NULL, NULL);

static int
resetbtn_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
resetbtn_attach(device_t parent, device_t self, void *aux)
{
	struct hollywood_attach_args *haa = aux;
	struct sysmon_pswitch *smpsw = device_private(self);
	int error;

	KASSERT(device_unit(self) == 0);

	aprint_naive("\n");
	aprint_normal(": Reset button\n");

	sysmon_task_queue_init();

	smpsw->smpsw_name = device_xname(self);
	smpsw->smpsw_type = PSWITCH_TYPE_RESET;
	error = sysmon_pswitch_register(smpsw);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to register reset button with sysmon: %d\n", error);
		smpsw = NULL;
	}

	hollywood_intr_establish(haa->haa_irq, IPL_HIGH, resetbtn_intr, smpsw,
	    device_xname(self));
}

static int
resetbtn_intr(void *arg)
{
	struct sysmon_pswitch *smpsw = arg;

	sysmon_task_queue_sched(0, resetbtn_task, smpsw);

	return 1;
}

static void
resetbtn_task(void *arg)
{
	struct sysmon_pswitch *smpsw = arg;
	bool pressed;

	pressed = (in32(PI_INTERRUPT_CAUSE) & RESET_SWITCH_STATE) == 0;

	if (smpsw != NULL) {
		sysmon_pswitch_event(smpsw,
		    pressed ? PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
	} else if (!pressed) {
		kern_reboot(0, NULL);
	}
}
