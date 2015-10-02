/*	$NetBSD: pci_milan.c,v 1.14 2015/10/02 05:22:50 msaitoh Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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
__KERNEL_RCSID(0, "$NetBSD: pci_milan.c,v 1.14 2015/10/02 05:22:50 msaitoh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/isa/isavar.h>		/* isa_intr_{dis}establish */
#include <dev/isa/isareg.h>		/* isa_intr_{dis}establish */

#include <sys/bswap.h>
#include <machine/isa_machdep.h>	/* isa_intr_{dis}establish */

#include <atari/pci/pci_vga.h>
#include <atari/dev/grf_etreg.h>

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return (6);
}

/*
 * These are defined in locore.s:
 */
pcireg_t	milan_pci_confread(pcitag_t);
void		milan_pci_confwrite(u_long, pcireg_t);
extern u_long	plx_status;

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	u_long		data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return 0xffffffff;

	data = bswap32(milan_pci_confread(tag | reg));
	if ((plx_status) & 0xf9000000) {
		/*
		 * Access error, assume nothing there...
		 */
		data = 0xffffffff;
	}
	return(data);
}


void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	milan_pci_confwrite(tag | reg, bswap32(data));
}

int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ih,
		 int attr, uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		return 0;
	default:
		return ENODEV;
	}
}

void *
pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level, int (*ih_fun)(void *), void *ih_arg)
{
	if (ih == 0 || ih >= 16 || ih == 2)
		panic("pci_intr_establish: bogus handle 0x%x", ih);
	return isa_intr_establish(NULL, ih, IST_LEVEL, level, ih_fun, ih_arg);
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	isa_intr_disestablish(NULL, cookie);
}

/*
 * VGA related stuff...
 * XXX: Currently, you can only boot the Milan through loadbsd.ttp, hence the
 *      text mode ;-)
 * It looks like the Milan BIOS is initializing the VGA card in a reasonably
 * standard text mode. However, the screen mode is 640*480 instead of 640*400.
 * Since wscons does not handle the right by default, the card is reprogrammed
 * to 640*400 using only 'standard' VGA registers (I hope!). So this ought to
 * work on cards other than the S3Trio card I have tested it on.
 */
static u_char crt_tab[] = {
	0x60, 0x53, 0x4f, 0x14, 0x56, 0x05, 0xc1, 0x1f,
	0x00, 0x4f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
	0x98, 0x3d, 0x8f, 0x28, 0x0f, 0x8f, 0xc1, 0xc3,
	0xff };

/*
 * XXX: Why are we repeating this everywhere! (Leo)
 */
#define PCI_LINMEMBASE  0x0e000000

void
milan_vga_init(pci_chipset_tag_t pc, pcitag_t tag, int id, volatile u_char *ba, u_char *fb)
{
	int			i, csr;

	/* Turn on the card */
	pci_conf_write(pc, tag, PCI_MAPREG_START, PCI_LINMEMBASE);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= (PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_IO_ENABLE);
	csr |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	/*
	 * Make sure we're allowed to write all crt-registers and reload them.
	 */
	WCrt(ba, CRT_ID_END_VER_RETR, (RCrt(ba, CRT_ID_END_VER_RETR) & 0x7f));

	for (i = 0; i < 0x18; i++)
		WCrt(ba, i, crt_tab[i]);

	/*
	 * The Milan has a white border... make it black
	 */
	WAttr(ba, 0x11, 0|0x20);
}
