/*	$NetBSD: zynq7000_uart.c,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq7000_uart.c,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $");

#include "opt_zynq.h"
#include "opt_zynquart.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <arm/zynq/zynq7000_reg.h>
#include <arm/zynq/zynq7000_var.h>
#include <arm/zynq/zynq_uartreg.h>
#include <arm/zynq/zynq_uartvar.h>

int
zynquart_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct axi_attach_args * const aa = aux;

	switch (aa->aa_addr) {
	case UART0_BASE:
	case UART1_BASE:
		return 1;
	}

	return 0;
}

void
zynquart_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * aa = aux;

	zynquart_attach_common(parent, self,
	    aa->aa_iot, aa->aa_addr, aa->aa_size, aa->aa_irq, 0);
}

