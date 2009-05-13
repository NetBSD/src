/*	$NetBSD: hd64461.c,v 1.19.14.1 2009/05/13 17:17:47 jym Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: hd64461.c,v 1.19.14.1 2009/05/13 17:17:47 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/debug.h>

#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461intcreg.h>
#include <hpcsh/dev/hd64461/hd64461var.h>

/* HD64461 modules. INTC, TIMER, POWER modules are included in hd64461if */
STATIC const struct hd64461_module {
	const char *name;
} hd64461_modules[] = {
	[HD64461_MODULE_VIDEO]		= { "hd64461video" },
	[HD64461_MODULE_PCMCIA]		= { "hd64461pcmcia" },
	[HD64461_MODULE_UART]		= { "hd64461uart" },
#if 0 /* there are no drivers, so don't bother */
	[HD64461_MODULE_GPIO]		= { "hd64461gpio" },
	[HD64461_MODULE_AFE]		= { "hd64461afe" },
	[HD64461_MODULE_FIR]		= { "hd64461fir" },
#endif
};

STATIC int hd64461_match(device_t, cfdata_t, void *);
STATIC void hd64461_attach(device_t, device_t, void *);
STATIC int hd64461_print(void *, const char *);
#ifdef DEBUG
STATIC void hd64461_info(void);
#endif

CFATTACH_DECL_NEW(hd64461if, 0,
    hd64461_match, hd64461_attach, NULL, NULL);

STATIC int
hd64461_match(device_t parent, cfdata_t cf, void *aux)
{

	switch (cpu_product) {
	case CPU_PRODUCT_7709:
	case CPU_PRODUCT_7709A:
		break;

	default:
		return (0);	/* HD64461 only supports SH7709 interface */
	}

	if (strcmp("hd64461if", cf->cf_name) != 0)
		return (0);

	return (1);
}

STATIC void
hd64461_attach(device_t parent, device_t self, void *aux)
{
	struct hd64461_attach_args ha;
	const struct hd64461_module *module;
	uint16_t stbcr;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");
#ifdef DEBUG
	if (bootverbose)
		hd64461_info();
#endif

	stbcr = hd64461_reg_read_2(HD64461_SYSSTBCR_REG16);

	/* we don't use TIMER */
	stbcr |= HD64461_SYSSTBCR_STM0ST | HD64461_SYSSTBCR_STM1ST;

	/* no drivers for FIR and AFE */
	stbcr |= HD64461_SYSSTBCR_SIRST
		| HD64461_SYSSTBCR_SAFEST
		| HD64461_SYSSTBCR_SAFECKE_IST
		| HD64461_SYSSTBCR_SAFECKE_OST;

	hd64461_reg_write_2(HD64461_SYSSTBCR_REG16, stbcr);


	/* Attach all sub modules */
	for (i = 0; i < __arraycount(hd64461_modules); ++i) {
		module = &hd64461_modules[i];
		if (module->name == NULL)
			continue;
		ha.ha_module_id = i;
		config_found(self, &ha, hd64461_print);
	}

	/* XXX: TODO */
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}

STATIC int
hd64461_print(void *aux, const char *pnp)
{
	struct hd64461_attach_args *ha = aux;

	if (pnp)
		aprint_normal("%s at %s",
		    hd64461_modules[ha->ha_module_id].name, pnp);

	return (UNCONF);
}


#ifdef DEBUG

STATIC void
hd64461_info(void)
{
	uint16_t r16;

	dbg_banner_function();

	/*
	 * System
	 */
	printf("STBCR (Standby Control Register)\n");
	r16 = hd64461_reg_read_2(HD64461_SYSSTBCR_REG16);
#define DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_SYSSTBCR_##m, #m)
	DBG_BITMASK_PRINT(r16, CKIO_STBY);
	DBG_BITMASK_PRINT(r16, SAFECKE_IST);
	DBG_BITMASK_PRINT(r16, SLCKE_IST);
	DBG_BITMASK_PRINT(r16, SAFECKE_OST);
	DBG_BITMASK_PRINT(r16, SLCKE_OST);
	DBG_BITMASK_PRINT(r16, SMIAST);
	DBG_BITMASK_PRINT(r16, SLCDST);
	DBG_BITMASK_PRINT(r16, SPC0ST);
	DBG_BITMASK_PRINT(r16, SPC1ST);
	DBG_BITMASK_PRINT(r16, SAFEST);
	DBG_BITMASK_PRINT(r16, STM0ST);
	DBG_BITMASK_PRINT(r16, STM1ST);
	DBG_BITMASK_PRINT(r16, SIRST);
	DBG_BITMASK_PRINT(r16, SURTSD);
#undef DBG_BITMASK_PRINT
	printf("\n");

	printf("SYSCR (System Configuration Register)\n");
	r16 = hd64461_reg_read_2(HD64461_SYSSYSCR_REG16);
#define DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_SYSSYSCR_##m, #m)
	DBG_BITMASK_PRINT(r16, SCPU_BUS_IGAT);
	DBG_BITMASK_PRINT(r16, SPTA_IR);
	DBG_BITMASK_PRINT(r16, SPTA_TM);
	DBG_BITMASK_PRINT(r16, SPTB_UR);
	DBG_BITMASK_PRINT(r16, WAIT_CTL_SEL);
	DBG_BITMASK_PRINT(r16, SMODE1);
	DBG_BITMASK_PRINT(r16, SMODE0);
#undef DBG_BITMASK_PRINT
	printf("\n");

	printf("SCPUCR (CPU Data Bus Control Register)\n");
	r16 = hd64461_reg_read_2(HD64461_SYSSCPUCR_REG16);
#define DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_SYSSCPUCR_##m, #m)
	DBG_BITMASK_PRINT(r16, SPDSTOF);
	DBG_BITMASK_PRINT(r16, SPDSTIG);
	DBG_BITMASK_PRINT(r16, SPCSTOF);
	DBG_BITMASK_PRINT(r16, SPCSTIG);
	DBG_BITMASK_PRINT(r16, SPBSTOF);
	DBG_BITMASK_PRINT(r16, SPBSTIG);
	DBG_BITMASK_PRINT(r16, SPASTOF);
	DBG_BITMASK_PRINT(r16, SPASTIG);
	DBG_BITMASK_PRINT(r16, SLCDSTIG);
	DBG_BITMASK_PRINT(r16, SCPU_CS56_EP);
	DBG_BITMASK_PRINT(r16, SCPU_CMD_EP);
	DBG_BITMASK_PRINT(r16, SCPU_ADDR_EP);
	DBG_BITMASK_PRINT(r16, SCPDPU);
	DBG_BITMASK_PRINT(r16, SCPU_A2319_EP);
#undef DBG_BITMASK_PRINT
	printf("\n");

	printf("\n");
	/*
	 * INTC
	 */
	printf("NIRR (Interrupt Request Register)\n");
	r16 = hd64461_reg_read_2(HD64461_INTCNIRR_REG16);
#define DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_INTCNIRR_##m, #m)
	DBG_BITMASK_PRINT(r16, PCC0R);
	DBG_BITMASK_PRINT(r16, PCC1R);
	DBG_BITMASK_PRINT(r16, AFER);
	DBG_BITMASK_PRINT(r16, GPIOR);
	DBG_BITMASK_PRINT(r16, TMU0R);
	DBG_BITMASK_PRINT(r16, TMU1R);
	DBG_BITMASK_PRINT(r16, IRDAR);
	DBG_BITMASK_PRINT(r16, UARTR);
#undef DBG_BITMASK_PRINT
	printf("\n");

	printf("NIMR (Interrupt Mask Register)\n");
	r16 = hd64461_reg_read_2(HD64461_INTCNIMR_REG16);
#define DBG_BITMASK_PRINT(r, m)	dbg_bitmask_print(r, HD64461_INTCNIMR_##m, #m)
	DBG_BITMASK_PRINT(r16, PCC0M);
	DBG_BITMASK_PRINT(r16, PCC1M);
	DBG_BITMASK_PRINT(r16, AFEM);
	DBG_BITMASK_PRINT(r16, GPIOM);
	DBG_BITMASK_PRINT(r16, TMU0M);
	DBG_BITMASK_PRINT(r16, TMU1M);
	DBG_BITMASK_PRINT(r16, IRDAM);
	DBG_BITMASK_PRINT(r16, UARTM);
#undef DBG_BITMASK_PRINT
	printf("\n");

	dbg_banner_line();
}
#endif /* DEBUG */
