/*	$NetBSD: pucdata.c,v 1.57 2009/08/29 13:55:29 tsutsui Exp $	*/

/*
 * Copyright (c) 1998, 1999 Christopher G. Demetriou.  All rights reserved.
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

/*
 * PCI "universal" communications card driver configuration data (used to
 * match/attach the cards).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pucdata.c,v 1.57 2009/08/29 13:55:29 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pucvar.h>
#include <dev/ic/comreg.h>

const struct puc_device_description puc_devices[] = {
	/*
	 * SUNIX 40XX series of serial/parallel combo cards.
	 * Tested with 4055A and 4065A.
	 */
	{   "SUNIX 400X 1P",
	    {	0x1409,	0x7168,	0x1409,	0x4000 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},

	{   "SUNIX 401X 2P",
	    {	0x1409,	0x7168,	0x1409,	0x4010 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	{   "SUNIX 402X 1S",
	    {	0x1409,	0x7168,	0x1409,	0x4020 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
	    },
	},

	{   "SUNIX 403X 2S",
	    {	0x1409,	0x7168,	0x1409,	0x4030 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
	    },
	},

	{   "SUNIX 405X 4S",
	    {	0x1409,	0x7168,	0x1409,	0x4050 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x14, 0x08, COM_FREQ},
	    },
	},

	{   "SUNIX 406X 8S",
	    {	0x1409,	0x7168,	0x1409,	0x4060 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x14, 0x08, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x24, 0x00, COM_FREQ},
	    },
	},

	{   "SUNIX 407X 2S/1P",
	    {	0x1409,	0x7168,	0x1409,	0x4070 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	{   "SUNIX 408X 2S/2P",
	    {	0x1409,	0x7168,	0x1409,	0x4080 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

	{   "SUNIX 409X 4S/2P",
	    {	0x1409,	0x7168,	0x1409,	0x4090 },
	    {	0xffff,	0xffff,	0xffff,	0xeff0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ},
		{ PUC_PORT_TYPE_COM, 0x14, 0x08, COM_FREQ},
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

	/*
	 * XXX no entry because I have no data:
	 * XXX Dolphin Peripherals 4006 (single parallel)
	 */

	/*
	 * Dolphin Peripherals 4014 (dual parallel port) card.  PLX 9050, with
	 * a seemingly-lame EEPROM setup that puts the Dolphin IDs
	 * into the subsystem fields, and claims that it's a
	 * network/misc (0x02/0x80) device.
	 */
	{   "Dolphin Peripherals 4014",
	    {	0x10b5,	0x9050,	0xd84d,	0x6810	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x24, 0x00, 0x00 },
	    },
	},

	/*
	 * XXX Dolphin Peripherals 4025 (single serial)
	 * (clashes with Dolphin Peripherals  4036 (2s variant)
	 */

	/*
	 * Dolphin Peripherals 4035 (dual serial port) card.  PLX 9050, with
	 * a seemingly-lame EEPROM setup that puts the Dolphin IDs
	 * into the subsystem fields, and claims that it's a
	 * network/misc (0x02/0x80) device.
	 */
	{   "Dolphin Peripherals 4035",
	    {	0x10b5,	0x9050,	0xd84d,	0x6808	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
	    },
	},

	/*
	 * Dolphin Peripherals 4036 (dual serial port) card.
	 * (Dolpin 4025 has the same ID but only one port)
	 */
	{   "Dolphin Peripherals 4036",
	    {	0x1409,	0x7168,	0x0,	0x0	},
	    {	0xffff,	0xffff,	0x0,	0x0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8},
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8},
	    },
	},

	/*
	 * XXX no entry because I have no data:
	 * XXX Dolphin Peripherals 4078 (dual serial and single parallel)
	 */


	/*
	 * SIIG Boards.
	 *
	 * SIIG provides documentation for their boards at:
	 * <URL:http://www.siig.com/driver.htm>
	 *
	 * Please excuse the weird ordering, it's the order they
	 * use in their documentation.
	 */

	/*
	 * SIIG "10x" family boards.
	 */

	/* SIIG Cyber Serial PCI 16C550 (10x family): 1S */
	{   "SIIG Cyber Serial PCI 16C550 (10x family)",
	    {	0x131f,	0x1000,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR1 },
	    },
	},

	/* SIIG Cyber Serial PCI 16C650 (10x family): 1S */
	{   "SIIG Cyber Serial PCI 16C650 (10x family)",
	    {	0x131f,	0x1001,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR1 },
	    },
	},

	/* SIIG Cyber Serial PCI 16C850 (10x family): 1S */
	{   "SIIG Cyber Serial PCI 16C850 (10x family)",
	    {	0x131f,	0x1002,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR1 },
	    },
	},

	/* SIIG Cyber I/O PCI 16C550 (10x family): 1S, 1P */
	{   "SIIG Cyber I/O PCI 16C550 (10x family)",
	    {	0x131f,	0x1010,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x1c, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber I/O PCI 16C650 (10x family): 1S, 1P */
	{   "SIIG Cyber I/O PCI 16C650 (10x family)",
	    {	0x131f,	0x1011,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x1c, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber I/O PCI 16C850 (10x family): 1S, 1P */
	{   "SIIG Cyber I/O PCI 16C850 (10x family)",
	    {	0x131f,	0x1012,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x1c, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber Parallel PCI (10x family): 1P */
	{   "SIIG Cyber Parallel PCI (10x family)",
	    {	0x131f,	0x1020,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber Parallel Dual PCI (10x family): 2P */
	{   "SIIG Cyber Parallel Dual PCI (10x family)",
	    {	0x131f,	0x1021,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber Serial Dual PCI 16C550 (10x family): 2S */
	{   "SIIG Cyber Serial Dual PCI 16C550 (10x family)",
	    {	0x131f,	0x1030,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR2 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR3 },
	    },
	},

	/* SIIG Cyber Serial Dual PCI 16C650 (10x family): 2S */
	{   "SIIG Cyber Serial Dual PCI 16C650 (10x family)",
	    {	0x131f,	0x1031,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR2 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR3 },
	    },
	},

	/* SIIG Cyber Serial Dual PCI 16C850 (10x family): 2S */
	{   "SIIG Cyber Serial Dual PCI 16C850 (10x family)",
	    {	0x131f,	0x1032,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR2 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR3 },
	    },
	},

	/* SIIG Cyber 2S1P PCI 16C550 (10x family): 2S, 1P */
	{   "SIIG Cyber 2S1P PCI 16C550 (10x family)",
	    {	0x131f,	0x1034,	0,	0	},	/* XXX really? */
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR2 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR3 },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2S1P PCI 16C650 (10x family): 2S, 1P */
	{   "SIIG Cyber 2S1P PCI 16C650 (10x family)",
	    {	0x131f,	0x1035,	0,	0	},	/* XXX really? */
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR2 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR3 },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2S1P PCI 16C850 (10x family): 2S, 1P */
	{   "SIIG Cyber 2S1P PCI 16C850 (10x family)",
	    {	0x131f,	0x1036,	0,	0	},	/* XXX really? */
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR2 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR3 },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 4S PCI 16C550 (10x family): 4S */
	{   "SIIG Cyber 4S PCI 16C550 (10x family)",
	    {	0x131f,	0x1050,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x24, 0x00, COM_FREQ * 8 },
	    },
	},

	/* SIIG Cyber 4S PCI 16C650 (10x family): 4S */
	{   "SIIG Cyber 4S PCI 16C650 (10x family)",
	    {	0x131f,	0x1051,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x24, 0x00, COM_FREQ * 8 },
	    },
	},

	/* SIIG Cyber 4S PCI 16C850 (10x family): 4S */
	{   "SIIG Cyber 4S PCI 16C850 (10x family)",
	    {	0x131f,	0x1052,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG10x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x24, 0x00, COM_FREQ * 8 },
	    },
	},

	/*
	 * SIIG "20x" family boards.
	 */

	/* SIIG Cyber Parallel PCI (20x family): 1P */
	{   "SIIG Cyber Parallel PCI (20x family)",
	    {	0x131f,	0x2020,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber Parallel Dual PCI (20x family): 2P */
	{   "SIIG Cyber Parallel Dual PCI (20x family)",
	    {	0x131f,	0x2021,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2P1S PCI 16C550 (20x family): 1S, 2P */
	{   "SIIG Cyber 2P1S PCI 16C550 (20x family)",
	    {	0x131f,	0x2040,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x14, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x1c, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2P1S PCI 16C650 (20x family): 1S, 2P */
	{   "SIIG Cyber 2P1S PCI 16C650 (20x family)",
	    {	0x131f,	0x2041,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x14, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x1c, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2P1S PCI 16C850 (20x family): 1S, 2P */
	{   "SIIG Cyber 2P1S PCI 16C850 (20x family)",
	    {	0x131f,	0x2042,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x14, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x1c, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber Serial PCI 16C550 (20x family): 1S */
	{   "SIIG Cyber Serial PCI 16C550 (20x family)",
	    {	0x131f,	0x2000,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
	    },
	},

	/* SIIG Cyber Serial PCI 16C650 (20x family): 1S */
	{   "SIIG Cyber Serial PCI 16C650 (20x family)",
	    {	0x131f,	0x2001,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
	    },
	},

	/* SIIG Cyber Serial PCI 16C850 (20x family): 1S */
	{   "SIIG Cyber Serial PCI 16C850 (20x family)",
	    {	0x131f,	0x2002,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
	    },
	},

	/* SIIG Cyber I/O PCI 16C550 (20x family): 1S, 1P */
	{   "SIIG Cyber I/O PCI 16C550 (20x family)",
	    {	0x131f,	0x2010,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x14, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber I/O PCI 16C650 (20x family): 1S, 1P */
	{   "SIIG Cyber I/O PCI 16C650 (20x family)",
	    {	0x131f,	0x2011,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x14, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber I/O PCI 16C850 (20x family): 1S, 1P */
	{   "SIIG Cyber I/O PCI 16C850 (20x family)",
	    {	0x131f,	0x2012,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_LPT, 0x14, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber Serial Dual PCI 16C550 (20x family): 2S */
	{   "SIIG Cyber Serial Dual PCI 16C550 (20x family)",
	    {	0x131f,	0x2030,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR1 },
	    },
	},

	/* SIIG Cyber Serial Dual PCI 16C650 (20x family): 2S */
	{   "SIIG Cyber Serial Dual PCI 16C650 (20x family)",
	    {	0x131f,	0x2031,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR1 },
	    },
	},

	/* SIIG Cyber Serial Dual PCI 16C850 (20x family): 2S */
	{   "SIIG Cyber Serial Dual PCI 16C850 (20x family)",
	    {	0x131f,	0x2032,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR1 },
	    },
	},

	/* SIIG Cyber 2S1P PCI 16C550 (20x family): 2S, 1P */
	{   "SIIG Cyber 2S1P PCI 16C550 (20x family)",
	    {	0x131f,	0x2060,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR1 },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2S1P PCI 16C650 (20x family): 2S, 1P */
	{   "SIIG Cyber 2S1P PCI 16C650 (20x family)",
	    {	0x131f,	0x2061,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR1 },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 2S1P PCI 16C850 (20x family): 2S, 1P */
	{   "SIIG Cyber 2S1P PCI 16C850 (20x family)",
	    {	0x131f,	0x2062,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR1 },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* SIIG Cyber 4S PCI 16C550 (20x family): 4S */
	{   "SIIG Cyber 4S PCI 16C550 (20x family)",
	    {	0x131f,	0x2050,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
	    },
	},

	/* SIIG Cyber 4S PCI 16C650 (20x family): 4S */
	{   "SIIG Cyber 4S PCI 16C650 (20x family)",
	    {	0x131f,	0x2051,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
	    },
	},

	/* SIIG Cyber 4S PCI 16C850 (20x family): 4S */
	{   "SIIG Cyber 4S PCI 16C850 (20x family)",
	    {	0x131f,	0x2052,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
	    },
	},

	/* SIIG PS8000 PCI 8S 16C550 (20x family): 8S - 16 Byte FIFOs */
	{   "SIIG PS8000 PCI 8S 16C550 (20x family)",
	    {	0x131f,	0x2080,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x18, COM_FREQ * 8 },
	    },
	},

	/* SIIG PS8000 PCI 8S 16C650 (20x family): 8S - 32 Byte FIFOs */
	{   "SIIG PS8000 PCI 8S 16C650 (20x family)",
	    {	0x131f,	0x2081,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x18, COM_FREQ * 8 },
	    },
	},

	/* SIIG PS8000 PCI 8S 16C850 (20x family): 8S - 128 Byte FIFOs */
	{   "SIIG PS8000 PCI 8S 16C850 (20x family)",
	    {	0x131f,	0x2082,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00,
		    (COM_FREQ * 8)|PUC_COM_SIIG20x|PUC_PORT_USR0 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x18, COM_FREQ * 8 },
	    },
	},
	/* VScom PCI-200: 2S */
	{   "VScom PCI-200",
	    {	0x10b5,	0x1103,	0x10b5,	0x1103	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ * 8 },
	    },
	},

	/* VScom PCI-400: 4S */
	{   "VScom PCI-400",
	    {	0x10b5,	0x1077,	0x10b5,	0x1077	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
	    },
	},

	/* VScom PCI-800: 8S */
	{   "VScom PCI-800",
	    {	0x10b5,	0x1076,	0x10b5,	0x1076	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x20, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x28, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x30, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x38, COM_FREQ * 8 },
	    },
	},

	{   "Titan PCI-010HV2",
	    {   0x14d2, 0xe001, 0,      0       },
	    {   0xffff, 0xffff, 0,      0       },
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},
	{   "Titan PCI-200HV2",
	    {   0x14d2, 0xe020, 0,      0       },
	    {   0xffff, 0xffff, 0,      0       },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
	    },
	},

	/*
	 * VScom PCI-800H. Uses 8 16950 UART, behind a PCI chips that offers
	 * 4 com port on PCI device 0 and 4 on PCI device 1. PCI device 0 has
	 * device ID 3 and PCI device 1 device ID 4.
	 */
	{   "Titan PCI-800H",
	    {	0x14d2,	0xa003,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x18, COM_FREQ * 8 },
	    },
	},
	{   "Titan PCI-800H",
	    {	0x14d2,	0xa004,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x18, COM_FREQ * 8 },
	    },
	},
        {   "Titan PCI-200H",
            {   0x14d2, 0xa005, 0,      0       },
            {   0xffff, 0xffff, 0,      0       },
            {
                { PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 8 },
                { PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 8 },
            },
        },
	{   "Titan PCI-800L",
	    {	0x14d2,	0x8080,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x18, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x20, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x20, 0x28, COM_FREQ * 8 },
	    },
	},
	/* NEC PK-UG-X001 K56flex PCI Modem card.
	   NEC MARTH bridge chip and Rockwell RCVDL56ACF/SP using. */
	{   "NEC PK-UG-X001 K56flex PCI Modem",
	    {	0x1033,	0x0074,	0x1033,	0x8014	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	    },
	},

	/* NEC PK-UG-X008 */
	{   "NEC PK-UG-X008",
	    {	0x1033,	0x007d,	0x1033,	0x8012	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ},
	    },
	},

	/* Lava Computers 2SP-PCI */
	{   "Lava Computers 2SP-PCI parallel port",
	    {	0x1407,	0x8000,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},

	/* Lava Computers 2SP-PCI and Quattro-PCI serial ports */
	{   "Lava Computers dual serial port",
	    {	0x1407,	0x0100,	0,	0	},
	    {	0xffff,	0xfffc,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	    },
	},

	/* Lava Computers DSerial PCI serial ports */
	{   "Lava Computers serial port",
	    {	0x1407,	0x0110,	0,	0	},
	    {	0xffff,	0xfffc,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	    },
	},

	/* Lava Computers Quattro-PCI serial ports */
	{   "Lava Quattro-PCI 4-port serial",
	    {   0x1407, 0x0120, 0,	0	},
	    {   0xffff, 0xfffc, 0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	    },
	},

	/* Lava Computers Octopus-550 serial ports */
	{   "Lava Computers Octopus-550 8-port serial",
	    {	0x1407,	0x0180,	0,	0	},
	    {	0xffff,	0xfffc,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
	    },
	},

	/* US Robotics (3Com) PCI Modems */
	{   "US Robotics (3Com) 3CP5609 PCI 16550 Modem",
	    {	0x12b9,	0x1008,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	    },
	},

	/* Actiontec  56K PCI Master */
	{   "Actiontec 56K PCI Master",
	    {	0x11c1,	0x0480,	0x0, 	0x0	},
	    {	0xffff,	0xffff,	0x0,	0x0	},
	    {
		{ PUC_PORT_TYPE_COM,	0x14,	0x00, COM_FREQ },
	    },
	},

	/*
	 * Boards with an Oxford Semiconductor chip.
	 *
	 * Oxford Semiconductor provides documentation for their chip at:
	 * <URL:http://www.plxtech.com/products/uart>
	 *
	 * As sold by Kouwell <URL:http://www.kouwell.com/>.
	 * I/O Flex PCI I/O Card Model-223 with 4 serial and 1 parallel ports.
	 */

	/* Oxford Semiconductor OXmPCI952 PCI UARTs */
	{   "Oxford Semiconductor OXmPCI952 UARTs",
	    {	0x1415,	0x950a,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ * 10 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ * 10 },
	    },
	},

	/* Oxford Semiconductor OX16PCI952 PCI `950 UARTs - 128 byte FIFOs */
	{   "Oxford Semiconductor OX16PCI952 UARTs",
	    {   0x1415, 0x9521, 0,	0	},
	    {   0xffff, 0xffff, 0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	    },
	},

	/* Oxford Semiconductor OX16PCI952 PCI Parallel port */
	{   "Oxford Semiconductor OX16PCI952 Parallel port",
	    {   0x1415, 0x9523, 0,	0	},
	    {   0xffff, 0xffff, 0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},

	/* Oxford Semiconductor OX16PCI954 PCI UARTs */
	{   "Oxford Semiconductor OX16PCI954 UARTs",
	    {	0x1415,	0x9501,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x10, 0x08, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x10, 0x10, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x10, 0x18, COM_FREQ },
	    },
	},

	/* Oxford Semiconductor OX16PCI954 PCI Parallel port */
	{   "Oxford Semiconductor OX16PCI954 Parallel port",
	    {	0x1415,	0x9513,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},

	/* Moxa Technologies Co., Ltd. PCI I/O Card 4S RS232/422/485 */
	{   "Moxa Technologies, SmartIO C104H/PCI",
	    {	0x1393,	0x1040,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
	    },
	},

	/* Moxa Technologies Co., Ltd. PCI I/O Card 4S RS232 */
	{   "Moxa Technologies, SmartIO CP104/PCI",
	    {	0x1393,	0x1041,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
	    },
	},

	/* Moxa Technologies Co., Ltd. PCI I/O Card 4S RS232 */
	{   "Moxa Technologies, SmartIO CP104-V2/PCI",
	    {	0x1393,	0x1042,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
	    },
	},

	/* Moxa Technologies Co., Ltd. PCI I/O Card 4S RS232/422/485 */
	{   "Moxa Technologies, SmartIO CP-114/PCI",
	    {	0x1393,	0x1141,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
	    },
	},

	/* Moxa Technologies Co., Ltd. PCI I/O Card 8S RS232 */
	{   "Moxa Technologies, SmartIO C168H/PCI",
	    {	0x1393,	0x1680,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x20, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x28, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x30, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x38, COM_FREQ * 8 },
	    },
	},

	/* NetMos 1P PCI : 1P */
	{   "NetMos NM9805 1284 Printer port",
	    {	0x9710,	0x9805,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
	    },
	},

	/* NetMos 2P PCI : 2P */
	{   "NetMos NM9815 Dual 1284 Printer port",
	    {	0x9710,	0x9815,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* NetMos 2S PCI NM9835 : 2S */
	{   "NetMos NM9835 Dual UART",
	    {	0x9710, 0x9835, 0x1000, 0x0002	},
	    {	0xffff, 0xffff, 0xffff, 0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	    },
	},

	/* NetMos 2S1P PCI 16C650 : 2S, 1P */
	{   "NetMos NM9835 Dual UART and 1284 Printer port",
	    {	0x9710,	0x9835,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_LPT, 0x18, 0x00, 0x00 },
	    },
	},

	/* NetMos 4S0P PCI NM9845 : 4S, 0P */
	{   "NetMos NM9845 Quad UART",
	   {   0x9710, 0x9845, 0x1000, 0x0004  },
	   {   0xffff, 0xffff, 0xffff, 0xffff  },
	   {
	       { PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
	   },
       },

	/* NetMos 4S1P PCI NM9845 : 4S, 1P */
	{   "NetMos NM9845 Quad UART and 1284 Printer port",
	   {   0x9710, 0x9845, 0x1000, 0x0014  },
	   {   0xffff, 0xffff, 0xffff, 0xffff  },
	   {
	       { PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	   },
       },

       /* NetMos 6S PCI 16C650 : 6S, 0P */
       {   "NetMos NM9845 6 UART",
	   {   0x9710, 0x9845, 0x1000, 0x0006  },
	   {   0xffff, 0xffff, 0xffff, 0xffff  },
	   {
	       { PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ },
	       { PUC_PORT_TYPE_COM, 0x24, 0x00, COM_FREQ },
	   },
       },

       /* NetMos 4S1P PCI NM9845 : 4S, 1P */
       {   "NetMos NM9845 Quad UART and 1284 Printer port (unknown type)",
	    {	0x9710,	0x9845,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_LPT, 0x20, 0x00, 0x00 },
	    },
	},

       /* NetMos 4S1P PCI NM9855 : 4S, 1P */
       {   "NetMos NM9855 Quad UART and 1284 Printer port (unknown type)",
	    {	0x9710,	0x9855,	0x1000,	0x0014	},
	    {	0xffff,	0xffff,	0xffff,	0xffff	},
	    {
		{ PUC_PORT_TYPE_LPT, 0x10, 0x00, 0x00 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x24, 0x00, COM_FREQ },
	    },
	},

	/*
	 * This is the Middle Digital, Inc. PCI-Weasel, which
	 * uses a PCI interface implemented in FPGA.
	 */
	{   "Middle Digital, Inc. Weasel serial port",
	    {	0xdeaf,	0x9051,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
	    },
	},

	/* Avlab Technology, Inc. Low Profile PCI 4 Serial: 4S */
	{   "Avlab Low Profile PCI 4 Serial",
	    {	0x14db,	0x2150,	0,	0	},
	    {	0xffff,	0xffff,	0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
	    },
	},

	/*
	 * Perle PCI-RAS 4 Modem ports
	 */
	{   "Perle Systems PCI-RAS 4 modem ports",
	    {	0x10b5, 0x9030, 0x155f, 0xf001	},
	    {	0xffff, 0xffff, 0xffff, 0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 4 },
	    },
	},

	/*
	 * Perle PCI-RASV92 4 Modem ports
	 */
	{   "Perle Systems PCI-RASV92 4 modem ports",
	    {	0x10b5, 0x9050, 0x155f, 0xf001	},
	    {	0xffff, 0xffff, 0xffff, 0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 4 },
	    },
	},

	/*
	 * Perle PCI-RAS 8 Modem ports
	 */
	{   "Perle Systems PCI-RAS 8 modem ports",
	    {	0x10b5, 0x9030, 0x155f, 0xf010	},
	    {	0xffff, 0xffff, 0xffff, 0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x20, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x28, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x30, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x38, COM_FREQ * 4 },
	    },
	},

	/*
	 * Perle PCI-RASV92 8 Modem ports
	 */
	{   "Perle Systems PCI-RASV92 8 modem ports",
	    {	0x10b5, 0x9050, 0x155f, 0xf010	},
	    {	0xffff, 0xffff, 0xffff, 0xffff	},
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x20, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x28, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x30, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x38, COM_FREQ * 4 },
	    },
	},

	/*
	 * Boca Research Turbo Serial 654 (4 serial port) card.
	 * Appears to be the same as Chase Research PLC PCI-FAST4
	 * and Perle PCI-FAST4 Multi-Port serial cards.
	 */
	{   "Boca Research Turbo Serial 654",
	    {   0x10b5, 0x9050, 0x12e0, 0x0031  },
	    {   0xffff, 0xffff, 0xffff, 0xffff  },
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 4 },
	    },
	},

	/*
	 * Boca Research Turbo Serial 658 (8 serial port) card.
	 * Appears to be the same as Chase Research PLC PCI-FAST8
	 * and Perle PCI-FAST8 Multi-Port serial cards.
	 */
	{   "Boca Research Turbo Serial 658",
	    {   0x10b5, 0x9050, 0x12e0, 0x0021  },
	    {   0xffff, 0xffff, 0xffff, 0xffff  },
	    {
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x10, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x18, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x20, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x28, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x30, COM_FREQ * 4 },
		{ PUC_PORT_TYPE_COM, 0x18, 0x38, COM_FREQ * 4 },
	    },
	},

	/*
	 * Addi-Data APCI-7800 8-port serial card.
	 * Uses an AMCC chip as PCI bridge.
	 */
	{   "Addi-Data APCI-7800",
	    {   0x10e8, 0x818e, 0, 0  },
	    {   0xffff, 0xffff, 0, 0  },
	    {
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x14, 0x08, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x18, 0x08, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x1c, 0x08, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x20, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x20, 0x08, COM_FREQ },
	    },
	},

	{   "EXAR XR17D152",
	    {   0x13a8, 0x0152, 0,      0       },
	    {   0xffff, 0xffff, 0,      0       },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
	    },
	},
	{   "EXAR XR17D154",
	    {   0x13a8, 0x0154, 0,      0       },
	    {   0xffff, 0xffff, 0,      0       },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0400, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0600, COM_FREQ * 8 },
	    },
	},
	{   "EXAR XR17D158",
	    {   0x13a8, 0x0158, 0,      0       },
	    {   0xffff, 0xffff, 0,      0       },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0400, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0600, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0800, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0a00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0c00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0e00, COM_FREQ * 8 },
	    },
	},

	/* I-O DATA RSA-PCI: 2S */
	{   "I-O DATA RSA-PCI 2-port serial",
	    {	0x10fc, 0x0007, 0, 0 },
	    {	0xffff, 0xffff, 0, 0 },
	    {
		{ PUC_PORT_TYPE_COM, 0x14, 0x00, COM_FREQ },
		{ PUC_PORT_TYPE_COM, 0x18, 0x00, COM_FREQ },
	    },
	},
 
	/* Digi International Digi Neo 4 Serial */
	{ "Digi International Digi Neo 4 Serial",
	    {	PCI_VENDOR_DIGI, PCI_PRODUCT_DIGI_NEO4,		0, 0  },
	    {	0xffff, 0xffff,					0, 0  },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0400, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0600, COM_FREQ * 8 },
	    },
	},

	/* Digi International Digi Neo 8 Serial */
	{ "Digi International Digi Neo 8 Serial",
	    {	PCI_VENDOR_DIGI, PCI_PRODUCT_DIGI_NEO8,		0, 0  },
	    {	0xffff, 0xffff,					0, 0  },
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0400, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0600, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0800, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0a00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0c00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0e00, COM_FREQ * 8 },
	    },
	},

	/*
	 * B&B Electronics MIPort Serial cards.
	 */
	{ "BBELEC ISOLATED_2_PORT",
	    {	PCI_VENDOR_BBELEC, PCI_PRODUCT_BBELEC_ISOLATED_2_PORT, 0, 0 },
	    {	0xffff, 0xffff, 0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
	    },
	},
	{ "BBELEC ISOLATED_4_PORT",
	    {	PCI_VENDOR_BBELEC, PCI_PRODUCT_BBELEC_ISOLATED_4_PORT, 0, 0 },
	    {	0xffff, 0xffff, 0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0400, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0600, COM_FREQ * 8 },
	    },
	},
	{ "BBELEC ISOLATED_8_PORT",
	    {	PCI_VENDOR_BBELEC, PCI_PRODUCT_BBELEC_ISOLATED_8_PORT, 0, 0 },
	    {	0xffff, 0xffff, 0,	0	},
	    {
		{ PUC_PORT_TYPE_COM, 0x10, 0x0000, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0200, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0400, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0600, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0800, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0a00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0c00, COM_FREQ * 8 },
		{ PUC_PORT_TYPE_COM, 0x10, 0x0e00, COM_FREQ * 8 },
	    },
	},

	{ .name = NULL },
};
