/*	$NetBSD: grfabs.c,v 1.1.1.1 1995/03/26 07:12:15 leo Exp $	*/

/*
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

/*
 *  atari abstract graphics driver.
 */
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>

#include <machine/iomap.h>
#include <machine/video.h>
#include <machine/mfp.h>
#include <atari/dev/grfabs_reg.h>

/*
 * Function decls
 */
static dmode_t    *get_best_display_mode __P((int, int, int));
static view_t     *alloc_view __P((dmode_t *, dimen_t *, u_char));
static void       init_view __P((view_t *, bmap_t *, dmode_t *, box_t *));
static bmap_t	  *alloc_bitmap __P((u_long, u_long, u_char, int));
static void 	  free_bitmap __P((bmap_t *));
static colormap_t *alloc_colormap __P((dmode_t *));

/*
 * Ugh.. Stuff needed to allocate console structures before the VM-system
 * is running. There is no malloc() available at that time.
 */
extern int		atari_realconfig;	/* 0 -> no malloc	*/
static view_t		con_view;
static colormap_t	con_cmap;
static u_short		con_colors[MAX_CENTRIES];

/*
 * List of available graphic modes
 */
static LIST_HEAD(modelist, display_mode) modes;

static dmode_t vid_modes[] = {
	{ { NULL, NULL }, "stlow",  { 320,  200 },  1, RES_STLOW  },
	{ { NULL, NULL }, "stmid",  { 640,  200 },  2, RES_STMID  },
	{ { NULL, NULL }, "sthigh", { 640,  400 },  1, RES_STHIGH },
	{ { NULL, NULL }, "ttlow",  { 320,  480 },  8, RES_TTLOW  },
	{ { NULL, NULL }, "ttmid",  { 640,  480 },  4, RES_TTMID  },
	{ { NULL, NULL }, "tthigh", { 1280, 960 },  1, RES_TTHIGH },
	{ { NULL, NULL }, NULL,  }
};

/*
 * Default colors.....
 * Currently the TT-low (256 colors) just uses 16 time the 16-color default.
 * If you have a sensible 256 scale, feel free to add.....
 * The first 2 colors in all maps are {black,white}, so ite (text) displays
 * are initially readable. Also, this enables me to supply only 1 map. The
 * 4 color mode uses the first four entries of the 16 color mode thus giving
 * a gray scale display. (Maybe we can add an intensity bit to the ite...)
 */
static u_short def_color16[16] = {
	0x000,	/* black		*/
	0xfff,	/* white		*/
	0xccc,	/* light gray		*/
	0x888,	/* gray			*/
	0x00c,	/* blue			*/
	0x0c0,	/* green		*/
	0x0cc,	/* cyan			*/
	0xc00,	/* red			*/
	0xc0c,	/* magenta		*/
	0xcc0,	/* brown		*/
	0x00f,	/* light blue		*/
	0x0f0,	/* light green		*/
	0x0ff,	/* light cyan		*/
	0xf00,	/* light red		*/
	0xf0f,	/* light magenta	*/
	0xff0	/* light brown		*/
};

/*
 * XXX: called from ite console init routine.
 * Initialize list of posible video modes.
 */
int
grfabs_probe()
{
		dmode_t	*dm;
		int	i;
		int	has_mono;
	static  int	inited = 0;

	if(inited)
		return;	/* Has to be done only once */
	inited++;

	/*
	 * First find out what kind of monitor is attached. Dma-sound
	 * should be off because the 'sound-done' and 'monochrome-detect'
	 * are xor-ed together. I think that shutting it down here is the
	 * wrong place.
	 */
	has_mono = (MFP->mf_gpip & IA_MONO) == 0;

	LIST_INIT(&modes);
	for(i = 0; (dm = &vid_modes[i])->name != NULL; i++) {
		if(has_mono && (dm->vm_reg != RES_TTHIGH))
			continue;
		if(!has_mono && (dm->vm_reg == RES_TTHIGH))
			continue;
		LIST_INSERT_HEAD(&modes, dm, link);
	}
	return(1);
}

view_t *
grf_alloc_view(d, dim, depth)
dmode_t	*d;
dimen_t	*dim;
u_char	depth;
{
	if(!d)
		d = get_best_display_mode(dim->width, dim->height, depth);
	if(d) 
		return(alloc_view(d, dim, depth));
	return(NULL);
}

void grf_display_view(v)
view_t *v;
{
	dmode_t	*dm = v->mode;
	bmap_t	*bm = v->bitmap;

	if(dm->current_view) {
		/*
		 * Mark current view for this mode as no longer displayed
		 */
		dm->current_view->flags &= ~VF_DISPLAY;
	}
	dm->current_view = v;
	v->flags |= VF_DISPLAY;

	grf_use_colormap(v, v->colormap);

	/* XXX: should use vbl for this	*/
	VIDEO->vd_tt_res = dm->vm_reg;
	VIDEO->vd_raml   =  (u_long)bm->hw_address & 0xff;
	VIDEO->vd_ramm   = ((u_long)bm->hw_address >>  8) & 0xff;
	VIDEO->vd_ramh   = ((u_long)bm->hw_address >> 16) & 0xff;
}

void grf_remove_view(v)
view_t *v;
{
	dmode_t *mode = v->mode;

	if(mode->current_view == v) {
#if 0
		if(v->flags & VF_DISPLAY)
			panic("Cannot shutdown display\n"); /* XXX */
#endif
		mode->current_view = NULL;
	}
	v->flags &= ~VF_DISPLAY;
}

void grf_free_view(v)
view_t *v;
{
	if(v) {
		dmode_t *md = v->mode;

		grf_remove_view(v);
		if(v->colormap != &con_cmap)
			free(v->colormap, M_DEVBUF);
		free_bitmap(v->bitmap);
		if(v != &con_view)
			free(v, M_DEVBUF);
	}
}

int
grf_get_colormap(v, cm)
view_t		*v;
colormap_t	*cm;
{
	colormap_t	*gcm;
	int		i, n;

	bzero(cm->centry, cm->nentries * sizeof(u_short));

	gcm = v->colormap;
	n   = cm->nentries < gcm->nentries ? cm->nentries : gcm->nentries;
	for(i = 0; i < n; i++)
		cm->centry[i] = gcm->centry[i];
	return(0);
}

int
grf_use_colormap(v, cm)
view_t		*v;
colormap_t	*cm;
{
	dmode_t			*dm;
	volatile u_short	*creg;
	u_short			*src;
	u_short			ncreg;
	int			i;

	dm = v->mode;

	switch(dm->vm_reg) {
		case RES_STLOW:
			creg  = &VIDEO->vd_tt_rgb[0];
			ncreg = 16;
		break;
		case RES_STMID:
			creg  = &VIDEO->vd_tt_rgb[0];
			ncreg = 4;
			break;
		case RES_STHIGH:
			creg  = &VIDEO->vd_tt_rgb[254];
			ncreg = 2;
			break;
		case RES_TTLOW:
			creg  = &VIDEO->vd_tt_rgb[0];
			ncreg = 256;
			break;
		case RES_TTMID:
			creg  = &VIDEO->vd_tt_rgb[0];
			ncreg = 16;
			break;
		case RES_TTHIGH:
			return(0);	/* No colors	*/
		default:
			panic("grf_get_colormap: wrong mode!?");
	}
	if(cm->nentries < ncreg)
		ncreg = cm->nentries;

	for(i = 0, src = cm->centry; i < ncreg; i++)
		*creg++ = *src++;
	return(0);
}

static dmode_t *
get_best_display_mode(width, height, depth)
int	width, height, depth;
{
	dmode_t		*save;
	dmode_t		*dm;
	long   		dt, dx, dy, ct;

	save = NULL;
	dm = modes.lh_first;
	while(dm != NULL) {
		if(depth > dm->depth) {
			dm = dm->link.le_next;
			continue;
		}
		else if(width > dm->size.width || height > dm->size.height) {
			dm = dm->link.le_next;
			continue;
		}
		else if (width < dm->size.width || height < dm->size.height) {
			dm = dm->link.le_next;
			continue;
		}
		dx = abs(dm->size.width  - width);
		dy = abs(dm->size.height - height);
		ct = dx + dy;

		if (ct < dt || save == NULL) {
			save = dm;
			dt = ct;
		}
		dm = dm->link.le_next;
	}
	return (save);
}

static view_t *
alloc_view(mode, dim, depth)
dmode_t	*mode;
dimen_t	*dim;
u_char   depth;
{
	view_t *v;
	bmap_t *bm;

	if(!atari_realconfig)
		v = &con_view;
	else v = malloc(sizeof(*v), M_DEVBUF, M_WAITOK);

	bm = alloc_bitmap(mode->size.width, mode->size.height, mode->depth, 1);
	if(bm) {
		int     i;
		box_t   box;

		v->colormap = alloc_colormap(mode);
		if(v->colormap) {
			INIT_BOX(&box,0,0,mode->size.width,mode->size.height);
			init_view(v, bm, mode, &box);
			return(v);
		}
		free_bitmap(bm);
	}
	if(v != &con_view)
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
	bcopy(dbox, &v->display, sizeof(box_t));
}

/* bitmap functions */

static bmap_t *
alloc_bitmap(width, height, depth, clear)
u_long	width, height;
u_char	depth;
int	clear;
{
	int     i;
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
	bm_size    = atari_round_page((width * height * depth) / NBBY);
	total_size = bm_size + sizeof(bmap_t) + NBPG;

	if((bm = (bmap_t*)alloc_stmem(total_size, &hw_address)) == NULL)
		return(NULL);

	bm->plane         = (u_char*)bm + sizeof(bmap_t);
	bm->plane         = (u_char*)atari_round_page(bm->plane);
	bm->hw_address    = (u_char*)hw_address + sizeof(bmap_t);
	bm->hw_address    = (u_char*)atari_round_page(bm->hw_address);
	bm->bytes_per_row = (width * depth) / NBBY;
	bm->rows          = height;
	bm->depth         = depth;

	if(clear)
		bzero(bm->plane, bm_size);
	return(bm);
}

static void
free_bitmap(bm)
bmap_t *bm;
{
	if(bm)
		free_stmem(bm);
}

static colormap_t *
alloc_colormap(dm)
dmode_t		*dm;
{
	int		nentries, i;
	colormap_t	*cm;

	switch(dm->vm_reg) {
		case RES_STLOW:
		case RES_TTMID:
			nentries = 16;
			break;
		case RES_STMID:
			nentries = 4;
			break;
		case RES_STHIGH:
			nentries = 2;
			break;
		case RES_TTLOW:
			nentries = 256;
			break;
		case RES_TTHIGH:
			nentries = 0;
		default:
			panic("grf:alloc_colormap: wrong mode!?");
	}
	if(!atari_realconfig) {
		cm = &con_cmap;
		cm->centry = con_colors;
	}
	else {
		int size;

		size = sizeof(*cm) + (nentries * sizeof(u_short));
		cm   = malloc(size, M_DEVBUF, M_WAITOK);
		if(cm == NULL)
			return(NULL);
		cm->centry = (u_short *)&cm[1];

	}
	cm->nentries = nentries;

	for(i = 0; i < nentries; i++)
		cm->centry[i] = def_color16[i % 16];
	return(cm);
}
