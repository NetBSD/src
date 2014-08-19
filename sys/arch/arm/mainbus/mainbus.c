/* $NetBSD: mainbus.c,v 1.20.2.1 2014/08/20 00:02:47 tls Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * mainbus.c
 *
 * mainbus configuration
 *
 * Created      : 15/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.20.2.1 2014/08/20 00:02:47 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#if defined(arm32)		/* XXX */
#include <machine/io.h>
#endif
#include <sys/bus.h>
#include <arm/mainbus/mainbus.h>
#include "locators.h"

/*
 * mainbus is a root device so we a bus space tag to pass to children
 *
 * The tag is provided by mainbus_io.c and mainbus_io_asm.S
 */

extern struct bus_space mainbus_bs_tag;

/* Prototypes for functions provided */

static int  mainbusmatch(device_t, cfdata_t, void *);
static void mainbusattach(device_t, device_t, void *);
static int  mainbusprint(void *aux, const char *mainbus);
static int  mainbussearch(device_t, cfdata_t,
				const int *, void *);

/* attach and device structures for the device */

CFATTACH_DECL_NEW(mainbus, 0,
    mainbusmatch, mainbusattach, NULL, NULL);

/*
 * int mainbusmatch(device_t parent, cfdata_t cf, void *aux)
 *
 * Always match for unit 0
 */

static int
mainbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

/*
 * int mainbusprint(void *aux, const char *mainbus)
 *
 * print routine used during config of children
 */

static int
mainbusprint(void *aux, const char *mainbus)
{
	struct mainbus_attach_args *mb = aux;

	if (mb->mb_iobase != MAINBUSCF_BASE_DEFAULT)
		aprint_normal(" base 0x%x", mb->mb_iobase);
	if (mb->mb_iosize > 1)
		aprint_normal("-0x%x", mb->mb_iobase + mb->mb_iosize - 1);
	if (mb->mb_irq != -1)
		aprint_normal(" irq %d", mb->mb_irq);
	if (mb->mb_drq != -1)
		aprint_normal(" drq 0x%08x", mb->mb_drq);
	if (mb->mb_core != MAINBUSCF_CORE_DEFAULT)
		aprint_normal(" core %d", mb->mb_core);

/* XXXX print flags */
	return (QUIET);
}

/*
 * int mainbussearch(device_t parent, device_t self, void *aux)
 *
 * search routine used during the config of children
 */

static int
mainbussearch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args mb;
	int tryagain;

	do {
		if (cf->cf_loc[MAINBUSCF_BASE] == MAINBUSCF_BASE_DEFAULT) {
			mb.mb_iobase = MAINBUSCF_BASE_DEFAULT;
			mb.mb_iosize = 0;
			mb.mb_drq = MAINBUSCF_DACK_DEFAULT;
			mb.mb_irq = MAINBUSCF_IRQ_DEFAULT;
		} else {    
			mb.mb_iobase = cf->cf_loc[MAINBUSCF_BASE];
#if defined(arm32) && !defined(EB7500ATX)
			mb.mb_iobase += IO_CONF_BASE;
#endif
			mb.mb_iosize = cf->cf_loc[MAINBUSCF_SIZE];
			mb.mb_drq = cf->cf_loc[MAINBUSCF_DACK];
			mb.mb_irq = cf->cf_loc[MAINBUSCF_IRQ];
		}
		mb.mb_core = cf->cf_loc[MAINBUSCF_CORE];
		mb.mb_intrbase = cf->cf_loc[MAINBUSCF_INTRBASE];
		mb.mb_iot = &mainbus_bs_tag;

		tryagain = 0;
		if (config_match(parent, cf, &mb) > 0) {
			config_attach(parent, cf, &mb, mainbusprint);
#ifdef MULTIPROCESSOR
			tryagain = (cf->cf_fstate == FSTATE_STAR);
#endif
		}
	} while (tryagain);

	return (0);
}

/*
 * void mainbusattach(device_t parent, device_t self, void *aux)
 *
 * probe and attach all children
 */

static void
mainbusattach(device_t parent, device_t self, void *aux)
{
	aprint_naive("\n");
	aprint_normal("\n");

	config_search_ia(mainbussearch, self, "mainbus", NULL);
}

/* End of mainbus.c */
