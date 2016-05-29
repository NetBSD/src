/*	$NetBSD: imx7_uart.c,v 1.1.2.2 2016/05/29 08:44:16 skrll Exp $	*/

/*
 * Copyright (c) 2013 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx7_uart.c,v 1.1.2.2 2016/05/29 08:44:16 skrll Exp $");

#include "opt_imxuart.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7var.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>

/* ARGSUSED */
int
imxuart_match(device_t parent __unused, struct cfdata *cf __unused, void *aux)
{
	struct axi_attach_args * const aa = aux;

	switch (aa->aa_addr) {
	case (IMX7_AIPS_BASE + AIPS3_UART1_BASE):
	case (IMX7_AIPS_BASE + AIPS3_UART2_BASE):
	case (IMX7_AIPS_BASE + AIPS3_UART3_BASE):
	case (IMX7_AIPS_BASE + AIPS3_UART4_BASE):
	case (IMX7_AIPS_BASE + AIPS3_UART5_BASE):
	case (IMX7_AIPS_BASE + AIPS3_UART6_BASE):
	case (IMX7_AIPS_BASE + AIPS3_UART7_BASE):
		return 1;
	}

	return 0;
}

void
imxuart_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * aa = aux;

	imxuart_attach_common(parent, self,
	    aa->aa_iot, aa->aa_addr, aa->aa_size, aa->aa_irq, 0);
}
