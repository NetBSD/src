/*	$NetBSD: autoconf.c,v 1.3 2001/06/10 03:16:31 briggs Exp $	*/

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
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <powerpc/mpc6xx/pte.h>
#include <machine/intr.h>

struct device *booted_device;
int booted_partition;

void findroot __P((void));
void disable_intr(void);
void enable_intr(void);

void identifycpu(void);

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure()
{
	/* startrtclock(); */

	disable_intr();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	printf("biomask %x netmask %x ttymask %x\n",
	    (u_short)imask[IPL_BIO], (u_short)imask[IPL_NET],
	    (u_short)imask[IPL_TTY]);

	enable_intr();

	spl0();
}

void
cpu_rootconf()
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(void)
{
	int i, majdev, unit, part;
	struct device *dv;
	char buf[32];

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	for (i = 0; dev_name2blk[i].d_name != NULL; i++)
		if (majdev == dev_name2blk[i].d_maj)
			break;
	if (dev_name2blk[i].d_name == NULL)
		return;

	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	sprintf(buf, "%s%d", dev_name2blk[i].d_name, unit);
	for (dv = alldevs.tqh_first; dv != NULL;
	    dv = dv->dv_list.tqe_next) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			booted_device = dv;
			booted_partition = part;
			return;
		}
	}
}

#define MPC601		0x01
#define MPC603		0x03
#define MPC604		0x04
#define MPC602		0x05
#define MPC603e		0x06
#define MPC603ev	0x07
#define MPC750		0x08
#define MPC604ev	0x09
#define MPC7400		0x0c
#define MPC620		0x14
#define MPC8240		0x81

int cpu;
char cpu_model[80];
char cpu_name[] = "PowerPC";	/* cpu architecture */

struct cputab {
	int	version;
	char	*name;
};
static struct cputab models[] = {
	{ MPC601,     "601" },
	{ MPC603,     "603" },
	{ MPC604,     "604" },
	{ MPC602,     "602" },
	{ MPC603e,   "603e" },
	{ MPC603ev, "603ev" },
	{ MPC750,     "750" },
	{ MPC604ev, "604ev" },
	{ MPC7400,   "7400" },
	{ MPC620,     "620" },
	{ MPC8240,   "8240" },
	{ 0, NULL }
};

void
identifycpu()
{
	int pvr;
	struct cputab *cp = models;

	/*
	 * Find cpu type
	 */
	asm ("mfpvr %0" : "=r"(pvr));
	cpu = pvr >> 16;
	while (cp->name) {
		if (cp->version == cpu)
			break;
		cp++;
	}
	if (cp->name)
		strcpy(cpu_model, cp->name);
	else
		sprintf(cpu_model, "Version 0x%x", cpu);

	sprintf(cpu_model + strlen(cpu_model), " (Revision %d.%d)",
		(pvr >> 8) & 0xff, pvr & 0xff);

	printf("CPU: %s %s\n", cpu_name, cpu_model);
}

