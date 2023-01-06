/*	$NetBSD: pci_vga.c,v 1.20 2023/01/06 10:28:28 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pci_vga.c,v 1.20 2023/01/06 10:28:28 tsutsui Exp $");

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

#include "vga_pci.h"
#if NVGA_PCI > 0
#include <dev/cons.h>
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

static void loadfont(volatile uint8_t *, uint8_t *fb);

/* XXX: Shouldn't these be in font.h???? */
extern font_info	font_info_8x8;
extern font_info	font_info_8x16;

/* Console colors */
/* attribute controller registers */
static const uint8_t vga_atc[] = {
	0x00,	/* 00: internal palette  0 */
	0x01,	/* 01: internal palette  1 */
	0x02,	/* 02: internal palette  2 */
	0x03,	/* 03: internal palette  3 */
	0x04,	/* 04: internal palette  4 */
	0x05,	/* 05: internal palette  5 */
	0x14,	/* 06: internal palette  6 */
	0x07,	/* 07: internal palette  7 */
	0x38,	/* 08: internal palette  8 */
	0x39,	/* 09: internal palette  9 */
	0x3a,	/* 0A: internal palette 10 */
	0x3b,	/* 0B: internal palette 11 */
	0x3c,	/* 0C: internal palette 12 */
	0x3d,	/* 0D: internal palette 13 */
	0x3e,	/* 0E: internal palette 14 */
	0x3f,	/* 0F: internal palette 15 */
	0x0c,	/* 10: attribute mode control */
	0x00,	/* 11: overscan color */
	0x0f,	/* 12: color plane enable */
	0x08,	/* 13: horizontal PEL panning */
	0x00	/* 14: color select */
};

/* video DAC palette registers */
/* XXX only set up 16 colors used by internal palette in ATC regsters */
static const uint8_t vga_dacpal[] = {
	/* R     G     B */
	0x00, 0x00, 0x00,	/* BLACK        */
	0x00, 0x00, 0x2a,	/* BLUE	        */
	0x00, 0x2a, 0x00,	/* GREEN        */
	0x00, 0x2a, 0x2a,	/* CYAN         */
	0x2a, 0x00, 0x00,	/* RED          */
	0x2a, 0x00, 0x2a,	/* MAGENTA      */
	0x2a, 0x15, 0x00,	/* BROWN        */
	0x2a, 0x2a, 0x2a,	/* LIGHTGREY    */
	0x15, 0x15, 0x15,	/* DARKGREY     */
	0x15, 0x15, 0x3f,	/* LIGHTBLUE    */
	0x15, 0x3f, 0x15,	/* LIGHTGREEN   */
	0x15, 0x3f, 0x3f,	/* LIGHTCYAN    */
	0x3f, 0x15, 0x15,	/* LIGHTRED     */
	0x3f, 0x15, 0x3f,	/* LIGHTMAGENTA */
	0x3f, 0x3f, 0x15,	/* YELLOW       */
	0x3f, 0x3f, 0x3f	/* WHITE        */
};

static bus_space_tag_t	vga_iot, vga_memt;
static int		tags_valid = 0;

#define		VGA_REG_SIZE	(8*1024)
#define		VGA_FB_SIZE	(32*1024)

/*
 * Go look for a VGA card on the PCI-bus. This search is a
 * stripped down version of the PCI-probe. It only looks on
 * bus0 for VGA cards. The first card found is used.
 */
int
check_for_vga(bus_space_tag_t iot, bus_space_tag_t memt)
{
	pci_chipset_tag_t	pc = NULL; /* XXX */
	bus_space_handle_t	ioh_regs, memh_fb;
	pcitag_t		tag;
	int			device, found, maxndevs, i;
	int			got_ioh, got_memh, rv;
	uint32_t		id, class;
	volatile uint8_t	*regs;
	uint8_t			*fb;
	const char		*nbd = "NetBSD/Atari";

	found    = 0;
	tag      = 0;
	id       = 0;
	rv	 = 0;
	got_ioh	 = 0;
	got_memh = 0;
	maxndevs = pci_bus_maxdevs(pc, 0);

	/*
	 * Map 8Kb of registers and 32Kb frame buffer.
	 * XXX: The way the registers are mapped here is plain wrong.
	 *      We should try to pin-point the region down to 3[bcd]0 (see
	 *      .../dev/ic/vga.c).
	 */
	if (bus_space_map(iot, 0, VGA_REG_SIZE, 0, &ioh_regs))
		return 0;
	got_ioh = 1;

	if (bus_space_map(memt, 0xa0000, VGA_FB_SIZE, 0, &memh_fb))
		goto bad;
	got_memh = 1;
	regs = bus_space_vaddr(iot, ioh_regs);
	fb   = bus_space_vaddr(memt, memh_fb);

	for (device = 0; !found && (device < maxndevs); device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;

		/*
		 * Check if we have some display device here...
		 */
		class = pci_conf_read(pc, tag, PCI_CLASS_REG);
		i = 0;
		if (PCI_CLASS(class) == PCI_CLASS_PREHISTORIC &&
		    PCI_SUBCLASS(class) == PCI_SUBCLASS_PREHISTORIC_VGA)
			i = 1;
		if (PCI_CLASS(class) == PCI_CLASS_DISPLAY &&
		    PCI_SUBCLASS(class) == PCI_SUBCLASS_DISPLAY_VGA)
			i = 1;
		if (i == 0)
			continue;

#if _MILANHW_
		/* Don't need to be more specific */
		milan_vga_init(pc, tag, id, regs, fb);
		found = 1;
#else
		switch (id = PCI_PRODUCT(id)) {

			/*
			 * XXX Make the inclusion of the cases dependent
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
			case PCI_PRODUCT_ATI_RAGE_PRO_PCI_P:
				ati_vga_init(pc, tag, id, regs, fb);
				found = 1;
				break;
			default:
				break;
		}
#endif /* _MILANHW_ */
	}
	if (!found)
		goto bad;

	/*
	 * Assume the device is in CGA mode. Wscons expects this too...
	 */
	bus_space_unmap(memt, memh_fb, VGA_FB_SIZE);
	if (bus_space_map(memt, 0xb8000, VGA_FB_SIZE, 0, &memh_fb)) {
		got_memh = 0;
		goto bad;
	}
	fb = bus_space_vaddr(memt, memh_fb);

	/*
	 * Generic parts of the initialization...
	 */

	/* set ATC registers */
	for (i = 0; i < 21; i++)
		WAttr(regs, i, vga_atc[i]);

	/* set DAC palette */
	for (i = 0; i < 16; i++) {
		vgaw(regs, VDAC_ADDRESS_W, vga_atc[i]);
		vgaw(regs, VDAC_DATA, vga_dacpal[i * 3 + 0]);
		vgaw(regs, VDAC_DATA, vga_dacpal[i * 3 + 1]);
		vgaw(regs, VDAC_DATA, vga_dacpal[i * 3 + 2]);
	}

	loadfont(regs, fb);
#if NVGA_PCI > 0
	/* use explicit WSDISPLAY_FONTENC_IBM font that MI vga(4) assumes */
	vga_no_builtinfont = 1;
#endif

	/*
	 * Clear the screen and print a message. The latter
	 * is of diagnostic/debug use only.
	 */
	for (i = 50 * 80; i >= 0; i -= 2) {
		fb[i] = 0x20; fb[i+1] = 0x07;
	}
	for (i = 56; *nbd; i += 2)
		fb[i] = *nbd++;

	rv = 1;
	vga_iot  = iot;
	vga_memt = memt;
	rv = tags_valid = 1;

 bad:
	if (got_memh)
		bus_space_unmap(memt, memh_fb, VGA_FB_SIZE);
	if (got_ioh)
		bus_space_unmap(iot, ioh_regs, VGA_REG_SIZE);
	return rv;
}

#if NVGA_PCI > 0
void vgacnprobe(struct consdev *);
void vgacninit(struct consdev *);

void
vgacnprobe(struct consdev *cp)
{

	if (tags_valid)
		cp->cn_pri = CN_NORMAL;
}

void
vgacninit(struct consdev *cp)
{

	if (tags_valid) {
		/* XXX: Are those arguments correct? Leo */
		vga_cnattach(vga_iot, vga_memt, 8, 0);
	}
}
#endif /* NVGA_PCI */

/*
 * Generic VGA. Load the configured kernel font into the videomemory and
 * place the card into textmode.
 */
static void
loadfont(volatile uint8_t *ba, uint8_t *fb)
	/* ba:	 Register area KVA */
	/* fb:	 Frame buffer KVA  */
{
	font_info	*fd;
	uint8_t		*c, *f, tmp;
	uint16_t	z, y;

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

	c = (uint8_t *)(fb) + (32 * fd->font_lo);
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
