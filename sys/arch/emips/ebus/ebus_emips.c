/*	$NetBSD: ebus_emips.c,v 1.1.8.2 2011/06/06 09:05:16 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: ebus_emips.c,v 1.1.8.2 2011/06/06 09:05:16 jruoho Exp $");

#include "opt_xilinx_ml40x.h"
#include "opt_xs_bee3.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <emips/ebus/ebusvar.h>

#include <machine/autoconf.h>
#include <machine/sysconf.h>

#include <machine/emipsreg.h>
#include <emips/emips/emipstype.h>

static int	ebus_emips_match __P((struct device *, struct cfdata *, void *));
static void	ebus_emips_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(ebus_emips, sizeof(struct ebus_softc),
    ebus_emips_match, ebus_emips_attach, NULL, NULL);

#if defined(XILINX_ML40x) || defined(XS_BEE3)
struct ebus_attach_args ebus_emips_devs[] = {
   /* NAME     INTERRUPT        PHYS                      VIRT  PHYS_SIZE */
   { "eclock", AIC_TIMER,    	TIMER_DEFAULT_ADDRESS,    NULL, sizeof(struct _Tc) },
   { "dz",     AIC_USART,	    USART_DEFAULT_ADDRESS,    NULL, sizeof(struct _Usart)},
   { "ace",    AIC_SYSTEM_ACE,  IDE_DEFAULT_ADDRESS,      NULL, sizeof(struct _Sac) },
   { "ace",    AIC_SYSTEM_ACE2, IDE_DEFAULT_ADDRESS+256,  NULL, sizeof(struct _Sac) },
   { "enic",   AIC_ETHERNET,	ETHERNET_DEFAULT_ADDRESS, NULL, sizeof(struct _Enic) },
   { "icap",   AIC_ICAP,		ICAP_DEFAULT_ADDRESS, 	  NULL, sizeof(struct _Icap) },
   { "gpio",   AIC_GPIO,		GPIO_DEFAULT_ADDRESS, 	  NULL, sizeof(struct _Pio) },
   { "flash",  0,				FLASH_0_DEFAULT_ADDRESS,  NULL, sizeof(struct _Flash) },
   { "lcd",    0,				LCD_DEFAULT_ADDRESS, 	  NULL, sizeof(struct _Lcd) },
   { "evga",   AIC_VGA,			VGA_DEFAULT_ADDRESS, 	  NULL, sizeof(struct _Evga) },
   { "ps2",    AIC_PS2,			PS2_DEFAULT_ADDRESS, 	  NULL, sizeof(struct _Cpbdi) },
   { "ac97",   AIC_AC97,		AC97_DEFAULT_ADDRESS, 	  NULL, sizeof(struct _Cpbdi) },
};
static const int ebus_emips_ndevs =
	sizeof(ebus_emips_devs)/sizeof(ebus_emips_devs[0]);
#endif /* XILINX_ML40x */

static int ebus_attached;

static int
ebus_emips_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (ebus_attached)
		return (0);
	if (systype != XS_ML40x && systype != XS_BE3 && systype != XS_ML50x)
		return (0);
	if (strcmp(ma->ma_name, "baseboard") != 0)
		return (0);

	return (1);
}

static void
ebus_emips_attach(struct device *parent, struct device *self, void *aux)
{
	struct ebus_dev_attach_args ida;

	ebus_attached = 1;

	ida.ida_busname = "ebus";
	switch (systype) {
#if defined(XILINX_ML40x) || defined(XS_BEE3)
	case XS_ML40x:
	case XS_ML50x:
	case XS_BE3:
		ida.ida_devs = ebus_emips_devs;
		ida.ida_ndevs = ebus_emips_ndevs;
		break;
#endif
	default:
		panic("ebus_emips_attach: no ebus configured for systype = %d", systype);
	}

	ebusattach(parent, self, &ida);
}
