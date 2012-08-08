/*	$NetBSD: obs405_autoconf.c,v 1.5.8.1 2012/08/08 15:51:09 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: obs405_autoconf.c,v 1.5.8.1 2012/08/08 15:51:09 martin Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/device_if.h>
#include <sys/cpu.h>

#include <machine/obs405.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>


void
cpu_rootconf(void)
{

	rootconf();
}

void
obs405_device_register(device_t dev, void *aux, int com_freq)
{
	device_t parent = device_parent(dev);

	/* register "com" device */
	if (device_is_a(dev, "com") && device_is_a(parent, "opb")) {
		/* Set the frequency of the on-chip UART. */
		com_opb_device_register(dev, com_freq);
		return;
	}

	ibm4xx_device_register(dev, aux);
}
