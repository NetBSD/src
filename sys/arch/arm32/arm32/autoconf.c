/*	$NetBSD: autoconf.c,v 1.16 1997/03/26 22:38:52 gwr Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <machine/bootconfig.h>
#include <machine/irqhandler.h>

#include "wdc.h"
#include "fdc.h"
#include "md.h"
#include "sd.h"
#include "cd.h"
#include "podulebus.h"

static	struct device *booted_device;
static	int booted_partition;

extern dev_t dumpdev;

extern int pmap_debug_level;

char *	strstr	__P((char *s1, char *s2));
void	dumpconf __P(());

/* Table major numbers for the device names, NULL terminated */

struct devnametobdevmaj arm32_nam2blk[] = {
	{ "wd",		16 },
	{ "fd",		17 },
	{ "md",		18 },
	{ "sd",		24 },
	{ "cd",		26 },
	{ NULL,		0 },
};

/* Decode a device name to a major and minor number */

void
get_device(name, devpp, partp)
	char *name;
	struct device **devpp;
	int *partp;
{
	int loop, unit, part;
	char buf[32], *cp;
	struct device *dv;

	*devpp = NULL;
	*partp = 0;
    
	if (strncmp(name, "/dev/", 5) == 0)
		name += 5;

	for (loop = 0; arm32_nam2blk[loop].d_name != NULL; ++loop) {
		if (strncmp(name, arm32_nam2blk[loop].d_name,
		    strlen(arm32_nam2blk[loop].d_name)) == 0) {
			name += strlen(arm32_nam2blk[loop].d_name);

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

			sprintf(buf, "%s%d", arm32_nam2blk[loop].d_name, unit);
			for (dv = alldevs.tqh_first; dv != NULL;
			    dv = dv->dv_list.tqe_next) {
				if (strcmp(buf, dv->dv_xname) == 0) {
					*devpp = dv;
					*partp = part;
					return;
				}
			}
		}
	} 
}


/* Set the rootdev variable from the root specifier in the boot args */

void
set_root_device()
{
	char *ptr;
            
	if (boot_args) {
		ptr = strstr(boot_args, "root=");
		if (ptr) {
			ptr += 5;
			get_device(ptr, &booted_device, &booted_partition);

		}
	}
}


/*
 * Set up the root and swap devices from the boot args
 */
void
cpu_rootconf()
{
	set_root_device();

	printf("boot device: %s\n",
	    booted_device != NULL ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, arm32_nam2blk);
}


/*
 * void configure()
 *
 * Configure all the root devices
 * The root devices are expected to configure their own children
 */

void
configure()
{
	/*
	 * Configure all the roots.
	 * We have to have a mainbus
	 */

	config_rootfound("mainbus", NULL);
#if NPODULEBUS > 0
	config_rootfound("podulebus", NULL);
#endif	/* NPODULEBUS */

	/* Debugging information */

	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_clock=%08x\n",
	    irqmasks[IPL_BIO], irqmasks[IPL_NET], irqmasks[IPL_TTY],
	    irqmasks[IPL_CLOCK]);
	printf("ipl_imp=%08x ipl_none=%08x\n", irqmasks[IPL_IMP],
	    irqmasks[IPL_NONE]);

	/* Time to start taking interrupts so lets open the flood gates .... */
         
	(void)spl0();
}

/* End of autoconf.c */
