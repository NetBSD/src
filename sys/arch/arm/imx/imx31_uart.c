/*
 * Copyright (c) 2009, 2010  Genetec Corporation.  All rights reserved.
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
 *
 */

#include "opt_imxuart.h"
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <arm/imx/imx31reg.h>
#include <arm/imx/imx31var.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>

static int imx31_uart_match(device_t, struct cfdata *, void *);
static void imx31_uart_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imx31_uart, sizeof(struct imxuart_softc),
    imx31_uart_match, imx31_uart_attach, NULL, NULL);

int
imx31_uart_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct aips_attach_args * const aipsa = aux;

	switch (aipsa->aipsa_addr) {
	case UART1_BASE:
	case UART2_BASE:
	case UART3_BASE:
	case UART4_BASE:
	case UART5_BASE:
		return 1;
	}

	return 0;
}

void
imx31_uart_attach(device_t parent, device_t self, void *aux)
{
	struct aips_attach_args * aa = aux;

	imxuart_attach_common(parent, self,
	    aa->aipsa_memt, aa->aipsa_addr, aa->aipsa_size, aa->aipsa_intr, 0);
}

