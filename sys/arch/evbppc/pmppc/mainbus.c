/*	$NetBSD: mainbus.c,v 1.3.32.1 2011/06/23 14:19:10 cherry Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.3.32.1 2011/06/23 14:19:10 cherry Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/pmppc.h>
#include <arch/evbppc/pmppc/dev/mainbus.h>

#include <dev/ic/cpc700reg.h>

#include "locators.h"
#include "mainbus.h"

#if NCPU == 0
#error	A cpu device is now required
#endif

int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int mainbus_print(void *, const char *);

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static int
mainbus_submatch(device_t parent, cfdata_t cf,
		 const int *ldesc, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (cf->cf_loc[MAINBUSCF_ADDR] != maa->mb_addr)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mb_name, pnp);
	if (mba->mb_addr != MAINBUSCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%08lx", mba->mb_addr);
	if (mba->mb_irq != MAINBUSCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", mba->mb_irq);
	return (UNCONF);
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args maa;

	aprint_normal(": Artesyn PM/PPC\n");
	aprint_normal_dev(self, "%sPCI bus Monarch\n",
	       a_config.a_is_monarch ? "" : "not");
	aprint_normal_dev(self, "boot from %s, %sECC, %s L2 cache\n",
	       a_config.a_boot_device == A_BOOT_ROM ? "ROM" : "flash",
	       a_config.a_has_ecc ? "" : "no ",
	       a_config.a_l2_cache == A_CACHE_PARITY ? "parity" :
	       a_config.a_l2_cache == A_CACHE_NO_PARITY ? "no-parity" : "no");

	maa.mb_bt = &pmppc_mem_tag;

	maa.mb_name = "cpu";
	maa.mb_addr = MAINBUSCF_ADDR_DEFAULT;
	maa.mb_irq = MAINBUSCF_IRQ_DEFAULT;
	config_found(self, &maa, mainbus_print);

	if (a_config.a_has_rtc) {
		maa.mb_name = "rtc";
		maa.mb_addr = PMPPC_RTC;
		maa.mb_irq = PMPPC_I_RTC_INT;
		config_found_sm_loc(self, "mainbus", NULL, &maa, mainbus_print,
				    mainbus_submatch);
	}

	if (a_config.a_has_eth) {
		maa.mb_name = "cs";
		maa.mb_addr = PMPPC_CS_IO_BASE;
		maa.mb_irq = PMPPC_I_ETH_INT;
		config_found_sm_loc(self, "mainbus", NULL, &maa, mainbus_print,
				    mainbus_submatch);
		maa.mb_bt = &pmppc_mem_tag;
	}
	if (a_config.a_flash_width != 0) {
		maa.mb_name = "flash";
		maa.mb_addr = PMPPC_FLASH_BASE;
		maa.mb_irq = MAINBUSCF_IRQ_DEFAULT;
		maa.u.mb_flash.size = a_config.a_flash_size;
		maa.u.mb_flash.width = a_config.a_flash_width;
		config_found_sm_loc(self, "mainbus", NULL, &maa, mainbus_print,
				    mainbus_submatch);
	}

	maa.mb_name = "cpc";
	maa.mb_addr = MAINBUSCF_ADDR_DEFAULT;
	maa.mb_irq = MAINBUSCF_IRQ_DEFAULT;
	config_found(self, &maa, mainbus_print);
}

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (strcmp(maa->mb_name, cpu_cd.cd_name) != 0)
		return 0;

	if (cpu_info[0].ci_dev != NULL)
		return 0;

	return 1;
}

void
cpu_attach(device_t parent, device_t self, void *aux)
{
	(void) cpu_attach_common(self, 0);
}
