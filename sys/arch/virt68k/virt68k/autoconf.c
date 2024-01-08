/*	$NetBSD: autoconf.c,v 1.5 2024/01/08 05:10:51 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 * 	The Regents of the University of California. All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 * 
 *	@(#)autoconf.c  8.2 (Berkeley) 1/12/94
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.5 2024/01/08 05:10:51 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/bootinfo.h>
#include <machine/intr.h>

#include <virt68k/dev/mainbusvar.h>

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure(void)
{

	booted_device = NULL;	/* set by device drivers (if found) */

	/* Initialise interrupt handlers */
	intr_init();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");

	/*
	 * XXX Go ahead and enable interrupts now that we've configured
	 * XXX everything.
	 * XXX
	 * XXX See locore.s, right before main() is called.
	 */
	spl0();
}

void
cpu_rootconf(void)
{
	bootinfo_setup_initrd();
	rootconf();
}

paddr_t	consdev_addr;

void
device_register(device_t dev, void *aux)
{
	device_t parent = device_parent(dev);
	char rootname[DEVICE_XNAME_SIZE];
	bool rootname_valid = false;

	if (booted_device == NULL) {
		rootname_valid = bootinfo_getarg("root",
		    rootname, sizeof(rootname));
		if (rootname_valid) {
			size_t len = strlen(rootname);
			while (len) {
				len--;
				if (isdigit(rootname[len])) {
					break;
				}
				rootname[len] = '\0';
			}
			if (strcmp(device_xname(dev), rootname) == 0) {
				booted_device = dev;
			}
		}
	}

	if (device_is_a(parent, "mainbus")) {
		struct mainbus_attach_args *ma = aux;

		if (ma->ma_addr == consdev_addr) {
			prop_dictionary_set_bool(device_properties(dev),
						 "is-console", true);
		}
	}
}
