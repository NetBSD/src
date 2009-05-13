/*	$NetBSD: adb.c,v 1.24.32.1 2009/05/13 17:18:00 jym Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
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
 *	This product includes software developed by Bradley A. Grantham.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adb.c,v 1.24.32.1 2009/05/13 17:18:00 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/pio.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/akbdvar.h>
#include <macppc/dev/pm_direct.h>
#include <macppc/dev/viareg.h>

#include <dev/clock_subr.h>
#include <dev/ofw/openfirm.h>

#include "aed.h"
#include "apm.h"

/*
 * Function declarations.
 */
static int	adbmatch(struct device *, struct cfdata *, void *);
static void	adbattach(struct device *, struct device *, void *);
static int	adbprint(void *, const char *);
static void	adb_todr_init(void);

/*
 * Global variables.
 */
int     adb_polling = 0;	/* Are we polling?  (Debugger mode) */
int     adb_initted = 0;	/* adb_init() has completed successfully */
#ifdef ADB_DEBUG
int	adb_debug = 0;		/* Output debugging messages */
#endif /* ADB_DEBUG */

/*
 * Driver definition.
 */
CFATTACH_DECL(adb, sizeof(struct adb_softc),
    adbmatch, adbattach, NULL, NULL);

static int
adbmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_nreg < 8)
		return 0;

	if (ca->ca_nintr < 4)
		return 0;

	if (strcmp(ca->ca_name, "via-cuda") == 0)
		return 1;

	if (strcmp(ca->ca_name, "via-pmu") == 0)
		return 1;

	return 0;
}

static void
adbattach(struct device *parent, struct device *self, void *aux)
{
	struct adb_softc *sc = (struct adb_softc *)self;
	struct confargs *ca = aux;
	int irq = ca->ca_intr[0];
	int node;
	ADBDataBlock adbdata;
	struct adb_attach_args aa_args;
	int totaladbs;
	int adbindex, adbaddr, adb_node;

	extern volatile u_char *Via1Base;

	ca->ca_reg[0] += ca->ca_baseaddr;

	sc->sc_regbase = mapiodev(ca->ca_reg[0], ca->ca_reg[1]);
	Via1Base = sc->sc_regbase;

	if (strcmp(ca->ca_name, "via-cuda") == 0)
		adbHardware = ADB_HW_CUDA;
	else if (strcmp(ca->ca_name, "via-pmu") == 0)
		adbHardware = ADB_HW_PMU;

	node = of_getnode_byname(OF_parent(ca->ca_node), "extint-gpio1");
	if (node)
		OF_getprop(node, "interrupts", &irq, 4);

	printf(" irq %d: ", irq);

	adb_polling = 1;
	adb_node = of_getnode_byname(ca->ca_node, "adb");
	if (adb_node)
		ADBReInit();

	switch (adbHardware) {
	case ADB_HW_CUDA:
		intr_establish(irq, IST_LEVEL, IPL_TTY, adb_intr_cuda, sc);
		break;
	case ADB_HW_PMU:
		intr_establish(irq, IST_LEVEL, IPL_TTY, pm_intr, sc);
		pm_init();
		break;
	}

	adb_todr_init();

#if NAPM > 0
	/* Magic for signalling the apm driver to match. */
	aa_args.origaddr = ADBADDR_APM;
	aa_args.adbaddr = ADBADDR_APM;
	aa_args.handler_id = ADBADDR_APM;

	(void)config_found(self, &aa_args, NULL);
#endif

	/* 
	 * see if we're supposed to have an ADB bus
	 * since some PowerBooks don't have one and their PMUs barf on ADB
	 * commands we bail here if there's no adb node
	 */
	if (!adb_node)
		return;

#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: done with ADBReInit\n");
#endif
	totaladbs = CountADBs();

	printf("%d targets\n", totaladbs);

#if NAED > 0
	/* ADB event device for compatibility */
	aa_args.origaddr = 0;
	aa_args.adbaddr = 0;
	aa_args.handler_id = 0;
	(void)config_found(self, &aa_args, adbprint);
#endif

	/* for each ADB device */
	for (adbindex = 1; adbindex <= totaladbs; adbindex++) {
		/* Get the ADB information */
		adbaddr = GetIndADB(&adbdata, adbindex);

		aa_args.origaddr = adbdata.origADBAddr;
		aa_args.adbaddr = adbaddr;
		aa_args.handler_id = adbdata.devType;

		(void)config_found(self, &aa_args, adbprint);
	}

	if (adbHardware == ADB_HW_CUDA)
		adb_cuda_autopoll();
	adb_polling = 0;

}

int
adbprint(void *args, const char *name)
{
	struct adb_attach_args *aa_args = (struct adb_attach_args *)args;
	int rv = UNCONF;

	if (name) {	/* no configured device matched */
		rv = UNSUPP; /* most ADB device types are unsupported */

		/* print out what kind of ADB device we have found */
		aprint_normal("%s addr %d: ", name, aa_args->adbaddr);
		switch(aa_args->origaddr) {
#ifdef DIAGNOSTIC
		case 0:
			aprint_normal("ADB event device");
			rv = UNCONF;
			break;
		case ADBADDR_SECURE:
			aprint_normal("security dongle (%d)",
			    aa_args->handler_id);
			break;
#endif
		case ADBADDR_MAP:
			aprint_normal("mapped device (%d)",
			    aa_args->handler_id);
			rv = UNCONF;
			break;
		case ADBADDR_REL:
			aprint_normal("relative positioning device (%d)",
			    aa_args->handler_id);
			rv = UNCONF;
			break;
#ifdef DIAGNOSTIC
		case ADBADDR_ABS:
			switch (aa_args->handler_id) {
			case ADB_ARTPAD:
				aprint_normal("WACOM ArtPad II");
				break;
			default:
				aprint_normal("absolute positioning device (%d)",
				    aa_args->handler_id);
				break;
			}
			break;
		case ADBADDR_DATATX:
			aprint_normal("data transfer device (modem?) (%d)",
			    aa_args->handler_id);
			break;
		case ADBADDR_MISC:
			switch (aa_args->handler_id) {
			case ADB_POWERKEY:
				aprint_normal("Sophisticated Circuits PowerKey");
				break;
			default:
				aprint_normal("misc. device (remote control?) (%d)",
				    aa_args->handler_id);
				break;
			}
			break;
		default:
			aprint_normal("unknown type device, (handler %d)",
			    aa_args->handler_id);
			break;
#endif /* DIAGNOSTIC */
		}
	} else		/* a device matched and was configured */
                aprint_normal(" addr %d: ", aa_args->adbaddr);

	return rv;
}

#define DIFF19041970 2082844800

static int
adb_todr_get(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	unsigned long sec;

	if (adb_read_date_time(&sec) != 0)
		return EIO;
	tvp->tv_sec = sec - DIFF19041970;
	tvp->tv_usec = 0;
	return 0;
}

static int
adb_todr_set(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	unsigned long sec;

	sec = tvp->tv_sec + DIFF19041970;
	return adb_set_date_time(sec) ? EIO : 0;
}

void
adb_todr_init(void)
{
	static struct todr_chip_handle tch = {
		.todr_gettime = adb_todr_get,
		.todr_settime = adb_todr_set
	};

	todr_attach(&tch);
}
