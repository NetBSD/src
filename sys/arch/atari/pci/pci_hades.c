/*	$NetBSD: pci_hades.c,v 1.13.6.1 2017/12/03 11:35:58 jdolecek Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: pci_hades.c,v 1.13.6.1 2017/12/03 11:35:58 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <sys/bswap.h>

#include <atari/atari/device.h>
#include <atari/pci/pci_vga.h>
#include <atari/dev/grf_etreg.h>

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return (4);
}

static int pci_config_offset(pcitag_t);

/*
 * Atari_init.c maps the config areas PAGE_SIZE bytes apart....
 */
static int pci_config_offset(pcitag_t tag)
{
	int	device;

	device = (tag >> 11) & 0x1f;
	return(device * PAGE_SIZE);
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	u_long	data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return ((pcireg_t) -1);

	data = *(u_long *)(pci_conf_addr + pci_config_offset(tag) + reg);
	return (bswap32(data));
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	*((u_long *)(pci_conf_addr + pci_config_offset(tag) + reg))
		= bswap32(data);
}

/*
 * The interrupt stuff is rather ugly. On the Hades, all interrupt lines
 * for a slot are wired together and connected to IO 0,1,2 or 5 (slots:
 * (0-3) on the TT-MFP. The Pci-config code initializes the irq. number
 * to the slot position.
 */
static pci_intr_info_t iinfo[4] = { { -1 }, { -1 }, { -1 }, { -1 } };

static int	iifun(int, int);

static int
iifun(int slot, int sr)
{
	pci_intr_info_t *iinfo_p;
	int		s;

	iinfo_p = &iinfo[slot];

	/*
	 * Disable the interrupts
	 */
	MFP2->mf_imrb  &= ~iinfo_p->imask;

	if ((sr & PSL_IPL) >= (iinfo_p->ipl & PSL_IPL)) {
		/*
		 * We're running at a too high priority now.
		 */
		add_sicallback((si_farg)iifun, (void*)slot, 0);
	}
	else {
		s = splx(iinfo_p->ipl);
		(void) (iinfo_p->ifunc)(iinfo_p->iarg);
		splx(s);

		/*
		 * Re-enable interrupts after handling
		 */
		MFP2->mf_imrb |= iinfo_p->imask;
	}
	return 1;
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
	pci_intr_info_t *iinfo_p;
	struct intrhand	*ihand;
	int		slot;

	slot    = ih;
	iinfo_p = &iinfo[slot];

	if (iinfo_p->ipl > 0)
	    panic("pci_intr_establish: interrupt was already established");

	ihand = intr_establish((slot == 3) ? 23 : 16 + slot, USER_VEC, 0,
				(hw_ifun_t)iifun, (void *)slot);
	if (ihand != NULL) {
		iinfo_p->ipl   = level;
		iinfo_p->imask = (slot == 3) ? 0x80 : (0x01 << slot);
		iinfo_p->ifunc = ih_fun;
		iinfo_p->iarg  = ih_arg;
		iinfo_p->ihand = ihand;

		/*
		 * Enable (unmask) the interrupt
		 */
		MFP2->mf_imrb |= iinfo_p->imask;
		MFP2->mf_ierb |= iinfo_p->imask;
		return(iinfo_p);
	}
	return NULL;
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	pci_intr_info_t *iinfo_p = (pci_intr_info_t *)cookie;

	if (iinfo->ipl < 0)
	    panic("pci_intr_disestablish: interrupt was not established");

	MFP2->mf_imrb &= ~iinfo->imask;
	MFP2->mf_ierb &= ~iinfo->imask;
	(void) intr_disestablish(iinfo_p->ihand);
	iinfo_p->ipl = -1;
}

/*
 * XXX: Why are we repeating this everywhere! (Leo)
 */
#define PCI_LINMEMBASE  0x0e000000

static u_char crt_tab[] = {
	0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
	0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x8e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
	0xff };

static u_char seq_tab[] = {
	0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00 };

static u_char attr_tab[] = {
	0x0c, 0x00, 0x0f, 0x08, 0x00, 0x00, 0x00, 0x00 };

static u_char gdc_tab[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00, 0xff };

void
ati_vga_init(pci_chipset_tag_t pc, pcitag_t tag, int id, volatile u_char *ba, u_char *fb)
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
	for (i = 0; i < 8; i++)
		WSeq(ba, i, seq_tab[i]);
	for (i = 0; i < 9; i++)
		WGfx(ba, i, gdc_tab[i]);
	for (i = 0x10; i < 0x18; i++)
		WAttr(ba, i, attr_tab[i - 0x10]);
	WAttr(ba, 0x20, 0);
}
