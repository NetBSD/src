/*	$NetBSD: grfabs_et.c,v 1.27.44.2 2009/08/19 18:46:02 yamt Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
 * All rights reserved.
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
 *      This product includes software developed by Leo Weppelman.
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
 * Most of the lower-level et4000 stuff was derived from:
 *	.../amiga/dev/grf_et.c
 *
 * Which was copyrighted by:
 *	Copyright (c) 1996 Tobias Abt
 *	Copyright (c) 1995 Ezra Story
 *	Copyright (c) 1995 Kari Mettinen
 *	Copyright (c) 1994 Markus Wild
 *	Copyright (c) 1994 Lutz Vieweg
 *
 * Thanks guys!
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grfabs_et.c,v 1.27.44.2 2009/08/19 18:46:02 yamt Exp $");

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

/*
 * For PCI probing...
 */
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/iomap.h>
#include <machine/video.h>
#include <machine/mfp.h>
#include <machine/cpu.h>
#include <atari/atari/device.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grfabs_et.h>
#include <atari/dev/grf_etreg.h>

#define	SAVEBUF_SIZE	(32*1024 + sizeof(save_area_t))

/*
 * Allow a 16Kb io-region and a 4MB frame buffer to be mapped. This
 * is more or less required by the XFree server.
 */
#define	REG_MAPPABLE	(16 * 1024)
#define	FRAME_MAPPABLE	(4 * 1024 * 1024)
#define VGA_MAPPABLE	(128 * 1024)
#define VGA_BASE	0xa0000

/*
 * Linear memory base, near the end of the pci area
 */
#define PCI_LINMEMBASE  0x0e000000

/*
 * Function decls
 */
static void       init_view(view_t *, bmap_t *, dmode_t *, box_t *);
static colormap_t *alloc_colormap(dmode_t *);
static void	  et_display_view(view_t *);
static view_t	  *et_alloc_view(dmode_t *, dimen_t *, u_char);
static void	  et_free_view(view_t *);
static void	  et_loadmode(struct grfvideo_mode *, et_sv_reg_t *);
static void	  et_remove_view(view_t *);
static void	  et_save_view(view_t *);
static int	  et_use_colormap(view_t *, colormap_t *);

/*
 * Our function switch table
 */
struct grfabs_sw et_vid_sw = {
	et_display_view,
	et_alloc_view,
	et_free_view,
	et_remove_view,
	et_save_view,
	et_use_colormap
};

static struct grfvideo_mode hw_modes[] = {
    { 
	0, "", 22450000,		/* num, descr, pix-clock	*/
	640, 400, 4,			/* width, height, depth		*/
	632/8, 672/8, 688/8, 808/8, 768/8,/* HBS, HBE, HSS, HSE, HT	*/
	399, 450, 408, 413, 449		/* VBS, VBE, VSS, VSE, VT	*/
    },
    { 
	0, "", 25175000,		/* num, descr, pix-clock	*/
	640, 480, 4,			/* width, height, depth		*/
	632/8, 672/8, 688/8, 752/8, 752/8,/* HBS, HBE, HSS, HSE, HT	*/
	481, 522, 490, 498, 522		/* VBS, VBE, VSS, VSE, VT	*/
    }
};

static dmode_t vid_modes[] = {
    { { NULL, NULL },
	"640x400", { 640, 400 }, 1, (void*)&hw_modes[0], &et_vid_sw },
    { { NULL, NULL },
	"640x480", { 640, 480 }, 1, (void*)&hw_modes[1], &et_vid_sw },
    { { NULL, NULL }, NULL,  }
};

#define	ET_NUMCLOCKS	32

static u_int et_clockfreqs[ET_NUMCLOCKS] = {
	 6293750,  7080500,  7875000,  8125000,
	 9000000,  9375000, 10000000, 11225000,
	12587500, 14161000, 15750000, 16250000,
	18000000, 18750000, 20000000, 22450000,
	25175000, 28322000, 31500000, 32500000,
	36000000, 37500000, 40000000, 44900000,
	50350000, 56644000, 63000000, 65000000,
	72000000, 75000000, 80000000, 89800000
};

static bmap_t	con_bm; /* XXX */

struct grfabs_et_priv {
	pcitag_t		pci_tag;
	void			*regkva;
	void 			*memkva;
	u_int			linbase;
	int			regsz;
	int			memsz;
	int			board_type;
} et_priv;

/*
 * Board types:
 */
#define	BT_ET4000		1
#define	BT_ET6000		2

/*
 * XXX: called from ite console init routine.
 * Initialize list of posible video modes.
 */
void
et_probe_video(MODES *modelp)
{
	dmode_t	*dm;
	int	i;

	for (i = 0; (dm = &vid_modes[i])->name != NULL; i++) {
		LIST_INSERT_HEAD(modelp, dm, link);
	}
}

static void
et_display_view(view_t *v)
{
	dmode_t		*dm = v->mode;
	bmap_t		*bm = v->bitmap;
	int		sv_size;
	u_short		*src, *dst;
	save_area_t	*sa;

	if (dm->current_view && (dm->current_view != v)) {
		/*
		 * Mark current view for this mode as no longer displayed
		 */
		dm->current_view->flags &= ~VF_DISPLAY;
	}
	dm->current_view = v;
	v->flags |= VF_DISPLAY;

	if ((sa = (save_area_t*)v->save_area) == NULL)
		return; /* XXX: Can't happen.... */

	/*
	 * Restore register settings and turn the plane pointer
	 * to the card-memory
	 */
	et_hwrest(&sa->sv_regs);
	bm->plane = et_priv.memkva;

	et_use_colormap(v, v->colormap);

	/*
	 * Copy the backing store to card-memory
	 */
	sv_size = sa->fb_size;
	src     = sa->sv_fb;
	dst     = (u_short *)bm->plane;
	while (sv_size--)
		*dst++ = *src++;
}

void
et_remove_view(view_t *v)
{
	dmode_t *mode = v->mode;

	if (mode->current_view == v) {
#if 0
		if (v->flags & VF_DISPLAY)
			panic("Cannot shutdown display"); /* XXX */
#endif
		mode->current_view = NULL;
	}
	v->flags &= ~VF_DISPLAY;
}

void
et_save_view(view_t *v)
{
	bmap_t		*bm = v->bitmap;
	u_char		font_height;
	int		sv_size;
	u_short		*src, *dst;
	save_area_t	*sa;
	volatile u_char *ba;

	if (!atari_realconfig)
		return;

	ba = et_priv.regkva;

	if (RGfx(ba, GCT_ID_MISC) & 1) {
#if 0 /* XXX: Can't use printf here.... */
		printf("et_save_view: Don't know how to save"
			" a graphics mode\n");
#endif
		return;
	}
	if (v->save_area == NULL)
		v->save_area = malloc(SAVEBUF_SIZE, M_DEVBUF, M_NOWAIT);

	/*
	 * Calculate the size of the copy
	 */
	font_height = RCrt(ba, CRT_ID_MAX_ROW_ADDRESS) & 0x1f;
	sv_size = bm->bytes_per_row * (bm->rows / (font_height + 1));
	sv_size = min(SAVEBUF_SIZE, sv_size);

	/*
	 * Save all we need to know....
	 */
	sa  = (save_area_t *)v->save_area;
	et_hwsave(&sa->sv_regs);
	sa->fb_size = sv_size;
	src = (u_short *)bm->plane;
	dst = sa->sv_fb;
	while (sv_size--)
		*dst++ = *src++;
	bm->plane = (u_char *)sa->sv_fb;
}

void
et_free_view(view_t *v)
{
	if(v) {
		et_remove_view(v);
		if (v->colormap != &gra_con_cmap)
			free(v->colormap, M_DEVBUF);
		if (v->save_area != NULL)
			free(v->save_area, M_DEVBUF);
		if (v != &gra_con_view) {
			free(v->bitmap, M_DEVBUF);
			free(v, M_DEVBUF);
		}
	}
}

static int
et_use_colormap(view_t *v, colormap_t *cm)
{
	return (0); /* XXX: Nothing here for now... */
}

static view_t *
et_alloc_view(dmode_t *mode, dimen_t *dim, u_char depth)
{
	view_t		*v;
	bmap_t		*bm;
	box_t		box;
	save_area_t	*sa;

	if (!atari_realconfig) {
		v  = &gra_con_view;
		bm = &con_bm;
	}
	else {
		v  = malloc(sizeof(*v), M_DEVBUF, M_WAITOK);
		bm = malloc(sizeof(*bm), M_DEVBUF, M_WAITOK);
	}
	v->bitmap = bm;

	/*
	 * Initialize the bitmap
	 */
	bm->plane         = et_priv.memkva;
	bm->vga_address   = (void *)kvtop(et_priv.memkva);
	bm->vga_base      = VGA_BASE;
	bm->hw_address    = (void *)(PCI_MEM_PHYS | et_priv.linbase);
	bm->lin_base      = et_priv.linbase;
	bm->regs          = et_priv.regkva;
	bm->hw_regs       = (void *)kvtop(et_priv.regkva);
	bm->reg_size      = REG_MAPPABLE;
	bm->phys_mappable = FRAME_MAPPABLE;
	bm->vga_mappable  = VGA_MAPPABLE;

	bm->bytes_per_row = (mode->size.width * depth) / NBBY;
	bm->rows          = mode->size.height;
	bm->depth         = depth;

	/*
	 * Allocate a save_area.
	 * Note: If atari_realconfig is false, no save area is (can be)
	 * allocated. This means that the plane is the video memory,
	 * which is what's wanted in this case.
	 */
	if (atari_realconfig) {
		v->save_area = malloc(SAVEBUF_SIZE, M_DEVBUF, M_WAITOK);
		sa           = (save_area_t*)v->save_area;
		sa->fb_size  = 0;
		bm->plane    = (u_char *)sa->sv_fb;
		et_loadmode(mode->data, &sa->sv_regs);
	}
	else v->save_area = NULL;
	
	v->colormap = alloc_colormap(mode);
	if (v->colormap) {
		INIT_BOX(&box,0,0,mode->size.width,mode->size.height);
		init_view(v, bm, mode, &box);
		return (v);
	}
	if (v != &gra_con_view) {
		free(v, M_DEVBUF);
		free(bm, M_DEVBUF);
	}
	return (NULL);
}

static void
init_view(view_t *v, bmap_t *bm, dmode_t *mode, box_t *dbox)
{
	v->bitmap    = bm;
	v->mode      = mode;
	v->flags     = 0;
	memcpy(&v->display, dbox, sizeof(box_t));
}

/* XXX: No more than a stub... */
static colormap_t *
alloc_colormap(dmode_t *dm)
{
	colormap_t	*cm;
	int		i;

	cm = &gra_con_cmap;
	cm->entry = gra_con_colors;

	cm->first = 0;
	cm->size  = 2;

	for (i = 0; i < 2; i++)
		cm->entry[i] = gra_def_color16[i % 16];
	return (cm);
}

/*
 * Go look for a VGA card on the PCI-bus. This search is a
 * stripped down version of the PCI-probe. It only looks on
 * bus0 for et4000/et6000 cards. The first card found is used.
 */
int
et_probe_card(void)
{
	pci_chipset_tag_t	pc = NULL; /* XXX */
	pcitag_t		tag;
	int			device, found, id, maxndevs;

	found    = 0;
	tag      = 0;
	id       = 0;
	maxndevs = pci_bus_maxdevs(pc, 0);

	for (device = 0; !found && (device < maxndevs); device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;
		switch (PCI_PRODUCT(id)) {
			case PCI_PRODUCT_TSENG_ET6000:
			case PCI_PRODUCT_TSENG_ET4000_W32P_A:
			case PCI_PRODUCT_TSENG_ET4000_W32P_B:
			case PCI_PRODUCT_TSENG_ET4000_W32P_C:
			case PCI_PRODUCT_TSENG_ET4000_W32P_D:
				found = 1;
				break;
			default:
				break;
		}
	}
	if (!found)
		return (0);

	if (PCI_PRODUCT(id) ==  PCI_PRODUCT_TSENG_ET6000)
		et_priv.board_type = BT_ET6000;
	else {
#ifdef ET4000_HAS_2MB_MEM
		volatile u_char *ba;
#endif

		et_priv.board_type = BT_ET4000;

#ifdef ET4000_HAS_2MB_MEM
		/* set KEY to access the tseng private registers */
		ba = (volatile void *)pci_io_addr;
		vgaw(ba, GREG_HERCULESCOMPAT, 0x03);
		vgaw(ba, GREG_DISPMODECONTROL, 0xa0);

		/* enable memory interleave */
		WCrt(ba, CRT_ID_RASCAS_CONFIG, 0xa0);
		WCrt(ba, CRT_ID_VIDEO_CONFIG2, 0x89);
#endif
	}

	et_priv.pci_tag = tag;

	/*
	 * The things below are setup in atari_init.c
	 */
	et_priv.regkva  = (void *)pci_io_addr;
	et_priv.memkva  = (void *)pci_mem_addr;
	et_priv.linbase = PCI_LINMEMBASE; /* XXX pci_conf_read??? */
	et_priv.memsz   = PCI_VGA_SIZE;
	et_priv.regsz   = PCI_IO_SIZE;

	if (!atari_realconfig) {
		et_loadmode(&hw_modes[0], NULL);
		return (1);
	}

	return (1);
}

static void
et_loadmode(struct grfvideo_mode *mode, et_sv_reg_t *regs)
{
	unsigned short	HDE, VDE;
	int	    	lace, dblscan;
	int     	uplim, lowlim;
	int		i;
	unsigned char	clock, tmp;
	volatile u_char	*ba;
	et_sv_reg_t	loc_regs;

	if (regs == NULL)
		regs = &loc_regs;

	ba  = et_priv.regkva;
	HDE = mode->disp_width / 8 - 1;
	VDE = mode->disp_height - 1;

	/* figure out whether lace or dblscan is needed */

	uplim   = mode->disp_height + (mode->disp_height / 4);
	lowlim  = mode->disp_height - (mode->disp_height / 4);
	lace    = (((mode->vtotal * 2) > lowlim)
		   && ((mode->vtotal * 2) < uplim)) ? 1 : 0;
	dblscan = (((mode->vtotal / 2) > lowlim)
		   && ((mode->vtotal / 2) < uplim)) ? 1 : 0;

	/* adjustments */
	if (lace)
		VDE /= 2;

	regs->misc_output = 0x23; /* Page 0, Color mode */
	regs->seg_sel     = 0x00;
	regs->state_ctl   = 0x00;

	regs->seq[SEQ_ID_RESET]           = 0x03; /* reset off		*/
	regs->seq[SEQ_ID_CLOCKING_MODE]   = 0x21; /* Turn off screen	*/
	regs->seq[SEQ_ID_MAP_MASK]        = 0xff; /* CPU writes all planes*/
	regs->seq[SEQ_ID_CHAR_MAP_SELECT] = 0x00; /* Char. generator 0	*/
	regs->seq[SEQ_ID_MEMORY_MODE]     = 0x0e; /* Seq. Memory mode	*/

	/*
	 * Set the clock...
	 */
	for(clock = ET_NUMCLOCKS-1; clock > 0; clock--) {
		if (et_clockfreqs[clock] <= mode->pixel_clock)
			break;
	}
	regs->misc_output |= (clock & 3) << 2;
	regs->aux_mode     = 0xb4 | ((clock & 8) << 3);
	regs->compat_6845  = (clock & 4) ? 0x0a : 0x08;

	/*
	 * The display parameters...
	 */
	regs->crt[CRT_ID_HOR_TOTAL]        =  mode->htotal;
	regs->crt[CRT_ID_HOR_DISP_ENA_END] = ((HDE >= mode->hblank_start)
						? mode->hblank_stop - 1
						: HDE);
	regs->crt[CRT_ID_START_HOR_BLANK]  = mode->hblank_start;
	regs->crt[CRT_ID_END_HOR_BLANK]    = (mode->hblank_stop & 0x1f) | 0x80;
	regs->crt[CRT_ID_START_HOR_RETR]   = mode->hsync_start;
	regs->crt[CRT_ID_END_HOR_RETR]     = (mode->hsync_stop & 0x1f)
						| ((mode->hblank_stop & 0x20)
							? 0x80 : 0x00);
	regs->crt[CRT_ID_VER_TOTAL]        = mode->vtotal;
	regs->crt[CRT_ID_START_VER_RETR]   = mode->vsync_start;
	regs->crt[CRT_ID_END_VER_RETR]     = (mode->vsync_stop & 0x0f) | 0x30;
	regs->crt[CRT_ID_VER_DISP_ENA_END] = VDE;
	regs->crt[CRT_ID_START_VER_BLANK]  = mode->vblank_start;
	regs->crt[CRT_ID_END_VER_BLANK]    = mode->vblank_stop;
	regs->crt[CRT_ID_MODE_CONTROL]     = 0xab;
	regs->crt[CRT_ID_START_ADDR_HIGH]  = 0x00;
	regs->crt[CRT_ID_START_ADDR_LOW]   = 0x00;
	regs->crt[CRT_ID_LINE_COMPARE]     = 0xff;
	regs->crt[CRT_ID_UNDERLINE_LOC]    = 0x00;
	regs->crt[CRT_ID_PRESET_ROW_SCAN]  = 0x00;
	regs->crt[CRT_ID_OFFSET]           = mode->disp_width/16;
	regs->crt[CRT_ID_MAX_ROW_ADDRESS]  =
		0x40 |
		(dblscan ? 0x80 : 0x00) |
		((mode->vblank_start & 0x200) ? 0x20 : 0x00);
	regs->crt[CRT_ID_OVERFLOW] =
		0x10 |
		((mode->vtotal       & 0x100) ? 0x01 : 0x00) |
		((VDE                & 0x100) ? 0x02 : 0x00) |
		((mode->vsync_start  & 0x100) ? 0x04 : 0x00) |
		((mode->vblank_start & 0x100) ? 0x08 : 0x00) |
		((mode->vtotal       & 0x200) ? 0x20 : 0x00) |
		((VDE                & 0x200) ? 0x40 : 0x00) |
		((mode->vsync_start  & 0x200) ? 0x80 : 0x00);
	regs->overfl_high =
		0x10 |
		((mode->vblank_start & 0x400) ? 0x01 : 0x00) |
		((mode->vtotal       & 0x400) ? 0x02 : 0x00) |
		((VDE                & 0x400) ? 0x04 : 0x00) |
		((mode->vsync_start  & 0x400) ? 0x08 : 0x00) |
		(lace ? 0x80 : 0x00);
	regs->hor_overfl =
		((mode->htotal       & 0x100) ? 0x01 : 0x00) |
		((mode->hblank_start & 0x100) ? 0x04 : 0x00) |
		((mode->hsync_start  & 0x100) ? 0x10 : 0x00);

	regs->grf[GCT_ID_SET_RESET]        = 0x00;
	regs->grf[GCT_ID_ENABLE_SET_RESET] = 0x00;
	regs->grf[GCT_ID_COLOR_COMPARE]    = 0x00;
	regs->grf[GCT_ID_DATA_ROTATE]      = 0x00;
	regs->grf[GCT_ID_READ_MAP_SELECT]  = 0x00;
	regs->grf[GCT_ID_GRAPHICS_MODE]    = mode->depth == 1 ? 0x00: 0x40;
	regs->grf[GCT_ID_MISC]             = 0x01;
	regs->grf[GCT_ID_COLOR_XCARE]      = 0x0f;
	regs->grf[GCT_ID_BITMASK]          = 0xff;

	for (i = 0; i < 0x10; i++)
		regs->attr[i] = i;
	regs->attr[ACT_ID_ATTR_MODE_CNTL]  = 0x01;
	regs->attr[ACT_ID_OVERSCAN_COLOR]  = 0x00;
	regs->attr[ACT_ID_COLOR_PLANE_ENA] = 0x0f;
	regs->attr[ACT_ID_HOR_PEL_PANNING] = 0x00;
	regs->attr[ACT_ID_COLOR_SELECT]    = 0x00;
	regs->attr[ACT_ID_MISCELLANEOUS]   = 0x00;

	/*
	 * XXX: This works for depth == 4. I need some better docs
	 * to fix the other modes....
	 */
	/*
	 * What we need would be probe functions for RAMDAC/clock chip
	 */
	vgar(ba, VDAC_ADDRESS);		/* clear old state */
	vgar(ba, VDAC_MASK);
	vgar(ba, VDAC_MASK);
	vgar(ba, VDAC_MASK);
	vgar(ba, VDAC_MASK);

	vgaw(ba, VDAC_MASK, 0);		/* set to palette */
	vgar(ba, VDAC_ADDRESS);		/* clear state */

	vgaw(ba, VDAC_MASK, 0xff);
	/*
	 * End of depth stuff
	 */

	/*
	 * Compute Hsync & Vsync polarity
	 * Note: This seems to be some kind of a black art :-(
	 */
	tmp = regs->misc_output & 0x3f;
#if 1 /* This is according to my BW monitor & Xfree... */
	if (VDE < 400) 
		tmp |= 0x40;	/* -hsync +vsync */
	else if (VDE < 480)
		tmp |= 0xc0;	/* -hsync -vsync */
#else /* This is according to my color monitor.... */
	if (VDE < 400) 
		tmp |= 0x00;	/* +hsync +vsync */
	else if (VDE < 480)
		tmp |= 0x80;	/* +hsync -vsync */
#endif
	/* I'm unable to try the rest.... */
	regs->misc_output = tmp;

	if(regs == &loc_regs)
		et_hwrest(regs);
}

void
et_hwsave(et_sv_reg_t *et_regs)
{
	volatile u_char *ba;
	int		i, s;

	ba = et_priv.regkva;

	s = splhigh();

	/*
	 * General VGA registers
	 */
	et_regs->misc_output = vgar(ba, GREG_MISC_OUTPUT_R);
	for(i = 0; i < 25; i++)
		et_regs->crt[i]  = RCrt(ba, i);
	for(i = 0; i < 21; i++)
		et_regs->attr[i] = RAttr(ba, i | 0x20);
	for(i = 0; i < 9; i++)
		et_regs->grf[i]  = RGfx(ba, i);
	for(i = 0; i < 5; i++)
		et_regs->seq[i]  = RSeq(ba, i);

	/*
	 * ET4000 extensions
	 */
	et_regs->ext_start   = RCrt(ba, CTR_ID_EXT_START);
	et_regs->compat_6845 = RCrt(ba, CRT_ID_6845_COMPAT);
	et_regs->overfl_high = RCrt(ba, CRT_ID_OVERFLOW_HIGH);
	et_regs->hor_overfl  = RCrt(ba, CRT_ID_HOR_OVERFLOW);
	et_regs->state_ctl   = RSeq(ba, SEQ_ID_STATE_CONTROL);
	et_regs->aux_mode    = RSeq(ba, SEQ_ID_AUXILIARY_MODE);
	et_regs->seg_sel     = vgar(ba, GREG_SEGMENTSELECT);

	splx(s);
}

void
et_hwrest(et_sv_reg_t *et_regs)
{
	volatile u_char *ba;
	int		i, s;

	ba = et_priv.regkva;

	s = splhigh();

	vgaw(ba, GREG_SEGMENTSELECT, 0);
	vgaw(ba, GREG_MISC_OUTPUT_W, et_regs->misc_output);

	/*
	 * General VGA registers
	 */
	WSeq(ba, SEQ_ID_RESET, 0x01);
	for(i = 1; i < 5; i++)
		WSeq(ba, i, et_regs->seq[i]);
	WSeq(ba, SEQ_ID_RESET, 0x03);

	/*
	 * Make sure we're allowed to write all crt-registers
	 */
	WCrt(ba, CRT_ID_END_VER_RETR,
		et_regs->crt[CRT_ID_END_VER_RETR] & 0x7f);
	for(i = 0; i < 25; i++)
		WCrt(ba, i, et_regs->crt[i]);
	for(i = 0; i < 9; i++)
		WGfx(ba, i, et_regs->grf[i]);
	for(i = 0; i < 21; i++)
		WAttr(ba, i | 0x20, et_regs->attr[i]);

	/*
	 * ET4000 extensions
	 */
	WSeq(ba, SEQ_ID_STATE_CONTROL, et_regs->state_ctl);
	WSeq(ba, SEQ_ID_AUXILIARY_MODE, et_regs->aux_mode);
	WCrt(ba, CTR_ID_EXT_START, et_regs->ext_start);
	WCrt(ba, CRT_ID_6845_COMPAT, et_regs->compat_6845);
	WCrt(ba, CRT_ID_OVERFLOW_HIGH, et_regs->overfl_high);
	WCrt(ba, CRT_ID_HOR_OVERFLOW, et_regs->hor_overfl);
	vgaw(ba, GREG_SEGMENTSELECT, et_regs->seg_sel);

	i = et_regs->seq[SEQ_ID_CLOCKING_MODE] & ~0x20;
	WSeq(ba, SEQ_ID_CLOCKING_MODE, i);

	splx(s);
}
