/*	$NetBSD: ite_cc.c,v 1.17 2002/03/17 19:40:35 atatat Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/termios.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <atari/atari/device.h>
#include <atari/dev/itevar.h>
#include <atari/dev/iteioctl.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grfvar.h>
#include <atari/dev/font.h>
#include <atari/dev/viewioctl.h>
#include <atari/dev/viewvar.h>

#include "grfcc.h"

/*
 * This is what ip->priv points to;
 * it contains local variables for custom-chip ites.
 */
struct ite_priv {
	u_char	**row_ptr;	/* array of pointers into the bitmap	*/
	u_long	row_bytes;
	u_long	cursor_opt;
	u_short	*column_offset;	/* array of offsets for columns		*/
	u_int	row_offset;	/* the row offset			*/
	u_short	width;		/* the bitmap width			*/
	u_short	underline;	/* where the underline goes		*/
	u_short	ft_x;		/* the font width			*/
	u_short	ft_y;		/* the font height			*/
	u_char	*font_cell[256];/* the font pointer			*/
};
typedef struct ite_priv ipriv_t;

/*
 * We need the following space to get an ite-console setup before
 * the VM-system is brought up. We setup for a 1280x960 monitor with
 * an 8x8 font.
 */
#define	CONS_MAXROW	120	/* Max. number of rows on console	*/
#define	CONS_MAXCOL	160	/* Max. number of columns on console	*/
static u_short	con_columns[CONS_MAXCOL];
static u_char	*con_rows[CONS_MAXROW];
static ipriv_t	con_ipriv;

extern font_info	font_info_8x8;
extern font_info	font_info_8x16;

static void view_init __P((struct ite_softc *));
static void view_deinit __P((struct ite_softc *));
static int  itecc_ioctl __P((struct ite_softc *, u_long, caddr_t, int,
							struct proc *));
static int  ite_newsize __P((struct ite_softc *, struct itewinsize *));
static void cursor32 __P((struct ite_softc *, int));
static void putc8 __P((struct ite_softc *, int, int, int, int));
static void clear8 __P((struct ite_softc *, int, int, int, int));
static void scroll8 __P((struct ite_softc *, int, int, int, int));
static void scrollbmap __P((bmap_t *, u_short, u_short, u_short, u_short,
							short, short));

/*
 * grfcc config stuff
 */
void grfccattach __P((struct device *, struct device *, void *));
int  grfccmatch __P((struct device *, struct cfdata *, void *));
int  grfccprint __P((void *, const char *));

struct cfattach grfcc_ca = {
	sizeof(struct grf_softc), grfccmatch, grfccattach
};

/*
 * only used in console init.
 */
static struct cfdata *cfdata_grf   = NULL;

/*
 * Probe functions we can use:
 */
#ifdef TT_VIDEO
void	tt_probe_video __P((MODES *));
#endif /* TT_VIDEO */
#ifdef FALCON_VIDEO
void	falcon_probe_video __P((MODES *));
#endif /* FALCON_VIDEO */

int
grfccmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	static int	did_consinit = 0;
	static int	must_probe = 1;
	grf_auxp_t	*grf_auxp = auxp;

	if (must_probe) {
		/*
		 * Check if the layers we depend on exist
		 */
		if (!(machineid & (ATARI_TT|ATARI_FALCON)))
			return (0);
#ifdef TT_VIDEO
		if ((machineid & ATARI_TT) && !grfabs_probe(&tt_probe_video))
			return (0);
#endif /* TT_VIDEO */
#ifdef FALCON_VIDEO
		if ((machineid & ATARI_FALCON)
		    && !grfabs_probe(&falcon_probe_video))
			return (0);
#endif /* FALCON_VIDEO */

		viewprobe();
		must_probe = 0;
	}

	if (atari_realconfig == 0) {
		/*
		 * Early console init. Only match first unit.
		 */
		if (did_consinit)
			return(0);
		if (viewopen(cfp->cf_unit, 0, 0, NULL))
			return(0);
		cfdata_grf = cfp;
		did_consinit = 1;
		return (1);
	}

	/*
	 * Normal config. When we are called directly from the grfbus,
	 * we only match the first unit. The attach function will call us for
	 * the other configured units.
	 */
	if (grf_auxp->from_bus_match && (did_consinit > 1))
		return (0);

	if (!grf_auxp->from_bus_match && (grf_auxp->unit != cfp->cf_unit))
		return (0);

	/*
	 * Final constraint: each grf needs a view....
	 */
	if((cfdata_grf == NULL) || (did_consinit > 1)) {
	    if(viewopen(cfp->cf_unit, 0, 0, NULL))
		return(0);
	}
	did_consinit = 2;
	return(1);
}

/*
 * attach: initialize the grf-structure and try to attach an ite to us.
 * note  : dp is NULL during early console init.
 */
void
grfccattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	static struct grf_softc		congrf;
	static int			first_attach = 1;
	       grf_auxp_t		*grf_bus_auxp = auxp;
	       grf_auxp_t		grf_auxp;
	       struct grf_softc		*gp;
	       int			maj;

	/*
	 * find our major device number 
	 */
	for(maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == grfopen)
			break;

	/*
	 * Handle exeption case: early console init
	 */
	if(dp == NULL) {
		congrf.g_unit    = cfdata_grf->cf_unit;
		congrf.g_grfdev  = makedev(maj, congrf.g_unit);
		congrf.g_itedev  = (dev_t)-1;
		congrf.g_flags   = GF_ALIVE;
		congrf.g_mode    = grf_mode;
		congrf.g_conpri  = grfcc_cnprobe();
		congrf.g_viewdev = congrf.g_unit;
		grfcc_iteinit(&congrf);
		grf_viewsync(&congrf);

		/* Attach console ite */
		atari_config_found(cfdata_grf, NULL, &congrf, grfccprint);
		return;
	}

	gp = (struct grf_softc *)dp;
	gp->g_unit = gp->g_device.dv_unit;
	grfsp[gp->g_unit] = gp;

	if((cfdata_grf != NULL) && (gp->g_unit == congrf.g_unit)) {
		/*
		 * We inited earlier just copy the info, take care
		 * not to copy the device struct though.
		 */
		bcopy(&congrf.g_display, &gp->g_display,
			(char *)&gp[1] - (char *)&gp->g_display);
	}
	else {
		gp->g_grfdev  = makedev(maj, gp->g_unit);
		gp->g_itedev  = (dev_t)-1;
		gp->g_flags   = GF_ALIVE;
		gp->g_mode    = grf_mode;
		gp->g_conpri  = 0;
		gp->g_viewdev = gp->g_unit;
		grfcc_iteinit(gp);
		grf_viewsync(gp);
	}

	printf(": width %d height %d", gp->g_display.gd_dwidth,
		    gp->g_display.gd_dheight);
	if(gp->g_display.gd_colors == 2)
		printf(" monochrome\n");
	else printf(" colors %d\n", gp->g_display.gd_colors);
	
	/*
	 * try and attach an ite
	 */
	config_found(dp, gp, grfccprint);

	/*
	 * If attaching the first unit, go ahead and 'find' the rest of us
	 */
	if (first_attach) {
		first_attach = 0;
		grf_auxp.from_bus_match = 0;
		for (grf_auxp.unit=1; grf_auxp.unit < NGRFCC; grf_auxp.unit++) {
		    config_found(pdp, (void*)&grf_auxp, grf_bus_auxp->busprint);
		}
	}
}

int
grfccprint(auxp, pnp)
void		*auxp;
const char	*pnp;
{
	if(pnp)
		printf("ite at %s", pnp);
	return(UNCONF);
}

/*
 * called from grf_cc to return console priority
 */
int
grfcc_cnprobe()
{
	return(CN_INTERNAL);
}
/*
 * called from grf_cc to init ite portion of 
 * grf_softc struct
 */
void
grfcc_iteinit(gp)
struct grf_softc *gp;
{

	gp->g_itecursor = cursor32;
	gp->g_iteputc   = putc8;
	gp->g_iteclear  = clear8;
	gp->g_itescroll = scroll8;
	gp->g_iteinit   = view_init;
	gp->g_itedeinit = view_deinit;
}

static void
view_deinit(ip)
struct ite_softc	*ip;
{
	ip->flags &= ~ITE_INITED;
}

static void
view_init(ip)
register struct ite_softc *ip;
{
	struct itewinsize	wsz;
	ipriv_t			*cci;

	if((cci = ip->priv) != NULL)
		return;

	ip->itexx_ioctl = itecc_ioctl;

#if defined(KFONT_8X8)
	ip->font = font_info_8x8;
#else
	ip->font = font_info_8x16;
#endif

	/* Find the correct set of rendering routines for this font.  */
	if(ip->font.width != 8)
		panic("kernel font size not supported");

	if(!atari_realconfig)
		ip->priv = cci = &con_ipriv;
	else ip->priv = cci = (ipriv_t*)malloc(sizeof(*cci), M_DEVBUF,M_WAITOK);
	if(cci == NULL)
		panic("No memory for ite-view");
	bzero(cci, sizeof(*cci));

	cci->cursor_opt    = 0;
	cci->row_ptr       = NULL;
	cci->column_offset = NULL;

	wsz.x      = ite_default_x;
	wsz.y      = ite_default_y;
	wsz.width  = ite_default_width;
	wsz.height = ite_default_height;
	wsz.depth  = ite_default_depth;

	ite_newsize (ip, &wsz);

	/*
	 * Only console will be turned on by default..
	 */
	if(ip->flags & ITE_ISCONS)
		ip->grf->g_mode(ip->grf, GM_GRFON, NULL, 0, 0);
}

static int
ite_newsize(ip, winsz)
struct ite_softc	*ip;
struct itewinsize	*winsz;
{
	struct view_size	vs;
	ipriv_t			*cci = ip->priv;    
	u_long			i, j;
	int			error = 0;
	view_t			*view;

	vs.x      = winsz->x;
	vs.y      = winsz->y;
	vs.width  = winsz->width;
	vs.height = winsz->height;
	vs.depth  = winsz->depth;

	error = viewioctl(ip->grf->g_viewdev, VIOCSSIZE, (caddr_t)&vs, 0,
								NOPROC);
	view  = viewview(ip->grf->g_viewdev);

	/*
	 * Reinitialize our structs
	 */
	ip->cols = view->display.width  / ip->font.width; 
	ip->rows = view->display.height / ip->font.height;

	/*
	 * save new values so that future opens use them
	 * this may not be correct when we implement Virtual Consoles
	 */
	ite_default_height = view->display.height;
	ite_default_width  = view->display.width;
	ite_default_x      = view->display.x;
	ite_default_y      = view->display.y;
	ite_default_depth  = view->bitmap->depth;

	if(cci->row_ptr && (cci->row_ptr != con_rows)) {
		free(cci->row_ptr, M_DEVBUF);
		cci->row_ptr = NULL;
	}
	if(cci->column_offset && (cci->column_offset != con_columns)) {
		free(cci->column_offset, M_DEVBUF);
		cci->column_offset = NULL;
	}

	if(!atari_realconfig) {
		cci->row_ptr       = con_rows;
		cci->column_offset = con_columns;
	}
	else {
	  cci->row_ptr = malloc(sizeof(u_char *) * ip->rows,M_DEVBUF,M_NOWAIT);
	  cci->column_offset = malloc(sizeof(u_int)*ip->cols,M_DEVBUF,M_NOWAIT);
	}

	if(!cci->row_ptr || !cci->column_offset)
		panic("No memory for ite-view");

	cci->width      = view->bitmap->bytes_per_row << 3;
	cci->underline  = ip->font.baseline + 1;
	cci->row_offset = view->bitmap->bytes_per_row;
	cci->ft_x       = ip->font.width;
	cci->ft_y       = ip->font.height;
	cci->row_bytes  = cci->row_offset * cci->ft_y;
	cci->row_ptr[0] = view->bitmap->plane;
	for(i = 1; i < ip->rows; i++) 
		cci->row_ptr[i] = cci->row_ptr[i-1] + cci->row_bytes;

	/*
	 * Initialize the column offsets to point at the correct location
	 * in the first plane. This definitely assumes a font width of 8!
	 */
	j = view->bitmap->depth * 2;
	cci->column_offset[0] = 0;
	for(i = 1; i < ip->cols; i++) 
		cci->column_offset[i] = ((i >> 1) * j) + (i & 1);

	/* initialize the font cell pointers */
	cci->font_cell[ip->font.font_lo] = ip->font.font_p;
	for(i = ip->font.font_lo+1; i <= ip->font.font_hi; i++)
		cci->font_cell[i] = cci->font_cell[i-1] + ip->font.height;
	    
	return(error);
}

int
itecc_ioctl(ip, cmd, addr, flag, p)
struct ite_softc	*ip;
u_long			cmd;
caddr_t			addr;
int			flag;
struct proc		*p;
{
	struct winsize		ws;
	struct itewinsize	*is;
	int			error = 0;
	view_t			*view = viewview(ip->grf->g_viewdev);

	switch (cmd) {
	case ITEIOCSWINSZ:
		is = (struct itewinsize *)addr;

		if(ite_newsize(ip, is))
			error = ENOMEM;
		else {
			view         = viewview(ip->grf->g_viewdev);
			ws.ws_row    = ip->rows;
			ws.ws_col    = ip->cols;
			ws.ws_xpixel = view->display.width;
			ws.ws_ypixel = view->display.height;
			ite_reset(ip);
			/*
			 * XXX tell tty about the change 
			 * XXX this is messy, but works 
			 */
			iteioctl(ip->grf->g_itedev,TIOCSWINSZ,(caddr_t)&ws,0,p);
		}
		break;
	case VIOCSCMAP:
	case VIOCGCMAP:
		/*
		 * XXX watchout for that NOPROC. its not really the kernel
		 * XXX talking these two commands don't use the proc pointer
		 * XXX though.
		 */
		error = viewioctl(ip->grf->g_viewdev, cmd, addr, flag, NOPROC);
		break;
	default:
		error = EPASSTHROUGH;
		break;
	}
	return (error);
}

static void
cursor32(struct ite_softc *ip, int flag)
{
	int	cend;
	u_char	*pl;
	ipriv_t	*cci;

	cci = ip->priv;

	if(flag == END_CURSOROPT)
		cci->cursor_opt--;
	else if(flag == START_CURSOROPT) {
			if(!cci->cursor_opt)
				cursor32(ip, ERASE_CURSOR);
			cci->cursor_opt++;
			return;		  /* if we are already opted. */
	}
    
	if(cci->cursor_opt) 
		return;		  /* if we are still nested. */
				  /* else we draw the cursor. */
    
	if(flag != DRAW_CURSOR && flag != END_CURSOROPT) {
		/*
		 * erase the cursor
		 */
		cend = ip->font.height-1; 
		pl   = cci->column_offset[ip->cursorx]
				+ cci->row_ptr[ip->cursory];
		__asm__ __volatile__
			("1: notb  %0@ ; addaw %4,%0\n\t"
			 "dbra  %1,1b"
			 : "=a" (pl), "=d" (cend)
			 : "0" (pl), "1" (cend),
			 "d" (cci->row_offset)
			 );
	}

	if(flag != DRAW_CURSOR && flag != MOVE_CURSOR && flag != END_CURSOROPT)
		return;
	
	/* 
	 * draw the cursor
	 */
	cend = min(ip->curx, ip->cols-1);
	if (flag == DRAW_CURSOR
		&& ip->cursorx == cend && ip->cursory == ip->cury)
		return;
	ip->cursorx = cend;
	ip->cursory = ip->cury;
	cend        = ip->font.height-1; 
	pl          = cci->column_offset[ip->cursorx]
			+ cci->row_ptr[ip->cursory];

	__asm__ __volatile__
		("1: notb  %0@ ; addaw %4,%0\n\t"
		 "dbra  %1,1b"
		 : "=a" (pl), "=d" (cend)
		 : "0" (pl), "1" (cend),
		 "d" (cci->row_offset)
		 );
}

static void
putc8(struct ite_softc *ip, int c, int dy, int dx, int mode)
{
    register ipriv_t	*cci = (ipriv_t *)ip->priv;
    register u_char	*pl  = cci->column_offset[dx] + cci->row_ptr[dy];
    register u_int	fh   = cci->ft_y;
    register u_int	ro   = cci->row_offset;
    register u_char	eor_mask;
    register u_char	bl, tmp, ul;
    register u_char	*ft;

    if(c < ip->font.font_lo || c > ip->font.font_hi)
		return;

	ft = cci->font_cell[c];

	if(!mode) {
		while(fh--) {
			*pl = *ft++; pl += ro;
		}
		return;
	}

	eor_mask = (mode & ATTR_INV) ? 0xff : 0x00;
	bl       = (mode & ATTR_BOLD) ? 1 : 0;
	ul       = (mode & ATTR_UL) ? fh - cci->underline : fh;
	for(; fh--; pl += ro) {
		if(fh != ul) {
			tmp = *ft++;
			if(bl)
				tmp |= (tmp >> 1);
			*pl = tmp ^ eor_mask;
		}
		else {
			*pl = 0xff ^ eor_mask;
			ft++;
		}
	}
}

static void
clear8(struct ite_softc *ip, int sy, int sx, int h, int w)
{
	ipriv_t	*cci = (ipriv_t *) ip->priv;
	view_t	*v   = viewview(ip->grf->g_viewdev);
	bmap_t	*bm  = v->bitmap;

	if((sx == 0) && (w == ip->cols)) {
		/* common case: clearing whole lines */
		while (h--) {
			int		i;
			u_char	*ptr = cci->row_ptr[sy]; 
			for(i = 0; i < ip->font.height; i++) {
				bzero(ptr, bm->bytes_per_row);
				ptr += bm->bytes_per_row;
			}
			sy++;
		}
	}
	else {
		/*
		 * clearing only part of a line
		 * XXX could be optimized MUCH better, but is it worth the
		 * trouble?
		 */

		int		i;
		u_char  *pls, *ple;

		pls = cci->row_ptr[sy];
		ple = pls + cci->column_offset[sx + w-1];
		pls = pls + cci->column_offset[sx];

		for(i = ((ip->font.height) * h)-1; i >= 0; i--) {
			u_char *p = pls;
			while(p <= ple)
				*p++ = 0;
			pls += bm->bytes_per_row; 
			ple += bm->bytes_per_row; 
		}
	}
}

/* Note: sx is only relevant for SCROLL_LEFT or SCROLL_RIGHT.  */
static void
scroll8(ip, sy, sx, count, dir)
register struct ite_softc	*ip;
register int			sy;
int				dir, sx, count;
{
	bmap_t *bm = viewview(ip->grf->g_viewdev)->bitmap;
	u_char *pl = ((ipriv_t *)ip->priv)->row_ptr[sy];

	if(dir == SCROLL_UP) {
		int	dy = sy - count;

		cursor32(ip, ERASE_CURSOR);
		scrollbmap(bm, 0, dy*ip->font.height, bm->bytes_per_row >> 3,
				(ip->bottom_margin-dy+1)*ip->font.height,
				0, -(count*ip->font.height));
	}
	else if(dir == SCROLL_DOWN) {
        	cursor32(ip, ERASE_CURSOR);
		scrollbmap(bm, 0, sy*ip->font.height, bm->bytes_per_row >> 3,
				(ip->bottom_margin-sy+1)*ip->font.height,
				0, count*ip->font.height);
	}
	else if(dir == SCROLL_RIGHT) {
		int	sofs = (ip->cols - count) * ip->font.width;
		int	dofs = (ip->cols) * ip->font.width;
		int	i, j;

		cursor32(ip, ERASE_CURSOR);
		for(j = ip->font.height-1; j >= 0; j--) {
		    int sofs2 = sofs, dofs2 = dofs;
		    for (i = (ip->cols - (sx + count))-1; i >= 0; i--) {
			int	t;
			sofs2 -= ip->font.width;
			dofs2 -= ip->font.width;
			asm("bfextu %1@{%2:%3},%0" : "=d" (t)
				: "a" (pl), "d" (sofs2), "d" (ip->font.width));
			asm("bfins %3,%0@{%1:%2}" :
				: "a" (pl), "d" (dofs2), "d" (ip->font.width),
				  "d" (t));
		    }
			pl += bm->bytes_per_row; 
		}
	}
	else { /* SCROLL_LEFT */
		int sofs = (sx) * ip->font.width;
		int dofs = (sx - count) * ip->font.width;
		int i, j;

		cursor32(ip, ERASE_CURSOR);
		for(j = ip->font.height-1; j >= 0; j--) {
		    int sofs2 = sofs, dofs2 = dofs;
		    for(i = (ip->cols - sx)-1; i >= 0; i--) {
			int	t;

			asm("bfextu %1@{%2:%3},%0" : "=d" (t)
				: "a" (pl), "d" (sofs2), "d" (ip->font.width));
			asm("bfins %3,%0@{%1:%2}"
				: : "a" (pl), "d" (dofs2),"d" (ip->font.width),
				    "d" (t));
			sofs2 += ip->font.width;
			dofs2 += ip->font.width;
		    }
		    pl += bm->bytes_per_row; 
		}
    }		
}

static void 
scrollbmap (bmap_t *bm, u_short x, u_short y, u_short width, u_short height, short dx, short dy)
{
    u_short lwpr  = bm->bytes_per_row >> 2;

    if(dx) {
    	/* FIX: */ panic ("delta x not supported in scroll bitmap yet.");
    } 

    if(dy == 0) {
        return;
    }
    if(dy > 0) {
		u_long *pl       = (u_long *)bm->plane;
		u_long *src_y    = pl + (lwpr*y);
		u_long *dest_y   = pl + (lwpr*(y+dy));
		u_long count     = lwpr*(height-dy);
		u_long *clr_y    = src_y;
		u_long clr_count = dest_y - src_y;
		u_long bc, cbc;
		
		src_y  += count - 1;
		dest_y += count - 1;

		bc = count >> 4;
		count &= 0xf;
		
		while (bc--) {
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		    *dest_y-- = *src_y--; *dest_y-- = *src_y--;
		}
		while (count--)
		    *dest_y-- = *src_y--;

		cbc = clr_count >> 4;
		clr_count &= 0xf;

		while (cbc--) {
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		}
		while(clr_count--)
		    *clr_y++ = 0;
    }
	else {
		u_long	*pl       = (u_long *)bm->plane;
		u_long	*src_y    = pl + (lwpr*(y-dy));
		u_long	*dest_y   = pl + (lwpr*y); 
		long	count     = lwpr*(height + dy);
		u_long	*clr_y    = dest_y + count;
		u_long	clr_count = src_y - dest_y;
		u_long	bc, cbc;

		bc = count >> 4;
		count &= 0xf;
		
		while(bc--) {
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		}
		while(count--)
		    *dest_y++ = *src_y++;

		cbc = clr_count >> 4;
		clr_count &= 0xf;

		while (cbc--) {
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		}
		while (clr_count--)
		    *clr_y++ = 0;
    }
}
