/*	$NetBSD: grfabs_et.c,v 1.2 1996/10/11 00:09:21 christos Exp $	*/

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
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/systm.h>

/*
 * For PCI probing...
 */
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/iomap.h>
#include <machine/video.h>
#include <machine/mfp.h>
#include <atari/atari/device.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grf_etreg.h>

#define	SAVEBUF_SIZE	(32*1024)

/*
 * Function decls
 */
static void       init_view __P((view_t *, bmap_t *, dmode_t *, box_t *));
static colormap_t *alloc_colormap __P((dmode_t *));
static void	  et_display_view __P((view_t *));
static view_t	  *et_alloc_view __P((dmode_t *, dimen_t *, u_char));
static void	  et_boardinit __P((void));
static void	  et_free_view __P((view_t *));
static void	  et_loadmode __P((struct grfvideo_mode *));
static void	  et_remove_view __P((view_t *));
static void	  et_save_view __P((view_t *));
static int	  et_use_colormap __P((view_t *, colormap_t *));

int	  et_probe_card __P((void)); /* XXX: to include file */
void	pccninit(void);	/* XXX: remove me */

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
#if 1
    { 
	0, "", 25175000,		/* num, descr, pix-clock	*/
	640, 480, 4,			/* width, height, depth		*/
	640/8, 672/8, 688/8, 752/8, 768/8,/* HBS, HBE, HSS, HSE, HT	*/
	481, 522, 490, 498, 522		/* VBS, VBE, VSS, VSE, VT	*/
    }
#else
    { 
	0, "", 25175000,		/* num, descr, pix-clock	*/
	640, 400, 4,			/* width, height, depth		*/
	640/8, 672/8, 688/8, 752/8, 768/8,/* HBS, HBE, HSS, HSE, HT	*/
	399, 450, 408, 413, 449		/* VBS, VBE, VSS, VSE, VT	*/
    }
#endif
};

static dmode_t vid_modes[] = {
    { { NULL, NULL },
	"ethigh", { 640, 480 }, 1, (void*)&hw_modes[0], &et_vid_sw },
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
	volatile caddr_t	regkva;
	volatile caddr_t	memkva;
	int			regsz;
	int			memsz;
	struct grfvideo_mode	*curr_mode;
} et_priv;

/*
 * XXX: called from ite console init routine.
 * Initialize list of posible video modes.
 */
void et_probe_video __P((MODES *));
void
et_probe_video(modelp)
MODES	*modelp;
{
	dmode_t	*dm;
	int	i;

	for (i = 0; (dm = &vid_modes[i])->name != NULL; i++) {
		LIST_INSERT_HEAD(modelp, dm, link);
	}
}

static void
et_display_view(v)
view_t *v;
{
	dmode_t		*dm = v->mode;
	bmap_t		*bm = v->bitmap;
	u_char		font_height;
	int		sv_size;
	u_short		*src, *dst;

	if (dm->current_view && (dm->current_view != v)) {
		/*
		 * Mark current view for this mode as no longer displayed
		 */
		dm->current_view->flags &= ~VF_DISPLAY;
	}
	dm->current_view = v;
	v->flags |= VF_DISPLAY;

	if (et_priv.curr_mode != (struct grfvideo_mode *)dm->data) {
		et_loadmode(dm->data);
		et_priv.curr_mode = dm->data;
	}
	et_use_colormap(v, v->colormap);
	bm->plane = et_priv.memkva;

	if (v->save_area == NULL)
		return; /* XXX: Can't happen.... */
	if (RGfx(et_priv.regkva, GCT_ID_MISC) & 1) {
		kprintf("et_display_view: Don't know how to restore"
			" a graphics mode\n");
		return;
	}

	/*
	 * Calculate the size of the copy
	 */
	font_height = RCrt(et_priv.regkva, CRT_ID_MAX_ROW_ADDRESS) & 0x1f;
	sv_size = bm->bytes_per_row * (bm->rows / (font_height + 1));

	src = (u_short *)v->save_area;
	dst = (u_short *)bm->plane;
	while (sv_size--)
		*dst++ = *src++;
}

void
et_remove_view(v)
view_t *v;
{
	dmode_t *mode = v->mode;

	if (mode->current_view == v) {
#if 0
		if (v->flags & VF_DISPLAY)
			panic("Cannot shutdown display\n"); /* XXX */
#endif
		mode->current_view = NULL;
	}
	v->flags &= ~VF_DISPLAY;
}

void
et_save_view(v)
view_t *v;
{
	bmap_t	*bm = v->bitmap;
	u_char	font_height;
	int	sv_size;
	u_short	*src, *dst;

	if (!atari_realconfig)
		return;
	if (RGfx(et_priv.regkva, GCT_ID_MISC) & 1) {
		kprintf("et_save_view: Don't know how to save"
			" a graphics mode\n");
		return;
	}
	if (v->save_area == NULL)
		v->save_area = malloc(SAVEBUF_SIZE, M_DEVBUF, M_WAITOK);

	/*
	 * Calculate the size of the copy
	 */
	font_height = RCrt(et_priv.regkva, CRT_ID_MAX_ROW_ADDRESS) & 0x1f;
	sv_size = bm->bytes_per_row * (bm->rows / (font_height + 1));

	src = (u_short *)bm->plane;
	dst = (u_short *)v->save_area;
	while (sv_size--)
		*dst++ = *src++;
	bm->plane = (u_char *)v->save_area;
}

void
et_free_view(v)
view_t *v;
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
et_use_colormap(v, cm)
view_t		*v;
colormap_t	*cm;
{
	return (0); /* XXX: Nothing here for now... */
}

static view_t *
et_alloc_view(mode, dim, depth)
dmode_t	*mode;
dimen_t	*dim;
u_char   depth;
{
	view_t *v;
	bmap_t *bm;
	box_t   box;

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
	bm->hw_address    = et_priv.memkva; /* XXX: kvtop? */
	bm->regs          = et_priv.regkva;
	bm->hw_regs       = et_priv.regkva; /* XXX: kvtop? */
	bm->reg_size      = et_priv.regsz;

	bm->bytes_per_row = (mode->size.width * depth) / NBBY;
	bm->rows          = mode->size.height;
	bm->depth         = depth;

	/*
	 * Allocate a save_area.
	 * Note: If atari_realconfig is false, no save area is (can be)
	 * allocated. This means that the plane is the video memory,
	 * wich is what's wanted in this case.
	 */
	if (atari_realconfig) {
		v->save_area = malloc(SAVEBUF_SIZE, M_DEVBUF, M_WAITOK);
		bm->plane    = (u_char *)v->save_area;
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
init_view(v, bm, mode, dbox)
view_t	*v;
bmap_t	*bm;
dmode_t	*mode;
box_t	*dbox;
{
	v->bitmap    = bm;
	v->mode      = mode;
	v->flags     = 0;
	bcopy(dbox, &v->display, sizeof(box_t));
}

/* XXX: No more than a stub... */
static colormap_t *
alloc_colormap(dm)
dmode_t		*dm;
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
 * bus0 for simple cards.
 */
int
et_probe_card()
{
	pci_chipset_tag_t	pc = NULL; /* XXX */
	pcitag_t		tag;
	int			class, device, found, id, maxndevs;

	found    = 0;
	tag      = 0;
	maxndevs = pci_bus_maxdevs(pc, 0);

	for (device = 0; device < maxndevs; device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;
		class = pci_conf_read(pc, tag, PCI_CLASS_REG);
		if (PCI_CLASS(class) == PCI_CLASS_PREHISTORIC
		    && PCI_SUBCLASS(class) == PCI_SUBCLASS_PREHISTORIC_VGA) {
			found = 1;
			break;
		}
		if (PCI_CLASS(class) == PCI_CLASS_DISPLAY
		    && PCI_SUBCLASS(class) == PCI_SUBCLASS_DISPLAY_VGA) {
			found = 1;
			break;
		}
	}
	if (!found)
		return (0);

	et_priv.pci_tag = tag;

	/*
	 * The things below are setup in atari_init.c
	 */
	et_priv.regkva  = (volatile caddr_t)pci_io_addr;
	et_priv.memkva  = (volatile caddr_t)pci_mem_addr;
	et_priv.memsz   = 4 * NBPG;
	et_priv.regsz   = NBPG;

	if (found && !atari_realconfig) {
		et_boardinit();
		et_loadmode(&hw_modes[0]);
		return (1);
	}

	return (1);
}

static void
et_loadmode(mode)
struct grfvideo_mode *mode;
{
	unsigned short	HDE, VDE;
	int	    	lace, dblscan;
	int     	uplim, lowlim;
	unsigned char	clock, tmp;
	volatile u_char	*ba;

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

	WSeq(ba, SEQ_ID_CLOCKING_MODE,   0x21); /* Turn off screen	*/
	WSeq(ba, SEQ_ID_MEMORY_MODE,     0x0e);	/* Seq. Memory mode	*/
	WSeq(ba, SEQ_ID_MAP_MASK,        0xff);	/* Cpu writes all planes*/
	WSeq(ba, SEQ_ID_CHAR_MAP_SELECT, 0x00); /* Char. generator 0	*/
	WGfx(ba, GCT_ID_READ_MAP_SELECT, 0x00);	/* Cpu reads plane 0	*/

	/*
	 * Set the clock...
	 */
	for(clock = ET_NUMCLOCKS-1; clock > 0; clock--) {
		if (et_clockfreqs[clock] <= mode->pixel_clock)
			break;
	}
	tmp = vgar(ba, GREG_MISC_OUTPUT_R) & 0xf3;
	vgaw(ba, GREG_MISC_OUTPUT_W, tmp | ((clock & 3) << 2));
	WSeq(ba, SEQ_ID_AUXILIARY_MODE, ((clock & 8) << 3) |
		(RSeq(ba, SEQ_ID_AUXILIARY_MODE) & 0xbf));
	WCrt(ba, CRT_ID_6845_COMPAT, (clock & 4) ? 0x0a : 0x08);

	/*
	 * load display parameters into board
	 */
	WCrt(ba, CRT_ID_HOR_TOTAL, mode->htotal);
	WCrt(ba, CRT_ID_HOR_DISP_ENA_END, ((HDE >= mode->hblank_start)
					  ? mode->hblank_stop - 1
					  : HDE));
	WCrt(ba, CRT_ID_START_HOR_BLANK, mode->hblank_start);
	WCrt(ba, CRT_ID_END_HOR_BLANK, (mode->hblank_stop & 0x1f) | 0x80);
	WCrt(ba, CRT_ID_START_HOR_RETR, mode->hsync_start);
	WCrt(ba, CRT_ID_END_HOR_RETR,
		(mode->hsync_stop & 0x1f) |
		((mode->hblank_stop & 0x20) ? 0x80 : 0x00));
	WCrt(ba, CRT_ID_VER_TOTAL, mode->vtotal);
	WCrt(ba, CRT_ID_OVERFLOW, 
		0x10 |
		((mode->vtotal       & 0x100) ? 0x01 : 0x00) |
		((VDE                & 0x100) ? 0x02 : 0x00) |
		((mode->vsync_start  & 0x100) ? 0x04 : 0x00) |
		((mode->vblank_start & 0x100) ? 0x08 : 0x00) |
		((mode->vtotal       & 0x200) ? 0x20 : 0x00) |
		((VDE                & 0x200) ? 0x40 : 0x00) |
		((mode->vsync_start  & 0x200) ? 0x80 : 0x00));
	WCrt(ba, CRT_ID_OVERFLOW_HIGH,
		0x10 |
		((mode->vblank_start & 0x400) ? 0x01 : 0x00) |
		((mode->vtotal       & 0x400) ? 0x02 : 0x00) |
		((VDE                & 0x400) ? 0x04 : 0x00) |
		((mode->vsync_start  & 0x400) ? 0x08 : 0x00) |
		(lace ? 0x80 : 0x00));
	WCrt(ba, CRT_ID_HOR_OVERFLOW,
		((mode->htotal       & 0x100) ? 0x01 : 0x00) |
		((mode->hblank_start & 0x100) ? 0x04 : 0x00) |
		((mode->hsync_start  & 0x100) ? 0x10 : 0x00));
	WCrt(ba, CRT_ID_MAX_ROW_ADDRESS,
		0x40 |
		(dblscan ? 0x80 : 0x00) |
		((mode->vblank_start & 0x200) ? 0x20 : 0x00));
	WCrt(ba, CRT_ID_START_VER_RETR, mode->vsync_start);
	WCrt(ba, CRT_ID_END_VER_RETR, (mode->vsync_stop & 0x0f) | 0x30);
	WCrt(ba, CRT_ID_VER_DISP_ENA_END, VDE);
	WCrt(ba, CRT_ID_START_VER_BLANK, mode->vblank_start);
	WCrt(ba, CRT_ID_END_VER_BLANK, mode->vblank_stop);

	WCrt(ba, CRT_ID_MODE_CONTROL, 0xab);
	WCrt(ba, CRT_ID_START_ADDR_HIGH, 0x00);
	WCrt(ba, CRT_ID_START_ADDR_LOW, 0x00);
	WCrt(ba, CRT_ID_LINE_COMPARE, 0xff);

	/* depth dependent stuff */
	WGfx(ba, GCT_ID_GRAPHICS_MODE, mode->depth == 1 ? 0x00: 0x40);
	WGfx(ba, GCT_ID_MISC, 0x01);

	/*
	 * XXX: This works for depth == 4. I need some better docs
	 * to fix the other modes....
	 */
	vgaw(ba, VDAC_MASK, 0xff);
	vgar(ba, VDAC_MASK);
	vgar(ba, VDAC_MASK);
	vgar(ba, VDAC_MASK);
	vgar(ba, VDAC_MASK);

	vgaw(ba, VDAC_MASK, 0);
	HDE = mode->disp_width / 16; /* XXX */

	WAttr(ba, ACT_ID_ATTR_MODE_CNTL, 0x01);
	WAttr(ba, 0x20 | ACT_ID_COLOR_PLANE_ENA, mode->depth == 1 ? 0: 0x0f);

	WCrt(ba, CRT_ID_OFFSET, HDE);
	/*
	 * End of depth stuff
	 */

	/*
	 * Compute Hsync & Vsync polarity
	 * Note: This seems to be some kind of a black art :-(
	 */
	tmp = vgar(ba, GREG_MISC_OUTPUT_R) & 0x3f;
#if 0 /* This is according to my BW monitor & Xfree... */
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
	vgaw(ba, GREG_MISC_OUTPUT_W, tmp);

	WSeq(ba, SEQ_ID_CLOCKING_MODE, 0x01); /* Turn on screen	*/
}

static void
et_boardinit()
{
	volatile u_char *ba;
	int		i, j;

	ba = et_priv.regkva;
	
	vgaw(ba, GREG_HERCULESCOMPAT,     0x03);
	vgaw(ba, GREG_DISPMODECONTROL,    0xa0);
	vgaw(ba, GREG_MISC_OUTPUT_W,      0x23);

	WSeq(ba, SEQ_ID_RESET,            0x03);
	WSeq(ba, SEQ_ID_CLOCKING_MODE,    0x21);	/* 8 dot, Display off */
	WSeq(ba, SEQ_ID_MAP_MASK,         0x0f);
	WSeq(ba, SEQ_ID_CHAR_MAP_SELECT,  0x00);
	WSeq(ba, SEQ_ID_MEMORY_MODE,      0x0e);
	WSeq(ba, SEQ_ID_AUXILIARY_MODE,   0xf4);

	WCrt(ba, CRT_ID_PRESET_ROW_SCAN,  0x00);
	WCrt(ba, CRT_ID_CURSOR_START,     0x00);
	WCrt(ba, CRT_ID_CURSOR_END,       0x08);
	WCrt(ba, CRT_ID_START_ADDR_HIGH,  0x00);
	WCrt(ba, CRT_ID_START_ADDR_LOW,   0x00);
	WCrt(ba, CRT_ID_CURSOR_LOC_HIGH,  0x00);
	WCrt(ba, CRT_ID_CURSOR_LOC_LOW,   0x00);

	WCrt(ba, CRT_ID_UNDERLINE_LOC,    0x07);
	WCrt(ba, CRT_ID_MODE_CONTROL,     0xa3);
	WCrt(ba, CRT_ID_LINE_COMPARE,     0xff);
	/*
	 * ET4000 special
	 */
	WCrt(ba, CRT_ID_RASCAS_CONFIG,    0x28);
	WCrt(ba, CTR_ID_EXT_START,        0x00);
	WCrt(ba, CRT_ID_6845_COMPAT,      0x08);
	WCrt(ba, CRT_ID_VIDEO_CONFIG1,    0x73);
	WCrt(ba, CRT_ID_VIDEO_CONFIG2,    0x09);
	
	WCrt(ba, CRT_ID_HOR_OVERFLOW,     0x00);

	WGfx(ba, GCT_ID_SET_RESET,        0x00);
	WGfx(ba, GCT_ID_ENABLE_SET_RESET, 0x00);
	WGfx(ba, GCT_ID_COLOR_COMPARE,    0x00);
	WGfx(ba, GCT_ID_DATA_ROTATE,      0x00);
	WGfx(ba, GCT_ID_READ_MAP_SELECT,  0x00);
	WGfx(ba, GCT_ID_GRAPHICS_MODE,    0x00);
	WGfx(ba, GCT_ID_MISC,             0x01);
	WGfx(ba, GCT_ID_COLOR_XCARE,      0x0f);
	WGfx(ba, GCT_ID_BITMASK,          0xff);

	vgaw(ba, GREG_SEGMENTSELECT,      0x00);

	for (i = 0; i < 0x10; i++)
		WAttr(ba, i, i);
	WAttr(ba, ACT_ID_ATTR_MODE_CNTL,  0x01);
	WAttr(ba, ACT_ID_OVERSCAN_COLOR,  0x00);
	WAttr(ba, ACT_ID_COLOR_PLANE_ENA, 0x0f);
	WAttr(ba, ACT_ID_HOR_PEL_PANNING, 0x00);
	WAttr(ba, ACT_ID_COLOR_SELECT,    0x00);
	WAttr(ba, ACT_ID_MISCELLANEOUS,   0x00);

	vgaw(ba, VDAC_MASK, 0xff);

#if 0 /* XXX: We like to do this: */
	delay(200000);
#else /* But because we run before the delay is initialized: */
	for(i = 0; i < 4000; i++)
		for(j =  0; j < 400; j++);
#endif

	/*
	 * colors initially set to greyscale
	 */
	vgaw(ba, VDAC_ADDRESS_W, 0);
	for (i = 255; i >= 0; i--) {
		vgaw(ba, VDAC_DATA, i);
		vgaw(ba, VDAC_DATA, i);
		vgaw(ba, VDAC_DATA, i);
	}
}
