/*	$NetBSD: adb.c,v 1.33 1999/11/28 10:10:51 scottr Exp $	*/

/*
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

#include "opt_adb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <mac68k/mac68k/macrom.h>
#include <mac68k/dev/adbvar.h>
#include <mac68k/dev/itevar.h>
#include <mac68k/dev/kbdvar.h>

#include "aed.h"		/* ADB Event Device for compatibility */

/*
 * Function declarations.
 */
static int	adbmatch __P((struct device *, struct cfdata *, void *));
static void	adbattach __P((struct device *, struct device *, void *));
static int	adbprint __P((void *, const char *));
void		adb_config_interrupts __P((struct device *));

extern void	adb_jadbproc __P((void));

/*
 * Global variables.
 */
int	adb_polling = 0;	/* Are we polling?  (Debugger mode) */
#ifdef ADB_DEBUG
int	adb_debug = 0;		/* Output debugging messages */
#endif /* ADB_DEBUG */

extern struct	mac68k_machine_S mac68k_machine;
extern int	adbHardware;
extern char	*adbHardwareDescr[];

/*
 * Driver definition.
 */
struct cfattach adb_ca = {
	sizeof(struct device), adbmatch, adbattach
};

static int
adbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	static int adb_matched = 0;

	/* Allow only one instance. */
	if (adb_matched)
		return (0);

	adb_matched = 1;
	return (1);
}

static void
adbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	printf("\n");

	/*
	 * Defer configuration until interrupts are enabled.
	 */
	config_interrupts(self, adb_config_interrupts);
}

void
adb_config_interrupts(self)
	struct device *self;
{
	ADBDataBlock adbdata;
	struct adb_attach_args aa_args;
	int totaladbs;
	int adbindex, adbaddr;

	printf("%s", self->dv_xname);
	adb_polling = 1;

#ifdef MRG_ADB
	/* 
	 * Even if serial console only, some models require the
         * ADB in order to get the date/time and do soft power.
	 */
	if ((mac68k_machine.serial_console & 0x03)) {
		printf(": using serial console\n");
		return;
	}

	if (!mrg_romready()) {
		printf(": no ROM ADB driver in this kernel for this machine\n");
		return;
	}

#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: call mrg_initadbintr\n");
#endif

	mrg_initadbintr();	/* Mac ROM Glue okay to do ROM intr */
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: returned from mrg_initadbintr\n");
#endif

	/* ADBReInit pre/post-processing */
	JADBProc = adb_jadbproc;

	/* Initialize ADB */
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: calling ADBAlternateInit.\n");
#endif

	printf(" (mrg)");
	ADBAlternateInit();
#else
	ADBReInit();
	printf(" (direct, %s)", adbHardwareDescr[adbHardware]);
#endif /* MRG_ADB */

#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: done with ADBReInit\n");
#endif

	totaladbs = CountADBs();

	printf(": %d target%s\n", totaladbs, (totaladbs == 1) ? "" : "s");

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

		aa_args.origaddr = (int)(adbdata.origADBAddr);
		aa_args.adbaddr = adbaddr;
		aa_args.handler_id = (int)(adbdata.devType);

		(void)config_found(self, &aa_args, adbprint);
	}
	adb_polling = 0;
}


int
adbprint(args, name)
        void *args;
        const char *name;
{
	struct adb_attach_args *aa_args = (struct adb_attach_args *)args;
	int rv = UNCONF;

	if (name) {	/* no configured device matched */
		rv = UNSUPP; /* most ADB device types are unsupported */

		/* print out what kind of ADB device we have found */
		printf("%s addr %d: ", name, aa_args->origaddr);
		switch(aa_args->origaddr) {
#ifdef DIAGNOSTIC
		case 0:
			printf("ADB event device");
			rv = UNCONF;
			break;
		case ADBADDR_SECURE:
			printf("security dongle (%d)", aa_args->handler_id);
			break;
#endif
		case ADBADDR_MAP:
			printf("mapped device (%d)", aa_args->handler_id);
			rv = UNCONF;
			break;
		case ADBADDR_REL:
			printf("relative positioning device (%d)",
			    aa_args->handler_id);
			rv = UNCONF;
			break;
#ifdef DIAGNOSTIC
		case ADBADDR_ABS:
			switch (aa_args->handler_id) {
			case ADB_ARTPAD:
				printf("WACOM ArtPad II");
				break;
			default:
				printf("absolute positioning device (%d)",
				    aa_args->handler_id);
				break;
			}
			break;
		case ADBADDR_DATATX:
			printf("data transfer device (modem?) (%d)",
			    aa_args->handler_id);
			break;
		case ADBADDR_MISC:
			switch (aa_args->handler_id) {
			case ADB_POWERKEY:
				printf("Sophisticated Circuits PowerKey");
				break;
			default:
				printf("misc. device (remote control?) (%d)",
				    aa_args->handler_id);
				break;
			}
			break;
		default:
			printf("unknown type device, (handler %d)",
			    aa_args->handler_id);
			break;
#endif /* DIAGNOSTIC */
		}
	} else		/* a device matched and was configured */
                printf(" addr %d: ", aa_args->origaddr);

	return (rv);
}
