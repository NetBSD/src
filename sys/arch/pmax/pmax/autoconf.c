/*	$NetBSD: autoconf.c,v 1.31.4.6 1999/10/26 03:45:44 nisimura Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: autoconf.c 1.31 91/01/21
 *
 *	@(#)autoconf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.31.4.6 1999/10/26 03:45:44 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>

struct intrhand intrtab[MAX_DEV_NCOOKIES];
u_int32_t iplmask[IPL_HIGH+1];		/* interrupt mask bits for each IPL */
u_int32_t oldiplmask[IPL_HIGH+1];	/* old values for splx(s) */

struct device	*booted_device;
int	booted_slot, booted_unit, booted_partition;
char	*booted_protocol;

void	calculate_iplmask __P((void));
int	nullintr __P((void *));

void
cpu_configure()
{
	int s;

	/* Initialize interrupt table array */
	for (s = 0; s < MAX_DEV_NCOOKIES; s++) {
		intrtab[s].ih_func = nullintr;
		intrtab[s].ih_arg = (void *)s;
	}

	(void)splhigh();
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");
	/* Reset any bus errors due to probing nonexistent devices. */
	(*platform.bus_reset)();
	calculate_iplmask();
	_splnone();	/* enable all source forcing SOFT_INTs cleared */
	
	if (cn_tab->cn_dev == NODEV)
		panic("No console driver.  Check kernel configuration.");
}

void
consinit()
{
	(*platform.cons_init)();
}

void
cpu_rootconf()
{
	printf("biomask %x netmask %x ttymask %x\n",
	    iplmask[IPL_BIO], iplmask[IPL_NET], iplmask[IPL_TTY]);
#if 0
	printf("boot configuration: via %s, slot %d, unit %d, part %d\n ",
		booted_protocol ? booted_protocol : "UNKNOWN",
		booted_slot, booted_unit, booted_partition);
#endif

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

/*
 * Look at the string 'cp' and decode the boot device.
 * Boot names can be something like 'rz(0,0,0)vmunix' or '5/rz0/vmunix'.
 */
void
makebootdev(cp)
	char *cp;
{
	booted_device = NULL;
	booted_slot = booted_unit = -1;
	booted_partition = 0;
	booted_protocol = NULL;

	if (cp[0] == 'r' && cp[1] == 'z' && cp[2] == '(') {
		if (cp[3] >= '0' && cp[3] <= '9' && cp[4] == ','
		    && cp[5] >= '0' && cp[5] <= '9' && cp[6] == ','
		    && cp[7] >= '0' && cp[7] <= '9' && cp[8] == ')')
			return;
		booted_slot = cp[3] - '0';
		booted_unit = cp[5] - '0';
		booted_partition = cp[7] - '0';
		booted_protocol = "SCSI";
	}
	if (cp[0] >= '0' && cp[0] <= '9' && cp[1] == '/') {
		booted_slot = cp[0] - '0';
		booted_unit = booted_partition = 0;
		if (cp[2] == 'r' && cp[3] == 'z'
		    && cp[4] >= '0' && cp[4] <= '9') {
			booted_protocol = "SCSI";
			booted_unit = cp[4] - '0';
		}
		else if (strncmp(cp+2, "tftp", 4) == 0)
			booted_protocol = "BOOTP";
		else if (strncmp(cp+2, "mop", 3) == 0)
			booted_protocol = "MOP";
	}
}

void
calculate_iplmask()
{
	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	iplmask[IPL_NONE] = 0;
	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	iplmask[IPL_BIO] |= iplmask[IPL_NONE];
	iplmask[IPL_NET] |= iplmask[IPL_BIO];
	iplmask[IPL_TTY] |= iplmask[IPL_NET];
	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	iplmask[IPL_IMP] |= iplmask[IPL_TTY];
	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	iplmask[IPL_CLOCK] |= iplmask[IPL_IMP];
	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	iplmask[IPL_HIGH] |= iplmask[IPL_CLOCK];
}

int
nullintr(v)
	void *v;
{
	printf("uncaught interrupt: cookie %d\n", (int)v);
	return 1;
}

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

struct xx_softc {
	struct device sc_dev;
	struct disk sc_dk;
	int flag;
	struct scsipi_link *sc_link;
};	
#define	SCSITARGETID(dev) ((struct xx_softc *)dev)->sc_link->scsipi_scsi.target

int slot_in_progress; /* XXX - TC slot being probed, ugly backdoor interface */

void
dk_establish(dk, dev)
	struct disk *dk;
	struct device *dev;
{
	if (booted_device || strcmp(booted_protocol, "SCSI"))
		return;
	if (booted_slot != slot_in_progress)
		return;
	if (booted_unit != SCSITARGETID(dev))
		return;
	booted_device = dev;
}
