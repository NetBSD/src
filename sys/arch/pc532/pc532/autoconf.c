/*	$NetBSD: autoconf.c,v 1.25.2.1 1997/03/12 21:16:29 is Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba 
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <machine/autoconf.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */

extern int cold;		/* cold start flag initialized in locore.s */
u_long bootdev = 0;		/* should be dev_t, but not until 32 bits */
struct device *booted_device;	/* boot device, set by dk_establish */

struct devnametobdevmaj pc532_nam2blk[] = {
	{ "sd",		0 },
	{ "st",		2 },
	{ "md",		3 },
	{ "cd",		4 },
	{ NULL,		0 },
};

/*
 * Determine i/o configuration for a machine.
 */
void
configure()
{
	extern int safepri;
	int i;
	int booted_partition = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	static const char *ipl_names[] = IPL_NAMES;

	/* Start the clocks. */
	startrtclock();

	/* Find out what the hardware configuration looks like! */
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("No mainbus found!");

	for (i = 0; i < NIPL; i++)
		if (*ipl_names[i])
			printf("%s%s=%x", i?", ":"", ipl_names[i], imask[i]);
	printf("\n");

	safepri = imask[IPL_ZERO];
	spl0();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, pc532_nam2blk);

	/*
	 * Configure swap area and related system
	 * parameter based on device(s) used.
	 */
	swapconf();
	dumpconf();
	cold = 0;
}

/* mem bus stuff */

static int mbprobe();
static void mbattach();

struct cfattach mainbus_ca = {
	sizeof(struct device), mbprobe, mbattach
};

struct cfdriver mainbus_cd = {
	NULL, "mainbus", DV_DULL
};

static int
mbprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (strcmp(cf->cf_driver->cd_name, "mainbus") == 0);
}

static int
mbprint(aux, pnp)
	void *aux;
	char *pnp;
{
	struct confargs *ca = aux;

	printf(" addr 0x%x", ca->ca_addr);
	if (ca->ca_irq != -1) {
		printf(", irq %d", ca->ca_irq & 15);
		if (ca->ca_irq & 0xf0)
			printf(", %d", ca->ca_irq >> 4);
	}

	return(UNCONF);
}

static int
mbsearch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct cfdata *cf = match;
	struct confargs ca;

	ca.ca_addr  = cf->cf_loc[0];
	ca.ca_irq   = cf->cf_loc[1];
	ca.ca_flags = cf->cf_flags;

	while ((*cf->cf_attach->ca_match)(parent, cf, &ca) > 0) {
		config_attach(parent, cf, &ca, mbprint);
		if (cf->cf_fstate != FSTATE_STAR)
			break;
	}
	return (0);
}

static void
mbattach(parent, self, aux)
	struct device *parent, *self;
 	void *aux;
{
	printf("\n");
	config_search(mbsearch, self, NULL);
}
