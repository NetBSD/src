/*	$NetBSD: autoconf.c,v 1.2.2.2 2002/04/17 00:04:44 nathanw Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/stdarg.h>

#include <dev/ofw/openfirm.h>
#include <dev/pci/pcivar.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

void canonicalize_bootpath(void);
int OF_interpret(char *cmd, int nreturns, ...);

extern char bootpath[256];
char cbootpath[256];
struct device *booted_device;	/* boot device */
int booted_partition;		/* ...and partition on that device */

u_int *heathrow_FCR = NULL;

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{

	intr_init();
	calc_delayconst();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

        printf("biomask %x netmask %x ttymask %x\n",
	       (u_short)imask[IPL_BIO], (u_short)imask[IPL_NET],
	       (u_short)imask[IPL_TTY]);
	
	(void)spl0();

	/*
	 * Now allow hardware interrupts.
	 */
	asm volatile ("wrteei 1");
}

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf(void)
{
	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

