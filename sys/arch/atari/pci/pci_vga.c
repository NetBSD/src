/*	$NetBSD: pci_vga.c,v 1.2 2001/05/21 14:30:42 leo Exp $	*/

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
#include <atari/include/iomap.h>

#include <atari/dev/font.h>

static void loadfont(volatile u_char *, u_char *fb);

/* XXX: Shouldn't these be in font.h???? */
extern font_info	font_info_8x8;
extern font_info	font_info_8x16;

/* Console colors */
static u_char conscolors[3][3] = {	/* background, foreground, hilite */
	{0x0, 0x0, 0x0}, {0x30, 0x30, 0x30}, { 0x3f,  0x3f,  0x3f}
};

/*
 * Go look for a VGA card on the PCI-bus. This search is a
 * stripped down version of the PCI-probe. It only looks on
 * bus0 for VGA cards. The first card found is used.
 */
int
check_for_vga()
{
	pci_chipset_tag_t	pc = NULL; /* XXX */
	pcitag_t		tag;
	int			device, found, id, maxndevs, i, j;
	volatile u_char		*regs;
	u_char			*fb;
	char			*nbd = "NetBSD/Atari";

	found    = 0;
	tag      = 0;
	id       = 0;
	maxndevs = pci_bus_maxdevs(pc, 0);

	/*
	 * These are setup in atari_init.c
	 */
	regs = (volatile caddr_t)pci_io_addr;
	fb   = (caddr_t)pci_mem_addr;

	for (device = 0; !found && (device < maxndevs); device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;
		switch (id = PCI_PRODUCT(id)) {

			/*
			 * XXX Make the inclusion of the cases dependend
			 *     on config options!
			 */
			case PCI_PRODUCT_TSENG_ET6000:
			case PCI_PRODUCT_TSENG_ET4000_W32P_A:
			case PCI_PRODUCT_TSENG_ET4000_W32P_B:
			case PCI_PRODUCT_TSENG_ET4000_W32P_C:
			case PCI_PRODUCT_TSENG_ET4000_W32P_D:
				tseng_init(pc, tag, id, regs, fb);
				found = 1;
				break;
			default:
				break;
		}
	}
	if (!found)
		return (0);

	/*
	 * Assume the device is in CGA mode. Wscons expects this too...
	 */
	fb = fb + 0x18000;

	/*
	 * Generic parts of the initialization...
	 */
	
	/* B&W colors */
	vgaw(regs, VDAC_ADDRESS_W, 0);
	for (i = 0; i < 256; i++) {
		j = (i & 1) ? ((i > 7) ? 2 : 1) : 0;
		vgaw(regs, VDAC_DATA, conscolors[j][0]);
		vgaw(regs, VDAC_DATA, conscolors[j][1]);
		vgaw(regs, VDAC_DATA, conscolors[j][2]);
	}

	loadfont(regs, fb);

	/*
	 * Clear the screen and print a message. The latter
	 * is of diagnostic/debug use only.
	 */
	for (i = 50 * 80; i >= 0; i -= 2) {
		fb[i] = 0x20; fb[i+1] = 0x07;
	}
	for (i = 56; *nbd; i += 2)
		fb[i] = *nbd++;
	
	return (1);
}

/*
 * Generic VGA. Load the configured kernel font into the videomemory and
 * place the card into textmode.
 */
static void
loadfont(ba, fb)
	volatile u_char *ba;	/* Register area KVA */
	u_char		*fb;	/* Frame buffer	KVA  */
{
	font_info	*fd;
	u_char		*c, *f, tmp;
	u_short		z, y;

#if defined(KFONT_8X8)
	fd = &font_info_8x8;
#else
	fd = &font_info_8x16;
#endif

	WAttr(ba, 0x20 | ACT_ID_ATTR_MODE_CNTL, 0x0a);
	WSeq(ba, SEQ_ID_MAP_MASK,	 0x04);
	WSeq(ba, SEQ_ID_MEMORY_MODE,	 0x06);
	WGfx(ba, GCT_ID_READ_MAP_SELECT, 0x02);
	WGfx(ba, GCT_ID_GRAPHICS_MODE,	 0x00);
	WGfx(ba, GCT_ID_MISC,		 0x0c);
	
	/*
	 * load text font into beginning of display memory. Each
	 * character cell is 32 bytes long (enough for 4 planes)
	 */
	for (z = 0, c = fb; z < 256 * 32; z++)
		*c++ = 0;

	c = (unsigned char *) (fb) + (32 * fd->font_lo);
	f = fd->font_p;
	z = fd->font_lo;
	for (; z <= fd->font_hi; z++, c += (32 - fd->height))
		for (y = 0; y < fd->height; y++) {
			*c++ = *f++;
		}

	/*
	 * Odd/Even addressing
	 */
	WSeq(ba, SEQ_ID_MAP_MASK,	 0x03);
	WSeq(ba, SEQ_ID_MEMORY_MODE,	 0x03);
	WGfx(ba, GCT_ID_READ_MAP_SELECT, 0x00);
	WGfx(ba, GCT_ID_GRAPHICS_MODE,	 0x10);
	WGfx(ba, GCT_ID_MISC,		 0x0e);

	/*
	 * Font height + underline location
	 */
	tmp = RCrt(ba, CRT_ID_MAX_ROW_ADDRESS) & 0xe0;
	WCrt(ba, CRT_ID_MAX_ROW_ADDRESS, tmp | (fd->height - 1));
	tmp = RCrt(ba, CRT_ID_UNDERLINE_LOC)   & 0xe0;
	WCrt(ba, CRT_ID_UNDERLINE_LOC,   tmp | (fd->height - 1));

	/*
	 * Cursor setup
	 */
	WCrt(ba, CRT_ID_CURSOR_START   , 0x00);
	WCrt(ba, CRT_ID_CURSOR_END     , fd->height - 1);
	WCrt(ba, CRT_ID_CURSOR_LOC_HIGH, 0x00);
	WCrt(ba, CRT_ID_CURSOR_LOC_LOW , 0x00);

	/*
	 * Enter text mode
	 */
	WCrt(ba, CRT_ID_MODE_CONTROL   , 0xa3);
	WAttr(ba, ACT_ID_ATTR_MODE_CNTL | 0x20, 0x0a);
}
