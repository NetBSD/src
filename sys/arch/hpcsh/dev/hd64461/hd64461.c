/*	$NetBSD: hd64461.c,v 1.7 2002/03/28 15:26:59 uch Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/boot_flag.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/debug.h>

#include <hpcsh/dev/hd64461/hd64461var.h>
#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461intcreg.h>

/* HD64461 modules. INTC, TIMER, POWER modules are included in hd64461if */
STATIC struct hd64461_module {
	const char *name;
} hd64461_modules[] = {
	[HD64461_MODULE_VIDEO]		= { "hd64461video" },
	[HD64461_MODULE_PCMCIA]		= { "hd64461pcmcia" },
	[HD64461_MODULE_GPIO]		= { "hd64461gpio" },
	[HD64461_MODULE_AFE]		= { "hd64461afe" },
	[HD64461_MODULE_UART]		= { "hd64461uart" },
	[HD64461_MODULE_FIR]		= { "hd64461fir" },
};
#define HD64461_NMODULE							\
	(sizeof hd64461_modules / sizeof(struct hd64461_module))

STATIC int hd64461_match(struct device *, struct cfdata *, void *);
STATIC void hd64461_attach(struct device *, struct device *, void *);
STATIC int hd64461_print(void *, const char *);
#ifdef DEBUG
STATIC void hd64461_info(void);
#endif

struct cfattach hd64461if_ca = {
	sizeof(struct device), hd64461_match, hd64461_attach
};

int
hd64461_match(struct device *parent, struct cfdata *cf, void *aux)
{

	switch (cpu_product) {
	default:
		/* HD64461 only supports SH7709 interface */
		return (0);
	case CPU_PRODUCT_7709:
		break;
	case CPU_PRODUCT_7709A:
		break;
	}

	if (strcmp("hd64461if", cf->cf_driver->cd_name))
		return (0);

	return (1);
}

void
hd64461_attach(struct device *parent, struct device *self, void *aux)
{
	struct hd64461_attach_args ha;
	struct hd64461_module *module;
	int i;

	printf("\n");
#ifdef DEBUG
	if (bootverbose)
		hd64461_info();
#endif

	/* Attach all sub modules */
	for (i = 0, module = hd64461_modules; i < HD64461_NMODULE;
	    i++, module++) {
		if (module->name == 0)
			continue;
		ha.ha_module_id = i;
		config_found(self, &ha, hd64461_print);
	}
}

int
hd64461_print(void *aux, const char *pnp)
{
	struct hd64461_attach_args *ha = aux;

	if (pnp)
		printf("%s at %s",
		    hd64461_modules[ha->ha_module_id].name, pnp);

	return (UNCONF);
}

#ifdef DEBUG
void
hd64461_info()
{
	u_int16_t r16;

	dbg_banner_function();

	/*
	 * System
	 */
	printf("STBCR (System Control Register)\n");
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
