/* $NetBSD: avalon_a12.c,v 1.11.6.1 2006/04/22 11:37:10 simonb Exp $ */

/* [Notice revision 2.2]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AVALON OR THE CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1995, 1996, 1997 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: avalon_a12.c,v 1.11.6.1 2006/04/22 11:37:10 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/prom.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include "a12dc.h"

#if NA12DC
#include "alpha/a12/a12dcreg.h"
#endif

#ifdef __never
#define	AVALON_A12()	/* Generate ctags(1) key */
#endif

void avalon_a12_init __P((void));
static void avalon_a12_cons_init __P((void));
static void avalon_a12_device_register __P((struct device *, void *));
static int  a12env __P((int));

void
avalon_a12_init()
{
	platform.family = platform.model =
		"Avalon A12 Scalable Parallel Processor";
	platform.iobus = "a12c";
	platform.cons_init = avalon_a12_cons_init;
	platform.device_register = avalon_a12_device_register;

	a12_cpu_local = a12env(A12CONS_CPU_LOCAL); /* our switch port */
	a12_cpu_ether = a12env(A12CONS_CPU_ETHER); /* route to outside world */
	a12_cpu_global= a12env(A12CONS_CPU_GLOBAL);/* loc in the big picture */
	a12_intr_register_icw(NULL);
}
/*
 * We _could_ get these configuration parameters directly from the
 * detached console, and thereby nuke yet another deprecated prom
 * call.
 */
static int
a12env(code)
	int    code;
{
	int	q;
	char	string[16];

	prom_getenv(code, string, sizeof(string));
	string[sizeof(string)-1] = '\0';
	q = string[0] - '0';
	if (string[0] && string[1] >= '0')
		q = q*10 + string[1] - '0';
	return q;
}

void
avalon_a12_cons_init()
{
#if NA12DC
	a12dccnattach();
#endif
}

void
avalon_a12_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	static int found, initted, diskboot, netboot;
	static struct device *pcidev, *ctrlrdev;
	struct bootdev_data *b = bootdev_data;
	struct device *parent = device_parent(dev);

	if (found)
		return;

	if (!initted) {
		diskboot = (strcasecmp(b->protocol, "SCSI") == 0);
		netboot = (strcasecmp(b->protocol, "BOOTP") == 0) ||
		    (strcasecmp(b->protocol, "MOP") == 0);
#if 0
		printf("diskboot = %d, netboot = %d\n", diskboot, netboot);
#endif
		initted =1;
	}

	if (pcidev == NULL) {
		if (!device_is_a(dev, "pci"))
			return;
		else {
			struct pcibus_attach_args *pba = aux;

			if ((b->slot / 1000) != pba->pba_bus)
				return;
	
			pcidev = dev;
#if 0
			printf("\npcidev = %s\n", dev->dv_xname);
#endif
			return;
		}
	}

	if (ctrlrdev == NULL) {
		if (parent != pcidev)
			return;
		else {
			struct pci_attach_args *pa = aux;
			int slot;

			slot = pa->pa_bus * 1000 + pa->pa_function * 100 +
			    pa->pa_device;
			if (b->slot != slot)
				return;
	
			if (netboot) {
				booted_device = dev;
#if 0
				printf("\nbooted_device = %s\n", dev->dv_xname);
#endif
				found = 1;
			} else {
				ctrlrdev = dev;
#if 0
				printf("\nctrlrdev = %s\n", dev->dv_xname);
#endif
			}
			return;
		}
	}

	if (!diskboot)
		return;

	if (device_is_a(dev, "sd") ||
	    device_is_a(dev, "st") ||
	    device_is_a(dev, "cd")) {
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;
		int unit;

		if (device_parent(parent) != ctrlrdev)
			return;

		unit = periph->periph_target * 100 + periph->periph_lun;
		if (b->unit != unit)
			return;
		if (b->channel != periph->periph_channel->chan_channel)
			return;

		/* we've found it! */
		booted_device = dev;
#if 0
		printf("\nbooted_device = %s\n", dev->dv_xname);
#endif
		found = 1;
	}
}
