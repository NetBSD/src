/*	$NetBSD: imx7_wdog.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx7_wdog.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <arm/imx/imx7var.h>
#include <arm/imx/imx7reg.h>
#include <arm/imx/imxwdogvar.h>

/* ARGSUSED */
int
wdog_match(device_t parent __unused, struct cfdata *cf __unused, void *aux)
{
	struct axi_attach_args *aa = aux;

	switch (aa->aa_addr) {
	case IMX7_AIPS_BASE + AIPS1_WDOG1_BASE:
	case IMX7_AIPS_BASE + AIPS1_WDOG2_BASE:
	case IMX7_AIPS_BASE + AIPS1_WDOG3_BASE:
	case IMX7_AIPS_BASE + AIPS1_WDOG4_BASE:
		return 1;
	}

	return 0;
}

/* ARGSUSED */
void
wdog_attach(device_t parent __unused, device_t self, void *aux)
{
	struct axi_attach_args *aa = aux;

	wdog_attach_common(parent, self, aa->aa_iot, aa->aa_addr,
	    AIPS1_WDOG_SIZE, aa->aa_irq);
}
