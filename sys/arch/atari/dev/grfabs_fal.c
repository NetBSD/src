/*	$NetBSD: grfabs_fal.c,v 1.17 2003/07/15 01:19:49 lukem Exp $	*/

/*
 * Copyright (c) 1995 Thomas Gerner.
 * Copyright (c) 1995 Leo Weppelman.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grfabs_fal.c,v 1.17 2003/07/15 01:19:49 lukem Exp $");

#ifdef FALCON_VIDEO
/*
 *  atari abstract graphics driver: Falcon-interface
 */
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/iomap.h>
#include <machine/video.h>
#include <machine/mfp.h>
#include <atari/atari/device.h>
#include <atari/atari/stalloc.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grfabs_fal.h>

/*
 * Function decls
 */
static void       init_view __P((view_t *, bmap_t *, dmode_t *, box_t *));
static bmap_t	  *alloc_bitmap __P((u_long, u_long, u_char));
static colormap_t *alloc_colormap __P((dmode_t *));
static void 	  free_bitmap __P((bmap_t *));
static void	  falcon_display_view __P((view_t *));
static view_t	  *falcon_alloc_view __P((dmode_t *, dimen_t *, u_char));
static void	  falcon_free_view __P((view_t *));
static void	  falcon_remove_view __P((view_t *));
static void	  falcon_save_view __P((view_t *));
static int	  falcon_use_colormap __P((view_t *, colormap_t *));
static void	  falcon_detect __P((dmode_t *));
static struct videl *falcon_getreg __P((u_short));

/*
 * Our function switch table
 */
struct grfabs_sw fal_vid_sw = {
	falcon_display_view,
	falcon_alloc_view,
	falcon_free_view,
	falcon_remove_view,
	falcon_save_view,
	falcon_use_colormap
};

struct falcon_hwregs {
	u_short		fal_mode;	/* falcon mode		  */
	struct videl	*fal_regs;	/* videl register values  */
};
#define vm_mode(dm)	(((struct falcon_hwregs*)(dm->data))->fal_mode)
#define vm_regs(dm)	(((struct falcon_hwregs*)(dm->data))->fal_regs)

/*
 * Note that the order of this table *must* match the order of
 * the table below!
 */
static struct falcon_hwregs fal_hwregs[] = {
	{ RES_FALAUTO    },
	{ RES_FAL_STHIGH },
	{ RES_FAL_STMID  },
	{ RES_FAL_STLOW  },
	{ RES_FAL_TTLOW  },
	{ RES_VGA2       },
	{ RES_VGA4       },
	{ RES_VGA16      },
	{ RES_VGA256     },
	{ RES_DIRECT     }
};
	

static dmode_t vid_modes[] = {
    { {NULL,NULL}, "falauto", {   0,  0 },  0, NULL, &fal_vid_sw},
    { {NULL,NULL}, "sthigh",  { 640,400 },  1, NULL, &fal_vid_sw},
    { {NULL,NULL}, "stmid",   { 640,200 },  2, NULL, &fal_vid_sw},
    { {NULL,NULL}, "stlow",   { 320,200 },  4, NULL, &fal_vid_sw},
    { {NULL,NULL}, "ttlow",   { 320,480 },  8, NULL, &fal_vid_sw},
    { {NULL,NULL}, "vga2",    { 640,480 },  1, NULL, &fal_vid_sw},
    { {NULL,NULL}, "vga4",    { 640,480 },  2, NULL, &fal_vid_sw},
    { {NULL,NULL}, "vga16",   { 640,480 },  4, NULL, &fal_vid_sw},
    { {NULL,NULL}, "vga256",  { 640,480 },  8, NULL, &fal_vid_sw},
    { {NULL,NULL}, "highcol", { 320,200 }, 16, NULL, &fal_vid_sw},
    { {NULL,NULL},  NULL,  }
};

/*
 * The following table contains timing values for the various video modes.
 * I have only a multisync display, therefore I can not say if this values
 * are useful at other displays.
 * Use other video modes at YOUR OWN RISK.
 * THERE IS NO WARRENTY ABOUT THIS VALUES TO WORK WITH A PARTICULAR
 * DISPLAY. -- Thomas
 */
static struct videl videlinit[] = {
	{ RES_FALAUTO,		    /* autodedect			    */
	0x0, 0x0, 0x0,   0x0, 0x0,   0x0,  0x0,  0x0,  0x0,   0x0,  0x0,  0x0,
	0x0, 0x0,   0x0,   0x0,  0x0,  0x0,   0x0,   0x0,   0x0 },

	{ FAL_VGA | RES_FAL_STHIGH, /* sthigh, 640x400, 2 colors	    */ 
	0x2, 0x0, 0x28,  0x0, 0x400, 0xc6, 0x8d, 0x15, 0x273, 0x50, 0x96, 0x0, 
	0x0, 0x419, 0x3af, 0x8f, 0x8f, 0x3af, 0x415, 0x186, 0x8 },

#if 0 /* not yet */
	{ FAL_SM | RES_FAL_STHIGH,  /* sthigh, 640x400, 2 colors	    */
	0x0, 0x0, 0x28,  0x2, 0x0,   0x1a, 0x0,  0x0,  0x20f, 0xc,  0x14, 0x0,
	0x0, 0x3e9, 0x0,   0x0,  0x43, 0x363, 0x3e7, 0x80,  0x8 },
#endif

	{ FAL_VGA | RES_FAL_STMID,  /* stmid, 640x200, 4 colors		    */
	0x2, 0x0, 0x50,  0x1, 0x0,   0x17, 0x12, 0x1,  0x20e, 0xd,  0x11, 0x0, 
	0x0, 0x419, 0x3af, 0x8f, 0x8f, 0x3af, 0x415, 0x186, 0x9 },

	{ FAL_VGA | RES_FAL_STLOW,  /* stlow, 320x200, 16 colors	    */
	0x2, 0x0, 0x50,  0x0, 0x0,   0x17, 0x12, 0x1,  0x20e, 0xd,  0x11, 0x0, 
	0x0, 0x419, 0x3af, 0x8f, 0x8f, 0x3af, 0x415, 0x186, 0x5 },

	{ FAL_VGA | RES_FAL_TTLOW,  /* ttlow, 320x480, 256 colors	    */
	0x2, 0x0, 0xa0,  0x0, 0x10,  0xc6, 0x8d, 0x15, 0x29a, 0x7b, 0x96, 0x0, 
	0x0, 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415, 0x186, 0x4 },

	{ FAL_VGA | RES_VGA2,	    /* vga, 640x480, 2 colors		    */
	0x2, 0x0, 0x28,  0x0, 0x400, 0xc6, 0x8d, 0x15, 0x273, 0x50, 0x96, 0x0, 
	0x0, 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415, 0x186, 0x8 },

	{ FAL_VGA | RES_VGA4,	    /* vga, 640x480, 4 colors		    */
	0x2, 0x0, 0x50,  0x1, 0x0,   0x17, 0x12, 0x1,  0x20e, 0xd,  0x11, 0x0, 
	0x0, 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415, 0x186, 0x8 },

	{ FAL_VGA | RES_VGA16,	    /* vga, 640x480, 16 colors		    */
	0x2, 0x0, 0xa0,  0x1, 0x0,   0xc6, 0x8d, 0x15, 0x2a3, 0x7c, 0x96, 0x0, 
	0x0, 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415, 0x186, 0x8 },

	{ FAL_VGA | RES_VGA256,	    /* vga, 640x480, 256 colors		    */
	0x2, 0x0, 0x140, 0x1, 0x10,  0xc6, 0x8d, 0x15, 0x2ab, 0x84, 0x96, 0x0, 
	0x0, 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415, 0x186, 0x8 },

	{ FAL_VGA | RES_DIRECT,	    /* direct video, 320x200, 65536 colors  */
	0x2, 0x0, 0x140, 0x0, 0x100, 0xc6, 0x8d, 0x15, 0x2ac, 0x91, 0x96, 0x0, 
	0x0, 0x419, 0x3ff, 0x3f, 0x3f, 0x3ff, 0x415, 0x186, 0x4 },

	{ 0xffff }		    /* end of list			    */
};

static u_short mon_type;
/*
 * XXX: called from ite console init routine.
 * Initialize list of possible video modes.
 */
void
falcon_probe_video(modelp)
MODES	*modelp;
{
	dmode_t	*dm;
	struct videl *vregs;
	int	i;

	mon_type = *(volatile unsigned char *)(AD_FAL_MON_TYPE);
	mon_type = (mon_type & 0xc0) << 2;

	/*
	 * get all possible modes
	 */

	for (i = 0; (dm = &vid_modes[i])->name != NULL; i++) {
		dm->data = (void *)&fal_hwregs[i];
		if (vm_mode(dm) == RES_FALAUTO) {
			vm_regs(dm) = falcon_getreg(RES_FALAUTO);
			falcon_detect(dm);
			LIST_INSERT_HEAD(modelp, dm, link);
		} else {
			vregs = falcon_getreg(vm_mode(dm) | mon_type);
			if (vregs) {
				vm_regs(dm) = vregs;
				LIST_INSERT_HEAD(modelp, dm, link);
			}
		}
	}

	/*
	 * This seems to prevent bordered screens.
	 */
	for (i=0; i < 16; i++)
		VIDEO->vd_fal_rgb[i] = CM_L2FAL(gra_def_color16[i]);
}

static struct videl *
falcon_getreg(mode)
u_short mode;
{
	int i;
	struct videl *vregs;
	
	for (i = 0; (vregs = &videlinit[i])->video_mode != 0xffff; i++)
		if ((vregs->video_mode) == mode)
			return vregs;

	return NULL; /* mode not found */
}

static void
falcon_detect(dm)
dmode_t *dm;
{
	u_short	falshift, stshift;
	struct videl *vregs = vm_regs(dm);

	/*
	 * First get the videl register values
	 */

	vregs->vd_syncmode	= VIDEO->vd_sync;
	vregs->vd_line_wide	= VIDEO->vd_line_wide;
	vregs->vd_vert_wrap	= VIDEO->vd_vert_wrap;
	vregs->vd_st_res	= VIDEO->vd_st_res;
	vregs->vd_fal_res	= VIDEO->vd_fal_res;
	vregs->vd_h_hold_tim	= VIDEO->vd_h_hold_tim;
	vregs->vd_h_bord_beg	= VIDEO->vd_h_bord_beg;
	vregs->vd_h_bord_end	= VIDEO->vd_h_bord_end;
	vregs->vd_h_dis_beg	= VIDEO->vd_h_dis_beg;
	vregs->vd_h_dis_end	= VIDEO->vd_h_dis_end;
	vregs->vd_h_ss		= VIDEO->vd_h_ss;
	vregs->vd_h_fs		= VIDEO->vd_h_fs;
	vregs->vd_h_hh		= VIDEO->vd_h_hh;
	vregs->vd_v_freq_tim	= VIDEO->vd_v_freq_tim;
	vregs->vd_v_bord_beg	= VIDEO->vd_v_bord_beg;
	vregs->vd_v_bord_end	= VIDEO->vd_v_bord_end;
	vregs->vd_v_dis_beg	= VIDEO->vd_v_dis_beg;
	vregs->vd_v_dis_end	= VIDEO->vd_v_dis_end;
	vregs->vd_v_ss		= VIDEO->vd_v_ss;
	vregs->vd_fal_ctrl	= VIDEO->vd_fal_ctrl;
	vregs->vd_fal_mode	= VIDEO->vd_fal_mode;
	

	/*
	 * Calculate the depth of the screen
	 */

	falshift = vregs->vd_fal_res;
	stshift = vregs->vd_st_res;

	if (falshift & 0x400)		/* 2 color */
		dm->depth = 1;
	else if (falshift & 0x100)	/* high color, direct */
		dm->depth = 16;
	else if (falshift & 0x10)	/* 256 color */
		dm->depth = 8;
	else if (stshift == 0)		/* 16 color */
		dm->depth = 4;
	else if (stshift == 1)		/* 4 color */
		dm->depth = 2;
	else dm->depth = 1;		/* 2 color */

	/*
	 * Now calculate the screen hight
	 */

	dm->size.height = vregs->vd_v_dis_end - vregs->vd_v_dis_beg;
	if (!((vregs->vd_fal_mode & 0x2) >> 1)) /* if not interlaced */
		dm->size.height >>=1;
	if (vregs->vd_fal_mode & 0x1)		/* if doublescan */
		dm->size.height >>=1;

	/*
	 * And the width
	 */

	dm->size.width = vregs->vd_vert_wrap * 16 / dm->depth;

}

u_long	falcon_needs_vbl;
void falcon_display_switch __P((void));

static void
falcon_display_view(v)
view_t *v;
{
	dmode_t	*dm = v->mode;
	bmap_t		*bm;
	struct videl	*vregs;
	static u_short last_mode = 0xffff;

	if (dm->current_view) {
		/*
		 * Mark current view for this mode as no longer displayed
		 */
		dm->current_view->flags &= ~VF_DISPLAY;
	}
	dm->current_view = v;
	v->flags |= VF_DISPLAY;
	vregs = vm_regs(v->mode);

	falcon_use_colormap(v, v->colormap);

	bm = v->bitmap;
	VIDEO->vd_ramh   = ((u_long)bm->hw_address >> 16) & 0xff;
	VIDEO->vd_ramm   = ((u_long)bm->hw_address >>  8) & 0xff;
	VIDEO->vd_raml   =  (u_long)bm->hw_address & 0xff;

	if (last_mode != vregs->video_mode) {
		last_mode = vregs->video_mode;

		if (dm->depth == 1) {
			/*
			 * Set the resolution registers to a mode, which guarantee
			 * no shifting when the register are written during vbl.
			 */
			VIDEO->vd_fal_res = 0;
			VIDEO->vd_st_res = 0;
		}

		/*
		 * Arrange for them to be activated
		 * at the second vbl interrupt.
		 */
		falcon_needs_vbl = (u_long)v;
	}
}

void
falcon_display_switch()
{
	view_t		*v;
	struct videl	*vregs;
	static int vbl_count = 1;

	if(vbl_count--) return;

	v = (view_t*)falcon_needs_vbl;

	vbl_count = 1;
	falcon_needs_vbl = 0;

	/*
	 * Write to videl registers only on VGA displays
	 * This is only a hack. Must be fixed soon. XXX -- Thomas
	 */
	if(mon_type != FAL_VGA) return;

	vregs = vm_regs(v->mode);

	VIDEO->vd_v_freq_tim    = vregs->vd_v_freq_tim;
	VIDEO->vd_v_ss          = vregs->vd_v_ss;
	VIDEO->vd_v_bord_beg    = vregs->vd_v_bord_beg;
	VIDEO->vd_v_bord_end    = vregs->vd_v_bord_end;
	VIDEO->vd_v_dis_beg     = vregs->vd_v_dis_beg;
	VIDEO->vd_v_dis_end     = vregs->vd_v_dis_end;
	VIDEO->vd_h_hold_tim    = vregs->vd_h_hold_tim;
	VIDEO->vd_h_ss          = vregs->vd_h_ss;
	VIDEO->vd_h_bord_beg    = vregs->vd_h_bord_beg;
	VIDEO->vd_h_bord_end    = vregs->vd_h_bord_end;
	VIDEO->vd_h_dis_beg     = vregs->vd_h_dis_beg;
	VIDEO->vd_h_dis_end     = vregs->vd_h_dis_end;
#if 0 /* This seems not to be necessary -- Thomas */
	VIDEO->vd_h_fs		= vregs->vd_h_fs;
	VIDEO->vd_h_hh		= vregs->vd_h_hh;
#endif
	VIDEO->vd_sync          = vregs->vd_syncmode;
	VIDEO->vd_fal_res       = 0;
	if (v->mode->depth == 2)
		VIDEO->vd_st_res        = vregs->vd_st_res;
	else {
		VIDEO->vd_st_res        = 0;
		VIDEO->vd_fal_res       = vregs->vd_fal_res;
	}
	VIDEO->vd_vert_wrap     = vregs->vd_vert_wrap;
	VIDEO->vd_line_wide     = vregs->vd_line_wide;
	VIDEO->vd_fal_ctrl      = vregs->vd_fal_ctrl;
	VIDEO->vd_fal_mode      = vregs->vd_fal_mode;
}

static void
falcon_remove_view(v)
view_t *v;
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
falcon_save_view(v)
view_t *v;
{
}

static void
falcon_free_view(v)
view_t *v;
{
	if (v) {
		falcon_remove_view(v);
		if (v->colormap != &gra_con_cmap)
			free(v->colormap, M_DEVBUF);
		free_bitmap(v->bitmap);
		if (v != &gra_con_view)
			free(v, M_DEVBUF);
	}
}

static int
falcon_use_colormap(v, cm)
view_t		*v;
colormap_t	*cm;
{
	dmode_t			*dm;
	volatile u_short	*creg;
	volatile u_long		*fcreg;
	u_long			*src;
	colormap_t		*vcm;
	u_long			*vcreg;
	u_short			ncreg;
	int			last_streg;
	int			i;

	dm  = v->mode;
	vcm = v->colormap;

	/*
	 * I guess it seems reasonable to require the maps to be
	 * of the same type...
	 */
	if (cm->type != vcm->type)
		return (EINVAL);

	/*
	 * First get the colormap addresses an calculate
	 * howmany colors are in it.
	 */
	if (dm->depth == 16) /* direct color, no colormap;
				but also not (yet) supported */
		return(0);
	fcreg = &VIDEO->vd_fal_rgb[0];
	creg  = &VIDEO->vd_st_rgb[0];
	ncreg = 1 << dm->depth;

	/* If first entry specified beyond capabilities -> error */
	if (cm->first >= ncreg)
		return (EINVAL);

	/*
	 * A little tricky, the actual colormap pointer will be NULL
	 * when view is not displaying, valid otherwise.
	 */
	if (v->flags & VF_DISPLAY) {
		creg = &creg[cm->first];
		fcreg = &fcreg[cm->first];
	} else {
		creg = NULL;
		fcreg = NULL;
	}

	vcreg  = &vcm->entry[cm->first];
	ncreg -= cm->first;
	last_streg = 16 - cm->first;
	if (cm->size > ncreg)
		return (EINVAL);
	ncreg = cm->size;

	for (i = 0, src = cm->entry; i < ncreg; i++, vcreg++) {
		*vcreg = *src++;

		/*
		 * If displaying, also update actual color register.
		 */
		if (fcreg != NULL) {
			*fcreg++ = CM_L2FAL(*vcreg);
			if (i < last_streg)
				*creg++ = CM_L2ST(*vcreg);
		}
	}
	return (0);
}

static view_t *
falcon_alloc_view(mode, dim, depth)
dmode_t	*mode;
dimen_t	*dim;
u_char   depth;
{
	view_t *v;
	bmap_t *bm;

	if (!atari_realconfig)
		v = &gra_con_view;
	else v = malloc(sizeof(*v), M_DEVBUF, M_NOWAIT);
	if (v == NULL)
		return(NULL);
	
	bm = alloc_bitmap(mode->size.width, mode->size.height, mode->depth);
	if (bm) {
		box_t   box;

		v->colormap = alloc_colormap(mode);
		if (v->colormap) {
			INIT_BOX(&box,0,0,mode->size.width,mode->size.height);
			init_view(v, bm, mode, &box);
			return(v);
		}
		free_bitmap(bm);
	}
	if (v != &gra_con_view)
		free(v, M_DEVBUF);
	return (NULL);
}

static void
init_view(v, bm, mode, dbox)
view_t	*v;
bmap_t	*bm;
dmode_t	*mode;
box_t	*dbox;
{
	v->bitmap = bm;
	v->mode   = mode;
	v->flags  = 0;
	bcopy(dbox, &v->display, sizeof(box_t));
}

/* bitmap functions */

static bmap_t *
alloc_bitmap(width, height, depth)
u_long	width, height;
u_char	depth;
{
	u_long  total_size, bm_size;
	void	*hw_address;
	bmap_t	*bm;

	/*
	 * Sigh, it seems for mapping to work we need the bitplane data to
	 *  1: be aligned on a page boundry.
	 *  2: be n pages large.
	 * 
	 * why? because the user gets a page aligned address, if this is before
	 * your allocation, too bad.  Also it seems that the mapping routines
	 * do not watch to closely to the allowable length. so if you go over
	 * n pages by less than another page, the user gets to write all over
	 * the entire page. Since you did not allocate up to a page boundry
	 * (or more) the user writes into someone elses memory. -ch
	 */
	bm_size    = m68k_round_page((width * height * depth) / NBBY);
	total_size = bm_size + sizeof(bmap_t) + PAGE_SIZE;

	if ((bm = (bmap_t*)alloc_stmem(total_size, &hw_address)) == NULL)
		return(NULL);

	bm->plane         = (u_char*)bm + sizeof(bmap_t);
	bm->plane         = (u_char*)m68k_round_page(bm->plane);
	bm->hw_address    = (u_char*)hw_address + sizeof(bmap_t);
	bm->hw_address    = (u_char*)m68k_round_page(bm->hw_address);
	bm->bytes_per_row = (width * depth) / NBBY;
	bm->rows          = height;
	bm->depth         = depth;
	bm->phys_mappable = (depth * width * height) / NBBY;
	bm->regs          = NULL;
	bm->hw_regs       = NULL;
	bm->reg_size      = 0;
	bm->vga_address   = NULL;
	bm->vga_mappable  = 0;
	bm->lin_base      = 0;
	bm->vga_base      = 0;

	bzero(bm->plane, bm_size);
	return (bm);
}

static void
free_bitmap(bm)
bmap_t *bm;
{
	if (bm)
		free_stmem(bm);
}

static colormap_t *
alloc_colormap(dm)
dmode_t		*dm;
{
	int		nentries, i;
	colormap_t	*cm;
	u_char		type = CM_COLOR;

	if (dm->depth == 16) /* direct color, no colormap;
				not (yet) supported */
		nentries = 0;
	else
		nentries = 1 << dm->depth;

	if (!atari_realconfig) {
		cm = &gra_con_cmap;
		cm->entry = gra_con_colors;
	}
	else {
		int size;

		size = sizeof(*cm) + (nentries * sizeof(cm->entry[0]));
		cm   = malloc(size, M_DEVBUF, M_NOWAIT);
		if (cm == NULL)
			return(NULL);
		cm->entry = (long *)&cm[1];

	}

	if ((cm->type = type) == CM_COLOR)
		cm->red_mask = cm->green_mask = cm->blue_mask = 0x3f;

	cm->first = 0;
	cm->size  = nentries;

	for (i = 0; i < nentries; i++)
		cm->entry[i] = gra_def_color16[i % 16];
	return (cm);
}
#endif /* FALCON_VIDEO */
