#include "ite.h"
#if ! defined (NITE)
#define NITE 1
#endif
#if NITE > 0

#include "param.h"
#include "conf.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "ite.h"
#include "itevar.h"
#include "iteioctl.h"
#include "machine/cpu.h"

#include "../amiga/cc_types.h"
#include "../amiga/cc_chipmem.h"

#include "grf/grf_types.h"
#include "grf/grf_view.h"
#include "grf/grf_bitmap.h"
#include "viewioctl.h"
#include "viewvar.h"
#include "termios.h"

extern unsigned char kernel_font_width, kernel_font_height, kernel_font_baseline; 
extern short         kernel_font_boldsmear;
extern unsigned char kernel_font_lo, kernel_font_hi;
extern unsigned char kernel_font[], kernel_cursor[];

/*
 * This is what ip->priv points to;
 * it contains local variables for custom-chip ites.
 */
typedef struct ite_priv {
  view_t *view;					  /* the view for this ite. */
  u_char **row_ptr;				  /* array of pointers into the bitmap  */
  u_long row_bytes;
  u_long cursor_opt;
  
  /* these are precalc'ed for the putc routine so it can be faster. */
  u_int  *column_offset;			  /* array of offsets for columns */
  u_int  row_offset;				  /* the row offset */
  u_word width;					  /* the bitmap width */
  u_word underline;				  /* where the underline goes */
  u_word ft_x;					  /* the font width */
  u_word ft_y;					  /* the font height */
  u_char *font_cell[256];			  /* the font pointer */
} ipriv_t;

extern struct itesw itesw[];

/* (M<=8)-by-N routines */
static void view_le32n_cursor(struct ite_softc *ip, int flag);
static void view_le8n_putc(struct ite_softc *ip, int c, int dy, int dx, int mode);
static void view_le8n_clear(struct ite_softc *ip, int sy, int sx, int h, int w);
static void view_le8n_scroll(struct ite_softc *ip, int sy, int sx, int count, int dir);
void scroll_bitmap (bmap_t *bm, u_word x, u_word y, u_word width, u_word height, word dx, word dy, u_byte mask);


/* globals */

int ite_default_x;
int ite_default_y;
int ite_default_width = 640;

#if defined (GRF_NTSC)
int ite_default_height = 400;
int ite_default_depth = 2;
#elif defined (GRF_PAL)
int ite_default_height = 512;
int ite_default_depth = 2;
#else
int ite_default_height = 400;
int ite_default_depth = 2;
#endif

/* audio bell stuff */
static char *bell_data;
static struct ite_bell_values bell_vals = { 64, 200, 10 };

cc_unblank ()
{
}

init_bell ()
{
    short i;
    static char sample[20] = {
	0,39,75,103,121,127,121,103,75,39,0,
	-39,-75,-103,-121,-127,-121,-103,-75,-39 };

    if (!bell_data) {
	bell_data = alloc_chipmem(20);
	if (!bell_data)
	   return (-1); 

	for (i=0; i<20; i++) 
	    bell_data[i] = sample[i];
    }
    return (0);
}

cc_bell ()
{
    if (bell_data) {
      play_sample (10, PREP_DMA_MEM(bell_data),
                   bell_vals.period, bell_vals.volume, 0x3, bell_vals.time);
    }
}

extern struct  ite_softc ite_softc[];
#define IPUNIT(ip) (((u_long)ip-(u_long)ite_softc)/sizeof(struct ite_softc))

int 
ite_new_size (struct ite_softc *ip, struct ite_window_size *vs)
{
    extern struct view_softc views[];
    ipriv_t *cci = ip->priv;    
    u_long fbp, i;
    int error;

    error = viewioctl (IPUNIT (ip), VIEW_SETSIZE, (struct view_size *)vs, 0, -1);
    cci->view = views[IPUNIT(ip)].view; 

    ip->rows = (cci->view->display.height) / ip->ftheight;
    ip->cols = (cci->view->display.width-1)  / ip->ftwidth; /* -1 for bold. */

    /* save new values so that future opens use them */
    /* this may not be correct when we implement Virtual Consoles */
    ite_default_height = cci->view->display.height;
    ite_default_width = cci->view->display.width;
    ite_default_x = cci->view->display.x;
    ite_default_y = cci->view->display.y;
    ite_default_depth = cci->view->bitmap->depth;

    if (cci->row_ptr) 
        free_chipmem (cci->row_ptr);
    if (cci->column_offset)
	free_chipmem (cci->column_offset);

    cci->row_ptr = alloc_chipmem (sizeof (u_byte *)*ip->rows);
    cci->column_offset = alloc_chipmem (sizeof (u_int)*ip->cols);
    
    if (!cci->row_ptr || !cci->column_offset) {
	panic ("no memory for console device.");
    }

    cci->width = cci->view->bitmap->bytes_per_row << 3;
    cci->underline = ip->ftbaseline + 1;
    cci->row_offset = (cci->view->bitmap->bytes_per_row + cci->view->bitmap->row_mod);
    cci->ft_x = ip->ftwidth;
    cci->ft_y = ip->ftheight;
 
    cci->row_bytes = cci->row_offset * ip->ftheight;

    /* initialize the row pointers */
    cci->row_ptr[0] = VDISPLAY_LINE (cci->view, 0, 0);
    for (i = 1; i < ip->rows; i++) 
	cci->row_ptr[i] = cci->row_ptr[i-1] + cci->row_bytes;

    /* initialize the column offsets */
    cci->column_offset[0] = 0;
    for (i = 1; i < ip->cols; i++) 
	cci->column_offset[i] = cci->column_offset[i-1] + cci->ft_x;

    /* initialize the font cell pointers */
    cci->font_cell[ip->font_lo] = ip->font;
    for (i=ip->font_lo+1; i<=ip->font_hi; i++)
	cci->font_cell[i] = cci->font_cell[i-1] + ip->ftheight;
	    
    return (error);
}


ite_view_init(ip)
	register struct ite_softc *ip;
{
    struct itesw *sp = itesw;
    ipriv_t *cci = ip->priv;
    struct ite_window_size vs;

    init_bell ();

    if (!cci) {
	ip->font     = kernel_font;
	ip->font_lo  = kernel_font_lo;
	ip->font_hi  = kernel_font_hi;
	ip->ftwidth  = kernel_font_width;
	ip->ftheight = kernel_font_height;
	ip->ftbaseline = kernel_font_baseline;
	ip->ftboldsmear = kernel_font_boldsmear;

	/* Find the correct set of rendering routines for this font.  */
	if (ip->ftwidth <= 8) {
		sp->ite_cursor = (void*)view_le32n_cursor;
		sp->ite_putc = (void*)view_le8n_putc;
		sp->ite_clear = (void*)view_le8n_clear;
		sp->ite_scroll = (void*)view_le8n_scroll;
	} else { 
		panic("kernel font size not supported");
	}

	cci = alloc_chipmem (sizeof (*cci));
	if (!cci) {
		panic ("no memory for console device.");
	}

	ip->priv = cci;
	cci->cursor_opt = 0;
	cci->view = NULL;
	cci->row_ptr = NULL;
	cci->column_offset = NULL;

#if 0
	/* currently the view is permanently open due to grf driver */
	if (viewopen (IPUNIT(ip), 0)) {
	    panic ("cannot get ahold of our view");
	}
#endif
	vs.x = ite_default_x;
	vs.y = ite_default_y;
	vs.width = ite_default_width;
	vs.height = ite_default_height;
	vs.depth = ite_default_depth;

	ite_new_size (ip, &vs);
	XXX_grf_cc_on (IPUNIT (ip));
	/* viewioctl (IPUNIT(ip), VIEW_DISPLAY, NULL, 0, -1); */
    }
}

int
ite_grf_ioctl (ip, cmd, addr, flag, p)
	struct ite_softc *ip;
	caddr_t addr;
	struct proc *p;
{
    int error = 0;
    struct ite_window_size *is;
    struct ite_bell_values *ib;
    ipriv_t *cci = ip->priv;

    switch (cmd) {
      case ITE_GET_WINDOW_SIZE:
	is = (struct ite_window_size *)addr;
        is->x = cci->view->display.x;
	is->y = cci->view->display.y;
	is->width = cci->view->display.width;
 	is->height = cci->view->display.height;
 	is->depth = cci->view->bitmap->depth;
        break;

      case ITE_SET_WINDOW_SIZE:
        is = (struct ite_window_size *)addr;

        if (ite_new_size (ip, is)) {
            error = ENOMEM;
        } else {
	    struct winsize ws;
	    ws.ws_row = ip->rows;
	    ws.ws_col = ip->cols;
	    ws.ws_xpixel = cci->view->display.width;
	    ws.ws_ypixel = cci->view->display.height;
	    ite_reset (ip);
	    /* XXX tell tty about the change *and* the process group should */
	    /* XXX be signalled---this is messy, but works nice :^) */
	    iteioctl (0, TIOCSWINSZ, &ws, 0, p);
        }
        break;

      case ITE_DISPLAY_WINDOW:
	XXX_grf_cc_on (IPUNIT (ip));
	/* viewioctl (IPUNIT(ip), VIEW_DISPLAY, NULL, 0, -1); */
	break;

      case ITE_REMOVE_WINDOW:
	XXX_grf_cc_off (IPUNIT (ip));
        /* viewioctl (IPUNIT(ip), VIEW_REMOVE, NULL, 0, -1); */
        break;

      case ITE_GET_BELL_VALUES:
      	ib = (struct ite_bell_values *)addr;
	bcopy (&bell_vals, ib, sizeof (struct ite_bell_values));	
	break;

      case ITE_SET_BELL_VALUES:
      	ib = (struct ite_bell_values *)addr;
	bcopy (ib, &bell_vals, sizeof (struct ite_bell_values));	
	break;
      case VIEW_USECOLORMAP:
      case VIEW_GETCOLORMAP:
	/* XXX needs to be fixed when multiple console implemented. */
	/* XXX watchout for that -1 its not really the kernel talking. */
	/* XXX these two commands don't use the proc pointer though. */
	error = viewioctl (0, cmd, addr, flag, -1);
	break;
      default:
        error = -1;
        break;
    }
    return (error);
}

ite_view_deinit(ip)
	struct ite_softc *ip;
{
  ip->flags &= ~ITE_INITED;
  /* FIX: free our stuff. */
  if (ip->priv) {
    ipriv_t *cci = ip->priv;

#if 0
    /* view stays open permanently */
    if (cci->view) {
        viewclose (IPUNIT(ip));
    }
#endif
    
    /* well at least turn it off. */
    XXX_grf_cc_off (IPUNIT (ip));
    /* viewioctl (IPUNIT(ip), VIEW_REMOVE, NULL, 0, -1); */
    
    if (cci->row_ptr) 
   	free_chipmem (cci->row_ptr);
    if (cci->column_offset)
	free_chipmem (cci->column_offset);
	
    free_chipmem (cci);
    ip->priv = NULL;
  }
}

/*** (M<8)-by-N routines ***/

static void
view_le32n_cursor(struct ite_softc *ip, int flag)
{
    ipriv_t  *cci = (ipriv_t *) ip->priv;
    view_t *v = cci->view;
    bmap_t *bm = v->bitmap;
    int dr_plane = (bm->depth > 1 ? bm->depth-1 : 0);
    int cend, ofs, h, cstart;
    unsigned char opclr, opset;
    u_char *pl;
    
    if (flag == START_CURSOROPT) {
	if (!cci->cursor_opt) {
	    view_le32n_cursor (ip, ERASE_CURSOR);
	}
	cci->cursor_opt++;
	return;				  /* if we are already opted. */
    } else if (flag == END_CURSOROPT) {
	cci->cursor_opt--;
	
    }
    
    if (cci->cursor_opt) 
	return;					  /* if we are still nested. */
						  /* else we draw the cursor. */

    cstart = 0;
    cend = ip->ftheight-1; 
    pl = VDISPLAY_LINE (v, dr_plane, (ip->cursory*ip->ftheight+cstart));
    ofs = (ip->cursorx * ip->ftwidth);
    
    /* erase cursor */
    if (flag != DRAW_CURSOR && flag != END_CURSOROPT) {
	/* erase the cursor */
	int h;
	if (dr_plane) {
	    for (h = cend; h >= 0; h--) {
		asm("bfclr %0@{%1:%2}"
		    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
		pl += cci->row_offset;
	    }
	} else {
	    for (h = cend; h >= 0; h--) {
		asm("bfchg %0@{%1:%2}"
		    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
		pl += cci->row_offset;
	    }
	}
    }
    
    if ((flag == DRAW_CURSOR || flag == MOVE_CURSOR || flag == END_CURSOROPT)) {
	
	ip->cursorx = MIN(ip->curx, ip->cols-1);
	ip->cursory = ip->cury;
	cstart = 0;
	cend = ip->ftheight-1; 
	pl = VDISPLAY_LINE (v, dr_plane, (ip->cursory*ip->ftheight+cstart));
	ofs = (ip->cursorx * ip->ftwidth);
	
	/* draw the cursor */
	if (dr_plane) {
	    for (h = cend; h >= 0; h--) {
		asm("bfset %0@{%1:%2}"
		    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
		pl += cci->row_offset;
	    }
	} else {
	    for (h = cend; h >= 0; h--) {
		asm("bfchg %0@{%1:%2}"
		    : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
		pl += cci->row_offset;
	    }
	}
    }
}


static inline
int expbits (int data)
{
    int i, nd = 0;
    if (data & 1) {
    	nd |= 0x02;
    }
    for (i=1; i < 32; i++) {
	if (data & (1 << i)) {
	   nd |= 0x5 << (i-1);
	}
    }
    nd &= ~data;
    return (~nd);
}


/* Notes: optimizations given the kernel_font_(width|height) #define'd.
 *        the dbra loops could be elminated and unrolled using height,
 *        the :width in the bfxxx instruction could be made immediate instead
 *        of a data register as it now is.
 *        the underline could be added when the loop is unrolled
 *
 *        It would look like hell but be very fast.*/
 
static void 
putc_nm (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    while (fh--) {
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (*f++), "a" (p), "d" (co), "d" (fw));
	p += ro;
    }
}

static void 
putc_in (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    while (fh--) {
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;
    }
}


static void 
putc_ul (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    int underline = cci->underline;
    while (underline--) {
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (*f++), "a" (p), "d" (co), "d" (fw));
	p += ro;
    }

    asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	 "d" (expbits(*f++)), "a" (p), "d" (co), "d" (fw));
    p += ro;

    underline = fh - cci->underline - 1;
    while (underline--) {
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (*f++), "a" (p), "d" (co), "d" (fw));
	p += ro;
    }
}


static void 
putc_ul_in (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    int underline = cci->underline;
    while (underline--) {
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;
    }

    asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	 "d" (~expbits(*f++)), "a" (p), "d" (co), "d" (fw));
    p += ro;

    underline = fh - cci->underline - 1;
    while (underline--) {
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~(*f++)), "a" (p), "d" (co), "d" (fw));
	p += ro;
    }
}

/* bold */
static void 
putc_bd (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    u_word ch;
    
    while (fh--) {
	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (ch), "a" (p), "d" (co), "d" (fw+1));
	p += ro;
    }
}

static void 
putc_bd_in (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    u_word ch;
    
    while (fh--) {
	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~(ch)), "a" (p), "d" (co), "d" (fw+1));
	p += ro;
    }
}


static void 
putc_bd_ul (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    int underline = cci->underline;
    u_word ch;

    while (underline--) {
	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (ch), "a" (p), "d" (co), "d" (fw+1));
	p += ro;
    }

    ch = *f++;
    ch |= ch << 1;
    asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	 "d" (expbits(ch)), "a" (p), "d" (co), "d" (fw+1));
    p += ro;

    underline = fh - cci->underline - 1;
    while (underline--) {
	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (ch), "a" (p), "d" (co), "d" (fw+1));
	p += ro;
    }
}


static void 
putc_bd_ul_in (cci,p,f,co,ro,fw,fh)
    register ipriv_t *cci;
    register u_char  *p;
    register u_char  *f;
    register u_int    co;
    register u_int    ro;
    register u_int    fw;
    register u_int    fh;
{
    int underline = cci->underline;
    u_word ch;
    
    while (underline--) {
	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~(ch)), "a" (p), "d" (co), "d" (fw+1));
	p += ro;
    }

    ch = *f++;
    ch |= ch << 1;
    asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	 "d" (~expbits(ch)), "a" (p), "d" (co), "d" (fw+1));
    p += ro;

    underline = fh - cci->underline - 1;
    while (underline--) {
	ch = *f++;
	ch |= ch << 1;
	asm ("bfins %0,%1@{%2:%3}" : /* no output */ :
	     "d" (~(ch)), "a" (p), "d" (co), "d" (fw+1));
	p += ro;
    }
}


typedef void cc_putc_func ();

cc_putc_func *put_func[ATTR_ALL+1] = {
    putc_nm,
    putc_in,
    putc_ul,
    putc_ul_in,
    putc_bd,
    putc_bd_in,
    putc_bd_ul,
    putc_bd_ul_in,
/* no support for blink */
    putc_nm,
    putc_in,
    putc_ul,
    putc_ul_in,
    putc_bd,
    putc_bd_in,
    putc_bd_ul,
    putc_bd_ul_in
};


/* FIX: shouldn't this advance the cursor even if the character to
        be output is not available in the font? -ch */

static void
view_le8n_putc(struct ite_softc *ip, int c, int dy, int dx, int mode)
{
    register ipriv_t *cci = (ipriv_t *) ip->priv;
    if (c < ip->font_lo || c > ip->font_hi)
	return;

    put_func[mode](cci,
		   cci->row_ptr[dy],
		   cci->font_cell[c],
		   cci->column_offset[dx],
		   cci->row_offset,
		   cci->ft_x,
		   cci->ft_y);
}

static void
view_le8n_clear(struct ite_softc *ip, int sy, int sx, int h, int w)
{
  ipriv_t *cci = (ipriv_t *) ip->priv;
  view_t *v = cci->view;
  bmap_t *bm = cci->view->bitmap;

  if ((sx == 0) && (w == ip->cols))
    {
      /* common case: clearing whole lines */
      while (h--)
	{
	  int i;
	  u_char *ptr = cci->row_ptr[sy]; 
	  for (i=0; i < ip->ftheight; i++) {
            bzero(ptr, bm->bytes_per_row);
            ptr += bm->bytes_per_row + bm->row_mod;			/* don't get any smart
                                                   ideas, becuase this is for
                                                   interleaved bitmaps */
          }
	  sy++;
	}
    }
  else
    {
      /* clearing only part of a line */
      /* XXX could be optimized MUCH better, but is it worth the trouble? */
      while (h--)
	{
	  u_char *pl = cci->row_ptr[sy];
          int ofs = sx * ip->ftwidth;
	  int i, j;
	  for (i = w-1; i >= 0; i--)
	    {
	      u_char *ppl = pl;
              for (j = ip->ftheight-1; j >= 0; j--)
	        {
	          asm("bfclr %0@{%1:%2}"
	              : : "a" (ppl), "d" (ofs), "d" (ip->ftwidth));
	          ppl += bm->row_mod + bm->bytes_per_row; 
	        }
	      ofs += ip->ftwidth;
	    }
	  sy++;
	}
    }
}

/* Note: sx is only relevant for SCROLL_LEFT or SCROLL_RIGHT.  */
static void
view_le8n_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
  bmap_t *bm = ((ipriv_t *)ip->priv)->view->bitmap;
  u_char *pl = ((ipriv_t *)ip->priv)->row_ptr[sy];

  if (dir == SCROLL_UP) 
    {
      int dy = sy - count;
      int height = ip->bottom_margin - sy + 1;
      int i;

      /*FIX: add scroll bitmap call */
        view_le32n_cursor(ip, ERASE_CURSOR);
	scroll_bitmap (bm, 0, dy*ip->ftheight,
		       bm->bytes_per_row >> 3, (ip->bottom_margin-dy+1)*ip->ftheight,
		       0, -(count*ip->ftheight), 0x1);
/*	if (ip->cursory <= bot || ip->cursory >= dy) {
	    ip->cursory -= count;
	} */
    }
  else if (dir == SCROLL_DOWN) 
    {
      int dy = sy + count;
      int height = ip->bottom_margin - dy + 1;
      int i;

      /* FIX: add scroll bitmap call */
        view_le32n_cursor(ip, ERASE_CURSOR);
	scroll_bitmap (bm, 0, sy*ip->ftheight,
		       bm->bytes_per_row >> 3, (ip->bottom_margin-sy+1)*ip->ftheight,
		       0, count*ip->ftheight, 0x1);
/*	if (ip->cursory <= bot || ip->cursory >= sy) {
	    ip->cursory += count;
	} */
    }
  else if (dir == SCROLL_RIGHT) 
    {
      int sofs = (ip->cols - count) * ip->ftwidth;
      int dofs = (ip->cols) * ip->ftwidth;
      int i, j;

      view_le32n_cursor(ip, ERASE_CURSOR);
      for (j = ip->ftheight-1; j >= 0; j--)
	{
	  int sofs2 = sofs, dofs2 = dofs;
	  for (i = (ip->cols - (sx + count))-1; i >= 0; i--)
	    {
	      int t;
	      sofs2 -= ip->ftwidth;
	      dofs2 -= ip->ftwidth;
	      asm("bfextu %1@{%2:%3},%0"
	          : "=d" (t)
		  : "a" (pl), "d" (sofs2), "d" (ip->ftwidth));
	      asm("bfins %3,%0@{%1:%2}"
	          : : "a" (pl), "d" (dofs2), "d" (ip->ftwidth), "d" (t));
	    }
	  pl += bm->row_mod + bm->bytes_per_row; 
	}
    }
  else /* SCROLL_LEFT */
    {
      int sofs = (sx) * ip->ftwidth;
      int dofs = (sx - count) * ip->ftwidth;
      int i, j;

      view_le32n_cursor(ip, ERASE_CURSOR);
      for (j = ip->ftheight-1; j >= 0; j--)
	{
	  int sofs2 = sofs, dofs2 = dofs;
	  for (i = (ip->cols - sx)-1; i >= 0; i--)
	    {
	      int t;
	      asm("bfextu %1@{%2:%3},%0"
	          : "=d" (t)
		  : "a" (pl), "d" (sofs2), "d" (ip->ftwidth));
	      asm("bfins %3,%0@{%1:%2}"
	          : : "a" (pl), "d" (dofs2), "d" (ip->ftwidth), "d" (t));
	      sofs2 += ip->ftwidth;
	      dofs2 += ip->ftwidth;
	    }
	  pl += bm->row_mod + bm->bytes_per_row; 
	}
    }		
}

void 
scroll_bitmap (bmap_t *bm, u_word x, u_word y, u_word width, u_word height, word dx, word dy, u_byte mask)
{
    u_word depth = bm->depth; 
    u_word lwpr = bm->bytes_per_row >> 2;
    if (dx) {
    	/* FIX: */ panic ("delta x not supported in scroll bitmap yet.");
    } 
    if (bm->flags & BMF_INTERLEAVED) {
	height *= depth;
	depth = 1;
    }
    if (dy == 0) {
        return;
    }
    if (dy > 0) {
    	int i;
    	for (i=0; i < depth && mask; i++, mask >>= 1) {
    	    if (0x1 & mask) {
	    	u_long *pl = (u_long *)bm->plane[i];
		u_long *src_y = pl + (lwpr*y);
		u_long *dest_y = pl + (lwpr*(y+dy));
		u_long count = lwpr*(height-dy);
		u_long *clr_y = src_y;
		u_long clr_count = dest_y - src_y;
		u_long bc, cbc;
		
		src_y += count - 1;
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
		while (count--) {
		    *dest_y-- = *src_y--;
		}

		cbc = clr_count >> 4;
		clr_count &= 0xf;

		while (cbc--) {
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		}
		while (clr_count--) {
		    *clr_y++ = 0;
		}
    	    }
	}
    } else if (dy < 0) {
    	int i;
    	for (i=0; i < depth && mask; i++, mask >>= 1) {
    	    if (0x1 & mask) {
    		u_long *pl = (u_long *)bm->plane[i];
    		u_long *src_y = pl + (lwpr*(y-dy));
    		u_long *dest_y = pl + (lwpr*y); 
		long count = lwpr*(height + dy);
		u_long *clr_y = dest_y + count;
		u_long clr_count = src_y - dest_y;
		u_long bc, cbc;

		bc = count >> 4;
		count &= 0xf;
		
		while (bc--) {
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		    *dest_y++ = *src_y++; *dest_y++ = *src_y++;
		}
		while (count--) {
		    *dest_y++ = *src_y++;
		}

		cbc = clr_count >> 4;
		clr_count &= 0xf;

		while (cbc--) {
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		    *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0; *clr_y++ = 0;
		}
		while (clr_count--) {
		    *clr_y++ = 0;
		}
	    }
	}
    }
}
#endif
