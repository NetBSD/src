/*	$NetBSD: pci_tseng.c,v 1.4 2001/05/21 14:30:41 leo Exp $	*/

/*
 * Copyright (c) 1999 Leo Weppelman.  All rights reserved.
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
 *	This product includes software developed by Leo Weppelman.
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
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <atari/pci/pci_vga.h>
#include <atari/dev/grf_etreg.h>

#define PCI_LINMEMBASE	0x0e000000
#define PCI_IOBASE	0x800

static void et6000_init(volatile u_char *, u_char *, int);

/*
 * Use tables for the card init...
 */
static u_char seq_tab[] = {
 	0x03, 0x01, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00 };

static u_char gfx_tab[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x0f, 0xff };

static u_char attr_tab[] = {
	0x0a, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00 };

static u_char crt_tab[] = {
	0x60, 0x53, 0x4f, 0x94, 0x56, 0x05, 0xc1, 0x1f,
	0x00, 0x4f, 0x00, 0x0f, 0x00, 0x00, 0x07, 0x80,
	0x98, 0x3d, 0x8f, 0x28, 0x0f, 0x8f, 0xc2, 0xa3,
	0xff, 0x3d, 0x8f, 0x28, 0x0f, 0x8f, 0xc2, 0xa3,
	0x60, 0x08, 0x00, 0x00, 0x56, 0x05, 0xc1, 0x1f,
	0x00, 0x4f, 0x00, 0x0f, 0x00, 0x00, 0x07, 0x80,
	0x00, 0x80, 0x28, 0x00, 0x00, 0x10, 0x43, 0x09,
	0x05, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00 };

static u_char ras_cas_tab[] = {
	0x11, 0x14, 0x15 };

void
tseng_init(pc, tag, id, ba, fb)
	pci_chipset_tag_t	pc;
	pcitag_t		tag;
	int			id;
	volatile u_char		*ba;
	u_char			*fb;
{
	int			i, j, csr;
	int			is_et6000 = 0;

	is_et6000 = (id ==  PCI_PRODUCT_TSENG_ET6000) ? 1 : 0;

	/* Turn on the card */
	pci_conf_write(pc, tag, PCI_MAPREG_START, PCI_LINMEMBASE);
	if (is_et6000)
		pci_conf_write(pc, tag, PCI_MAPREG_START+4,
					PCI_IOBASE | PCI_MAPREG_TYPE_IO);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= (PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_IO_ENABLE);
	csr |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	if (is_et6000) {
		/*
		 * The et6[01]000 cards have MDRAM chips. The
		 * timeing to those chips is not properly initialized
		 * by the card on init. The way to determine the
		 * values is not documented either :-( So that's why
		 * all this mess below (and in et6000_init()....
		 */
		for (i = 0; i < sizeof(ras_cas_tab); i++) {
			et6000_init(ba, fb, i);
			for (j = 0; j < 32; j++)
				fb[j] = j;
			for (j = 0; j < 32; j++)
				if (fb[j] != j)
					break;
			if (j == 32)
				break;
		}
	}

	vgaw(ba, GREG_MISC_OUTPUT_W,      0x63);
	vgaw(ba, GREG_VIDEOSYSENABLE,     0x01);
	WCrt(ba, 0x17               ,     0x00); /* color */
	WCrt(ba, 0x11               ,     0x00); /* color */
	vgaw(ba, VDAC_MASK          ,     0xff);
	WSeq(ba, SEQ_ID_RESET       ,     0x00);
	vgaw(ba, GREG_HERCULESCOMPAT,     0x03);
	vgaw(ba, GREG_DISPMODECONTROL,    0xa0);

	/* Load sequencer */
	for (i = 1; i < 8; i++)
		WSeq(ba, i, seq_tab[i]);
	WSeq(ba, SEQ_ID_RESET       ,     0x03);
	
	vgar(ba, VDAC_ADDRESS);	/* clear old state */
        vgar(ba, VDAC_MASK);
        vgar(ba, VDAC_MASK);
        vgar(ba, VDAC_MASK);
        vgar(ba, VDAC_MASK);
	vgaw(ba, VDAC_MASK, 0);		/* set to palette */
	vgar(ba, VDAC_ADDRESS);		/* clear state */
	vgaw(ba, VDAC_MASK, 0xff);

	/*
	 * Make sure we're allowed to write all crt-registers
	 */
	WCrt(ba, CRT_ID_END_VER_RETR, (RCrt(ba, CRT_ID_END_VER_RETR) & 0x7f));

	/* CRT registers */
	for (i = 0; i < 0x3e; i++)
		WCrt(ba, i, crt_tab[i]);

	/* GCT registers */
	for (i = 0; i < 0x09; i++)
		WGfx(ba, i, gfx_tab[i]);

	for (i = 0; i < 0x10; i++)
		WAttr(ba, i, i);
	for (; i < 0x18; i++)
		WAttr(ba, i, attr_tab[i - 0x10]);
	WAttr(ba, 0x20, 0);
}

/*
 * Initialize the et6000 specific (PCI) registers. Try to do it like the
 * video-bios would have done it, so things like Xservers get what they
 * expect. Most info was kindly provided by Koen Gadeyne.
 */

static void
et6000_init(ba, fb, iter)
volatile u_char *ba;
u_char		*fb;
int		iter;
{

	int		i;
	u_char		dac_tab[] = { 0x7d,0x67, 0x5d,0x64, 0x56,0x63,
				      0x28,0x22, 0x79,0x49, 0x6f,0x47,
				      0x28,0x41, 0x6b,0x44, 0x00,0x00,
				      0x00,0x00, 0x5d,0x25, 0x00,0x00,
				      0x00,0x00, 0x00,0x96 };

	ba += 0x800;


	ba[0x40] = 0x06;	/* Use standard vga addressing		*/
	ba[0x41] = 0x2a;	/* Performance control			*/
	ba[0x43] = 0x02;	/* XCLK/SCLK config			*/
	ba[0x44] = ras_cas_tab[iter];	/* RAS/CAS config		*/
	ba[0x46] = 0x00;	/* CRT display feature			*/
	ba[0x47] = 0x10;
	ba[0x58] = 0x00;	/* Video Control 1			*/
	ba[0x59] = 0x04;	/* Video Control 2			*/
	
	/*
	 * Setup a 'standard' CLKDAC
	 */
	ba[0x42] = 0x00;	/* MCLK == CLK0 */
	ba[0x67] = 0x00;	/* Start filling from dac-reg 0 and up... */
	for (i = 0; i < 0x16; i++)
		ba[0x69] = dac_tab[i];

	if (ba[8] == 0x70) { /* et6100, right? */
		volatile u_char *ma = (volatile u_char *)fb;
		u_char		bv;

		/*
		 * XXX Black magic to get the bloody MDRAM's to function...
                 * XXX _Only_ tested on my card! [leo]
		 */
		bv = ba[45];
		ba[0x45] = bv | 0x40;	/* Reset MDRAM's		*/
		ba[0x45] = bv | 0x70;	/* Program latency value	*/
		ma[0x0] = 0;		/* Yeah, right :-(		*/
		ba[0x45] = bv;		/* Back to normal		*/
	}
}
