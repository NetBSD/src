/*	$NetBSD: obs266_autoconf.c,v 1.2.8.1 2006/05/24 10:56:47 yamt Exp $	*/

/*
 * Copyright 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima for The NetBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obs266_autoconf.c,v 1.2.8.1 2006/05/24 10:56:47 yamt Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <machine/obs266.h>

#include <powerpc/ibm4xx/dcr405gp.h>


/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{

	intr_init();
	calc_delayconst();

	/* Make sure that timers run at CPU frequency */
	mtdcr(DCR_CPC0_CR1, mfdcr(DCR_CPC0_CR1) & ~CPC0_CR1_CETE);

	if (config_rootfound("plb", NULL) == NULL)
		panic("configure: mainbus not configured");

	printf("biomask %x netmask %x ttymask %x\n", (u_short)imask[IPL_BIO],
		(u_short)imask[IPL_NET], (u_short)imask[IPL_TTY]);

	(void)spl0();

	/*
	 * Now allow hardware interrupts.
	 */
	__asm volatile ("wrteei 1");
}

void device_register(struct device *dev, void *aux)
{

	obs405_device_register(dev, aux, OBS266_COM_FREQ);
}
