/*	$NetBSD: autoconf.c,v 1.2 2026/06/16 23:37:49 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

/*
 * Autoconfiguration for the ACube Sam460ex (AMCC 460EX).
 * Modeled on evbppc/obs405/obs600_autoconf.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2 2026/06/16 23:37:49 rkujawa Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <machine/sam460ex.h>

#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{

	/* UIC */
	mtdcr(DCR_UIC1_BASE + DCR_UIC_PR,
	    mfdcr(DCR_UIC1_BASE + DCR_UIC_PR) & ~0x80000000);
	mtdcr(DCR_UIC1_BASE + DCR_UIC_TR,
	    mfdcr(DCR_UIC1_BASE + DCR_UIC_TR) & ~0x80000000);
	mtdcr(DCR_UIC1_BASE + DCR_UIC_SR, 0x80000000);
	mtdcr(DCR_UIC3_BASE + DCR_UIC_PR,
	    mfdcr(DCR_UIC3_BASE + DCR_UIC_PR) & ~0x000ff000);
	mtdcr(DCR_UIC3_BASE + DCR_UIC_TR,
	    mfdcr(DCR_UIC3_BASE + DCR_UIC_TR) & ~0x000ff000);
	mtdcr(DCR_UIC3_BASE + DCR_UIC_SR, 0x000ff000);

	/*
	 * Initialize intr and add the cascaded UICs.
	 * UIC0 = 0, UIC1 = +32, UIC2 = +64, UIC3 = +96.
	 */
	intr_init();
	pic_add(&pic_uic1);
	pic_add(&pic_uic2);
	pic_add(&pic_uic3);

	if (config_rootfound("plb", NULL) == NULL)
		panic("configure: mainbus not configured");

	pic_finish_setup();

	genppc_cpu_configure();
}

void
device_register(device_t dev, void *aux)
{

	ibm4xx_device_register(dev, aux, sam460ex_com_freq());

	/* "console=fb" in bootargs: the SM502 wsdisplay becomes console */
	if (sam460ex_console_fb && device_is_a(dev, "voyagerfb"))
		prop_dictionary_set_bool(device_properties(dev),
		    "is_console", true);

	/*
	 * Match a "root=" from the bootargs.
	 */
	if (bootspec != NULL) {
		size_t len = strlen(bootspec);
		int part = 0;

		if (len > 1 && bootspec[len - 1] >= 'a' &&
		    bootspec[len - 1] < 'a' + MAXPARTITIONS &&
		    bootspec[len - 2] >= '0' && bootspec[len - 2] <= '9') {
			part = bootspec[len - 1] - 'a';
			len--;
		}
		if (strncmp(device_xname(dev), bootspec, len) == 0 &&
		    device_xname(dev)[len] == '\0') {
			booted_device = dev;
			booted_partition = part;
		}
	}
}

