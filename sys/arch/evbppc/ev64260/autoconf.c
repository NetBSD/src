/*	$NetBSD: autoconf.c,v 1.1 2003/03/05 22:08:26 matt Exp $	*/

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

struct device *booted_device;
int booted_partition;

void findroot __P((void));

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure()
{
        extern void init_interrupt __P((void));
        extern void calc_delayconst __P((void));

	extintr_disable();

        init_interrupt();
        calc_delayconst();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	printf("biomask %x netmask %x ttymask %x\n",
	    (u_short)imask[IPL_BIO], (u_short)imask[IPL_NET],
	    (u_short)imask[IPL_TTY]);

	curcpu()->ci_cpl = IPL_NONE;
	extintr_enable();

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

dev_t	bootdev = 0;

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
findroot(void)
{
	struct device *dv;
	const char *name;
	char buf[32];

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;


	name = devsw_blk2name(B_TYPE(bootdev));
	if (name == NULL)
		return;

	sprintf(buf, "%s%d", name, B_UNIT(bootdev));
	TAILQ_FOREACH(dv, &alldevs, dv_list) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			booted_device = dv;
			booted_partition = B_PARTITION(bootdev);
			return;
		}
	}
}
