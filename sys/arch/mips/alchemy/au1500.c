/* $NetBSD: au1500.c,v 1.6.12.1 2007/02/27 16:52:00 yamt Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: au1500.c,v 1.6.12.1 2007/02/27 16:52:00 yamt Exp $");

#include <sys/param.h>
#include <machine/bus.h>
#include <machine/locore.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>

static const char *au1500_irqnames[] = {
	"uart0",
	"pci inta",
	"pci intb",
	"uart3",
	"pci intc",
	"pci intd",
	"dma0",
	"dma1",
	"dma2",
	"dma3",
	"dma4",
	"dma5",
	"dma6",
	"dma7",
	"toy tick",
	"toy match0",
	"toy match1",
	"toy match2",
	"rtc tick",
	"rtc match0",
	"rtc match1",
	"rtc match2",
	"pci error",
	"irq 23",
	"usb intr",
	"usb suspend",
	"usb host",
	"ac97",
	"mac0",
	"mac1",
	"irq 30",
	"ac97 cmd",
	"gpio 0",
	"gpio 1",
	"gpio 2",
	"gpio 3",
	"gpio 4",
	"gpio 5",
	"gpio 6",
	"gpio 7",
	"gpio 8",
	"gpio 9",
	"gpio 10",
	"gpio 11",
	"gpio 12",
	"gpio 13",
	"gpio 14",
	"gpio 15",
	"gpio 200",
	"gpio 201",
	"gpio 202",
	"gpio 203",
	"gpio 20",
	"gpio 204",
	"gpio 205",
	"gpio 23",
	"gpio 24",
	"gpio 25",
	"gpio 26",
	"gpio 27",
	"gpio 28",
	"gpio 206",
	"gpio 207",
	"gpio 208:215"
};

static struct au_dev au1500_devices[] = {
	{ "com",	{ UART0_BASE },				   {  0, -1 }},
	{ "com",	{ UART3_BASE },				   {  3, -1 }},
	{ "aurtc",	{ -1 },					   { -1, -1 }},
	{ "aumac",	{ AU1500_MAC0_BASE, AU1500_MAC0_ENABLE,
			  MAC0_DMA_BASE },			   { 28, -1 }},
	{ "aumac",	{ AU1500_MAC1_BASE, AU1500_MAC1_ENABLE,
			  MAC1_DMA_BASE },			   { 29, -1 }},
	{ "ohci",	{ USBH_BASE, USBH_ENABLE, USBH_SIZE },	   { 26, -1 }},
	{ "aupci",	{ PCI_BASE },				   { -1, -1 }},
	{ "augpio",	{ GPIO_BASE, 23 },			   { -1, -1 }},
	{ "augpio",	{ GPIO2_BASE, 16 },			   { -1, -1 }},
#if 0
	{ "auaudio",	{ AC97_BASE },				   { 27, 31 }},
	{ "usbd",	{ USBD_BASE },				   { 24, 25 }},
#endif
	{ NULL }
};

static struct au_chipdep au1500_chipdep = {
	"au1500",
	{ IC0_BASE, IC1_BASE },		/* ICUs */
	au1500_devices,
	au1500_irqnames,
};

bool
au1500_match(struct au_chipdep **cpp)
{

	if (MIPS_PRID_COPTS(cpu_id) == MIPS_AU1500) {
		*cpp = &au1500_chipdep;
		return TRUE;
	}
	return FALSE;
}
