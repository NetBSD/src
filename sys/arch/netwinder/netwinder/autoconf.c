/*	$NetBSD: autoconf.c,v 1.8.2.1 2007/12/26 22:24:51 rjs Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.8.2.1 2007/12/26 22:24:51 rjs Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <machine/bootconfig.h>
#include <machine/intr.h>

#include "isa.h"

extern int footbridge_imask[NIPL];

void isa_intr_init (void);

/*
 * Set up the root device from the boot args
 */
void
cpu_rootconf(void)
{
	printf("boot device: %s\n",
	    booted_device != NULL ? booted_device->dv_xname : "<unknown>");
	setroot(booted_device, booted_partition);
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
	 * Since various PCI interrupts could be routed via the ICU
	 * (for PCI devices in the bridge) we need to set up the ICU
	 * now so that these interrupts can be established correctly
	 * i.e. This is a hack.
	 */
	isa_intr_init();

	config_rootfound("mainbus", NULL);

#if defined(DEBUG)
	/* Debugging information */
	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_vm=%08x\n",
	    footbridge_imask[IPL_BIO], footbridge_imask[IPL_NET],
	    footbridge_imask[IPL_TTY], footbridge_imask[IPL_VM]);

	printf("ipl_audio=%08x ipl_imp=%08x ipl_high=%08x ipl_serial=%08x\n",
	    footbridge_imask[IPL_AUDIO], footbridge_imask[IPL_CLOCK],
	    footbridge_imask[IPL_HIGH], footbridge_imask[IPL_SERIAL]);
#endif /* defined(DEBUG) */

	/* Time to start taking interrupts so lets open the flood gates .... */
	(void)spl0();
}

void
device_register(struct device *dev, void *aux)
{
}
/* End of autoconf.c */
