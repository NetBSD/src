/*	$NetBSD: autoconf.c,v 1.19.2.2 2017/12/03 11:36:43 jdolecek Exp $	*/

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
 * RiscBSD kernel project
 *
 * autoconf.c
 *
 * Autoconfiguration functions
 *
 * Created      : 08/10/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.19.2.2 2017/12/03 11:36:43 jdolecek Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <machine/irqhandler.h>

#include "isa.h"

#ifdef SHARK
#include <shark/shark/sequoia.h>
extern void	ofrootfound(void);
extern void	ofw_device_register(device_t, void *);
extern void	startrtclock(void);
#endif

#if defined(OFWGENCFG) || defined(SHARK)
/* Temporary for SHARK! */
extern void ofw_device_register(device_t, void *);
#include <machine/ofw.h>
#endif

extern dev_t dumpdev;

void isa_intr_init(void);

#ifndef MEMORY_DISK_IS_ROOT
static void get_device(char *name);
static void set_root_device(void);
#endif

#ifndef MEMORY_DISK_IS_ROOT
/* Decode a device name to a major and minor number */

static void
get_device(char *name)
{
	int unit, part;
	char devname[16], *cp;
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
	char *ptr;
            
	if (boot_file)
		get_device(boot_file);
	if (boot_args &&
	    get_bootconf_option(boot_args, "root", BOOTOPT_TYPE_STRING, &ptr))
		get_device(ptr);
}
#endif

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
	/*
	 * Configure all the roots.
	 * We have to have a mainbus
	 */

#ifdef SHARK
	sequoiaInit();
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
#if NISA > 0 && !defined(SHARK)
	isa_intr_init();
#endif

	config_rootfound("mainbus", NULL);
#if defined(OFWGENCFG) || defined(SHARK)
	/* Temporary for SHARK! */
	ofrootfound();
#endif

#if defined(DIAGNOSTIC)
	dump_spl_masks();
#endif /* defined(DIAGNOSTIC) */

	/* Time to start taking interrupts so lets open the flood gates .... */
	(void)spl0();
}

void
device_register(device_t dev, void *aux)
{
#if defined(OFWGENCFG) || defined(SHARK)
	/* Temporary for SHARK! */
	ofw_device_register(dev, aux);
#endif
}
/* End of autoconf.c */
