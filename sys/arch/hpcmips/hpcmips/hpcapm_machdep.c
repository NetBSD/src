/*	$NetBSD: hpcapm_machdep.c,v 1.2.92.1 2009/05/13 17:17:46 jym Exp $	*/

/*
 * Copyright (c) 2000 Takemura Shin
 * Copyright (c) 2000-2001 SATO Kazumi
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpcapm_machdep.c,v 1.2.92.1 2009/05/13 17:17:46 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/hpc/apm/apmvar.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include "vrip_common.h"
#if NVRIP_COMMON > 0
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vr_asm.h>
#endif

#include "opt_tx39xx.h"
#ifdef TX39XX
#include <hpcmips/tx/tx39var.h>
#endif

void
machine_standby(void)
{
#if NVRIP_COMMON > 0
	if (platid_match(&platid, &platid_mask_CPU_MIPS_VR_41XX)) {
		/*
		 * disable all interrupts except PIU interrupt
		 */
		vrip_intr_suspend();
		_spllower(~MIPS_INT_MASK_0);

		/*
		 * STANDBY instruction puts the CPU into power saveing
		 * state until some interrupt occuer.
		 * It sleeps until you push the power button.
		 */
		__asm(".set noreorder");
		__asm(".word	" ___STRING(VR_OPCODE_STANDBY));
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm(".set reorder");

		splhigh();
		vrip_intr_resume();
		delay(1000); /* 1msec */
	}
#endif /* NVRIP_COMMON > 0 */
}

void
machine_sleep(void)
{
#if NVRIP_COMMON > 0
	 if (platid_match(&platid, &platid_mask_CPU_MIPS_VR_41XX)) {
		 /*
		  * disable all interrupts except PIU interrupt
		  */
		 vrip_intr_suspend();
		 _spllower(~MIPS_INT_MASK_0);

		 /*
		  * SUSPEND instruction puts the CPU into power saveing
		  * state until some interrupt occuer.
		  * It sleeps until you push the power button.
		  */
		 __asm(".set noreorder");
		 __asm(".word	" ___STRING(VR_OPCODE_SUSPEND));
		 __asm("nop");
		 __asm("nop");
		 __asm("nop");
		 __asm("nop");
		 __asm("nop");
		 __asm(".set reorder");

		 splhigh();
		 vrip_intr_resume();
		 delay(1000); /* 1msec */
	 }
#endif /* NVRIP_COMMON > 0 */
#ifdef TX39XX
	 if (platid_match(&platid, &platid_mask_CPU_MIPS_TX)) {
		 tx39power_suspend_cpu();
	 }
#endif
}
