/*	$NetBSD: autoconf.c,v 1.21.2.2 2017/12/03 11:36:14 jdolecek Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe for
 *      the NetBSD project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.21.2.2 2017/12/03 11:36:14 jdolecek Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <machine/config_hook.h>

#include "opt_cputypes.h"
#if defined(CPU_SA1100) || defined(CPU_SA1110)
#include "sacom.h"
#endif

extern dev_t dumpdev;

void isa_intr_init(void);

#ifndef MEMORY_DISK_IS_ROOT
static void get_device(const char *);
static void set_root_device(void);
#endif

#ifndef MEMORY_DISK_IS_ROOT
/* Decode a device name to a major and minor number */
static void
get_device(const char *name)
{
	int unit, part;
	char devname[16];
	const char *cp;
	device_t dv;

	if (strncmp(name, "/dev/", 5) == 0)
		name += 5;

	if (devsw_name2blk(name, devname, sizeof(devname)) == -1)
		return;

	name += strlen(devname);
	unit = part = 0;

	cp = name;
	while (*cp >= '0' && *cp <= '9')
		unit = (unit * 10) + (*cp++ - '0');
	if (cp == name)
		return;

	if (*cp >= 'a' && *cp <= ('a' + MAXPARTITIONS))
		part = *cp - 'a';
	else if (*cp != '\0' && *cp != ' ')
		return;
	if ((dv = device_find_by_driver_unit(devname, unit)) != NULL) {
		booted_device = dv;
		booted_partition = part;
	}
}

/* Set the rootdev variable from the root specifier in the boot args */
static void
set_root_device(void)
{

	if (boot_file[0] != '\0')
		get_device(boot_file);
	else
		/* hpcboot doesn't pass a bootdev arg if wd0 */
		get_device("wd0");
}
#endif /* ifndef MEMORY_DISK_IS_ROOT */

/*
 * Set up the root device from the boot args
 */
void
cpu_rootconf(void)
{
#ifndef MEMORY_DISK_IS_ROOT
	set_root_device();

	printf("boot device: %s\n",
	    booted_device != NULL ? device_xname(booted_device) : "<unknown>");
#endif
	rootconf();
}


/*
 * void cpu_configure()
 *
 * Configure all the root devices
 * The root devices are expected to configure their own children
 */
void
cpu_configure(void)
{

	config_hook_init();

	/*
	 * Configure all the roots.
	 * We have to have a mainbus
	 */
#if 0
	startrtclock();
#endif

	/*
	 * Since the ICU is not standard on the ARM we don't know
	 * if we have one until we find a bridge.
	 * Since various PCI interrupts could be routed via the ICU
	 * (for PCI devices in the bridge) we need to set up the ICU
	 * now so that these interrupts can be established correctly
	 * i.e. This is a hack.
	 */

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	/* Debugging information */
#if defined(DIAGNOSTIC)
#if defined(CPU_SA1100) || defined(CPU_SA1110)
	dump_spl_masks();
#endif
#endif	/* DIAGNOSTIC */

	/* Time to start taking interrupts so lets open the flood gates .... */
	(void)spl0();
}

void
device_register(device_t dev, void *aux)
{
}

/*
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console would be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */

#include "biconsdev.h"

#include <dev/cons.h>

cons_decl(sacom);
#define biconscnpollc	nullcnpollc
cons_decl(bicons);   

struct consdev constab[] = {
#if (NSACOM > 0)
	cons_init(sacom),
#endif
#if (NBICONSDEV > 0)
	cons_init(bicons),
#endif
	{ NULL },
};
