#include "ite.h"
#if NITE > 0

#include "param.h"
#include "conf.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "ite.h"
#include "itevar.h"

#include "machine/cpu.h"

/* XXX */
#include "grfioctl.h"
#include "grfvar.h"
#include "grf_ccreg.h"

#include "../amiga/custom.h"

extern caddr_t CHIPMEMADDR;

extern unsigned char kernel_font_width, kernel_font_height;
extern unsigned char kernel_font_lo, kernel_font_hi;
extern unsigned char kernel_font[], kernel_cursor[];

/*
 * This holds the instructions to retarget the plane 0 pointer
 * at each split point.
 */
typedef struct {
  u_short wait[2];		/* wait instruction */
  u_short plane[4];		/* move + hi word, move + lo word */
} COP_ROW;

typedef struct {
  u_char *buf;			/* pointer to row within frame buffer */
  int polarity;			/* polarity for loading planes in copper list */
} BUF_ROW;

/*
 * This is what ip->priv points to;
 * it contains local variables for custom-chip ites.
 */
struct ccite {
  struct ccfb *fb;
  BUF_ROW *buf_rows;		/* array of pointers into the frame buffer */
  COP_ROW *cop_rows[2];		/* extension to grf_cc's copper lists */
};

static struct ccite ccite[NITE];

static BUF_ROW ccite_buf_rows[NITE][100]; /* XXX see below */

extern struct itesw itesw[];

/* 8-by-N routines */
static void cc_8n_cursor(struct ite_softc *ip, int flag);
static void cc_8n_putc(struct ite_softc *ip, int c, int dy, int dx, int mode);
static void cc_8n_clear(struct ite_softc *ip, int sy, int sx, int h, int w);
static void cc_8n_scroll(struct ite_softc *ip, int sy, int sx, int count, int dir);

/* (M<=8)-by-N routines */
static void cc_le32n_cursor(struct ite_softc *ip, int flag);
static void cc_le8n_putc(struct ite_softc *ip, int c, int dy, int dx, int mode);
static void cc_le8n_clear(struct ite_softc *ip, int sy, int sx, int h, int w);
static void cc_le8n_scroll(struct ite_softc *ip, int sy, int sx, int count, int dir);

/* Mykes: Insert your whiz-bang 8-by-8 routines here... ;-) */



customc_init(ip)
	register struct ite_softc *ip;
{
  struct ccite *cci;
  struct ccfb *fb;
  struct itesw *sp = &itesw[ip->type];

  if (ip->grf == 0)
    ip->grf = &grf_softc[ip - ite_softc];

  cci = &ccite[ip - ite_softc];
  ip->priv = cci;
  fb = (struct ccfb *) ip->grf->g_data;
  cci->fb = fb;

  ip->font     = kernel_font;
  ip->font_lo  = kernel_font_lo;
  ip->font_hi  = kernel_font_hi;
  ip->ftwidth  = kernel_font_width;
  ip->ftheight = kernel_font_height;
#if 0
  ip->cursor   = kernel_cursor;
#endif
  
  ip->rows     = fb->disp_height / ip->ftheight;
  ip->cols     = fb->disp_width  / ip->ftwidth;

  /* Find the correct set of rendering routines for this font.  */
#if 0
  /* The new unspecialized routines are faster than the old specialized ones
     for the same font!!! (and without even unrolling them...)
     Therefore I'm leaving them out for now.  */
  if (ip->ftwidth == 8)
    {
      sp->ite_cursor = cc_8n_cursor;
      sp->ite_putc = cc_8n_putc;
      sp->ite_clear = cc_8n_clear;
      sp->ite_scroll = cc_8n_scroll;
    }
  else
#endif
  if (ip->ftwidth <= 8)
    {
      sp->ite_cursor = (void*)cc_le32n_cursor;
      sp->ite_putc = (void*)cc_le8n_putc;
      sp->ite_clear = (void*)cc_le8n_clear;
      sp->ite_scroll = (void*)cc_le8n_scroll;
    }
  else
    panic("kernel font size not supported");
  
  /* XXX It may be better if this was dynamic based on ip->rows,
     but is dynamic memory allocation available at this point?  */
  cci->buf_rows = ccite_buf_rows[ip - ite_softc];
  
  /* Now allocate memory for the special screen-split copper lists.
     We will need a COP_ROW structure for each text row,
     plus an extra row to terminate the list.  */
  /* testing for the result is really redundant because chipmem_steal
     panics if it runs out of memory.. */
  if (! (cci->cop_rows[0] = (COP_ROW *)
	 chipmem_steal (sizeof(COP_ROW) * (ip->rows + 1)))
      || !(cci->cop_rows[1] = (COP_ROW *)
	   chipmem_steal (sizeof(COP_ROW) * (ip->rows + 1))))
    return 0;

  /* Initialize the screen-split row arrays.  */
  {
    int i, ypos = 0;
    u_long rowbytes = fb->fb_width >> 3;
    u_long fbp, fbp2;

    fbp = ((u_long)fb->fb + (fb->fb_x >> 3) + fb->fb_y * rowbytes);
    for (i = 0; i < ip->rows; i++)
      {
	cci->buf_rows[i].buf = (u_char*)fbp;
	cci->buf_rows[i].polarity = (fb->disp_y + ypos) & 1;

	COP_WAIT(cci->cop_rows[0][i].wait, (fb->disp_y + ypos + 1) >> 1);
	fbp2 = (fbp - (u_long)CHIPMEMADDR
		+ (cci->buf_rows[i].polarity ? rowbytes : 0));
	COP_MOVE(cci->cop_rows[0][i].plane, bplpth(0), HIADDR(fbp2));
	COP_MOVE(cci->cop_rows[0][i].plane+2, bplptl(0), LOADDR(fbp2));

	COP_WAIT(cci->cop_rows[1][i].wait, (fb->disp_y + ypos) >> 1);
	fbp2 = (fbp - (u_long)CHIPMEMADDR +
		(cci->buf_rows[i].polarity ? 0 : rowbytes));
	COP_MOVE(cci->cop_rows[1][i].plane, bplpth(0), HIADDR(fbp2));
	COP_MOVE(cci->cop_rows[1][i].plane+2, bplptl(0), LOADDR(fbp2));

	ypos += ip->ftheight;
	fbp += ip->ftheight * rowbytes;
      }

    /* Turn the display off after the last row;
       otherwise we'll get funny text at the bottom of the screen
       because of reordered rows.  */
    COP_WAIT(cci->cop_rows[0][i].wait+0, (fb->disp_y + ypos + 1) >> 1);
    COP_MOVE(cci->cop_rows[0][i].wait+2, bplcon0, 0x8204);
    COP_END (cci->cop_rows[0][i].wait+4);
    COP_WAIT(cci->cop_rows[1][i].wait+0, (fb->disp_y + ypos) >> 1);
    COP_MOVE(cci->cop_rows[1][i].wait+2, bplcon0, 0x8204);
    COP_END (cci->cop_rows[1][i].wait+4);
  }

  /* Install the new copper list extensions.  */
  cc_install_cop_ext(ip->grf, cci->cop_rows[0], cci->cop_rows[1]);
  
#if 0
  printf ("font@%x, cursor@%x\n", ip->font, ip->cursor);
  dump_copperlist (fb->cop1);
  dump_copperlist (fb->cop2);
#endif
}

customc_deinit(ip)
	struct ite_softc *ip;
{
  ip->flags &= ~ITE_INITED;

  /* Take our grubby little fingers out of the grf's copper list.  */
  cc_uninstall_cop_ext(ip->grf);
}

/*
 * Swap two text rows in the display by modifying the copper list.
 */
static __inline int
swap_rows(struct ite_softc *ip, int row1, int row2)
{
	struct ccite *cci = (struct ccite *) ip->priv;
	int rowbytes = cci->fb->fb_width >> 3;
	u_char *tmp, *fbp2;

	/* Swap the plane pointers */
	tmp = cci->buf_rows[row1].buf;
	cci->buf_rows[row1].buf = cci->buf_rows[row2].buf;
	cci->buf_rows[row2].buf = tmp;

	/* Update the copper lists */
	fbp2 = (cci->buf_rows[row1].buf - (u_long)CHIPMEMADDR
		+ (cci->buf_rows[row1].polarity ? rowbytes : 0));
	cci->cop_rows[0][row1].plane[1] = HIADDR(fbp2);
	cci->cop_rows[0][row1].plane[3] = LOADDR(fbp2);

	fbp2 = (cci->buf_rows[row1].buf - (u_long)CHIPMEMADDR
		+ (cci->buf_rows[row1].polarity ? 0 : rowbytes));
	cci->cop_rows[1][row1].plane[1] = HIADDR(fbp2);
	cci->cop_rows[1][row1].plane[3] = LOADDR(fbp2);

	fbp2 = (cci->buf_rows[row2].buf - (u_long)CHIPMEMADDR
		+ (cci->buf_rows[row2].polarity ? rowbytes : 0));
	cci->cop_rows[0][row2].plane[1] = HIADDR(fbp2);
	cci->cop_rows[0][row2].plane[3] = LOADDR(fbp2);

	fbp2 = (cci->buf_rows[row2].buf - (u_long)CHIPMEMADDR
		+ (cci->buf_rows[row2].polarity ? 0 : rowbytes));
	cci->cop_rows[1][row2].plane[1] = HIADDR(fbp2);
	cci->cop_rows[1][row2].plane[3] = LOADDR(fbp2);

	/* If the drawn cursor was on either row, swap it too.  */
	if (ip->cursory == row1)
		ip->cursory = row2;
	else if (ip->cursory == row2)
		ip->cursory = row1;
}



/*** 8-by-N routines ***/

static inline void
cc_8n_windowmove (src, srcx, srcy, srcmod,
		    dst, dstx, dsty, dstmod, h, w, op)
    unsigned char *src, *dst;
    unsigned short srcx, srcy, srcmod;
    unsigned short dstx, dsty, dstmod;
    unsigned short h, w;
    unsigned char op;
{
  short i;	/* NOT unsigned! */
  unsigned char h1;

  src += srcmod * srcy + (srcx >> 3);
  dst += dstmod * dsty + (dstx >> 3);

#if 0
printf("ccwm: %x-%x-%x-%x-%c\n", src, dst, h, w,
	op == RR_XOR ? '^' : op == RR_COPY ? '|' : op == RR_CLEAR ? 'C' : 'I');
#endif
  
  /* currently, only drawing to byte slots is supported... */
  if ((srcx & 07) || (dstx & 07) || (w & 07))
    panic ("customc_windowmove: odd offset");
    
  w >>= 3;
  
  /* Ok, this is nastier than it could be to help the optimizer unroll
     loops for the most common case of 8x8 characters.

     Note that bzero() does some clever optimizations for large range
     clears, so it should pay the subroutine call. */

/* perform OP on one bit row of data. */
#define ONEOP(dst, src, op) \
	do { if ((src) > (dst))					\
	  for (i = 0; i < w; i++) (dst)[i] op (src)[i];		\
	else							\
	  for (i = w - 1; i >= 0; i--) (dst)[i] op (src)[i]; } while (0)

/* perform a block of eight ONEOPs. This enables the optimizer to unroll
   the for-statements, as they have a loop counter known at compiletime */
#define EIGHTOP(dst, src, op) \
	for (h1 = 0; h1 < 8; h1++, src += srcmod, dst += dstmod) \
	    ONEOP (dst, src, op);
	  
  switch (op)
    {
    case RR_COPY:
      for (; h >= 8; h -= 8)
	EIGHTOP (dst, src, =);
      for (; h > 0; h--, src += srcmod, dst += dstmod)
        ONEOP (dst, src, =);
      break;
	  
    case RR_CLEAR:
      for (; h >= 8; h -= 8)
	for (h1 = 0; h1 < 8; h1++, dst += dstmod)
	  bzero (dst, w);
      for (; h > 0; h--, dst += dstmod)
        bzero (dst, w);
      break;
	  
    case RR_XOR:
      for (; h >= 8; h -= 8)
	EIGHTOP (dst, src, ^=);
      for (; h > 0; h--, src += srcmod, dst += dstmod)
        ONEOP (dst, src, ^=);
      break;
	  
    case RR_COPYINVERTED:
      for (; h >= 8; h -= 8)
	EIGHTOP (dst, src, =~);
      for (; h > 0; h--, src += srcmod, dst += dstmod)
        ONEOP (dst, src, =~);
      break;
    }
}


static void
cc_8n_cursor(ip, flag)
	register struct ite_softc *ip;
        register int flag;
{
  struct ccite *cci = (struct ccite *) ip->priv;
  struct ccfb *fb = cci->fb;
  /* the cursor is always drawn in the last plane */
  unsigned char *ovplane, opclr, opset;
  
  ovplane = fb->fb + (fb->fb_z - 1) * (fb->fb_planesize);

  if (flag == START_CURSOROPT || flag == END_CURSOROPT)
    return;

  /* if drawing into an overlay plane, don't xor, clr and set */
  if (fb->fb_z > fb->disp_z)
    {
      opclr = RR_CLEAR; opset = RR_COPY;
    }
  else
    {
      opclr = opset = RR_XOR;
    }

  if (flag != DRAW_CURSOR)
    {
      /* erase it */
      cc_8n_windowmove (ip->cursor, 0, 0, 1,
	  		  ovplane, fb->fb_x + ip->cursorx * ip->ftwidth,
    			  fb->fb_y + ip->cursory * ip->ftheight,
    			  fb->fb_width >> 3,
    			  ip->ftheight, ip->ftwidth, opclr);
    }
  if (flag == DRAW_CURSOR || flag == MOVE_CURSOR)
    {
      /* draw it */
      int newx = MIN(ip->curx, ip->cols - 1);
      cc_8n_windowmove (ip->cursor, 0, 0, 1,
	    		  ovplane, fb->fb_x + newx * ip->ftwidth,
    			  fb->fb_y + ip->cury * ip->ftheight,
    			  fb->fb_width >> 3,
    			  ip->ftheight, ip->ftwidth, opset);
      ip->cursorx = newx;
      ip->cursory = ip->cury;
    }
}

static void
cc_8n_putc(ip, c, dy, dx, mode)
	register struct ite_softc *ip;
	register int dy, dx;
	int c, mode;
{
  register int wrr = ((mode == ATTR_INV) ? RR_COPYINVERTED : RR_COPY);
  struct ccite *cci = (struct ccite *) ip->priv;
  struct ccfb *fb = cci->fb;

  if (c >= ip->font_lo && c <= ip->font_hi)
    {
      c -= ip->font_lo;

      cc_8n_windowmove (ip->font, 0, c * ip->ftheight, 1,
    			  cci->buf_rows[dy].buf,
			  dx * ip->ftwidth, 0, fb->fb_width >> 3,
    			  ip->ftheight, ip->ftwidth, wrr);
    }
}

static void
cc_8n_clear(ip, sy, sx, h, w)
	struct ite_softc *ip;
	register int sy, sx, h, w;
{
  struct ccite *cci = (struct ccite *) ip->priv;
  struct ccfb *fb = cci->fb;
  int y;

  for (y = sy; y < sy + h; y++)
    cc_8n_windowmove (0, 0, 0, 0,
    			cci->buf_rows[y].buf, sx * ip->ftwidth, 0,
    		        fb->fb_width >> 3,
    		        ip->ftheight, w * ip->ftwidth, RR_CLEAR);
}

/* Note: sx is only relevant for SCROLL_LEFT or SCROLL_RIGHT.  */
static void
cc_8n_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
  struct ccite *cci = (struct ccite *) ip->priv;

  if (dir == SCROLL_UP) 
    {
      int dy = sy - count;
      int bot = ip->inside_margins ? ip->bottom_margin : ip->rows - 1;
      int height = bot - sy + 1;
      int i;

      for (i = 0; i < height; i++)
	swap_rows(ip, sy + i, dy + i);
    }
  else if (dir == SCROLL_DOWN) 
    {
      int dy = sy + count;
      int bot = ip->inside_margins ? ip->bottom_margin : ip->rows - 1;
      int height = bot - dy + 1;
      int i;

      for (i = (height - 1); i >= 0; i--)
	swap_rows(ip, sy + i, dy + i);
    }
  else if (dir == SCROLL_RIGHT) 
    {
      struct ccfb *fb = cci->fb;

      cc_8n_cursor(ip, ERASE_CURSOR);
      cc_8n_windowmove(cci->buf_rows[sy].buf,
			 sx * ip->ftwidth, 0, fb->fb_width >> 3,
			 cci->buf_rows[sy].buf,
			 (sx + count) * ip->ftwidth, 0, fb->fb_width >> 3,
			 ip->ftheight, (ip->cols - (sx + count)) * ip->ftwidth, RR_COPY);
    }
  else 
    {
      struct ccfb *fb = cci->fb;

      cc_8n_cursor(ip, ERASE_CURSOR);
      cc_8n_windowmove(cci->buf_rows[sy].buf,
			 sx * ip->ftwidth, 0, fb->fb_width >> 3,
			 cci->buf_rows[sy].buf,
			 (sx - count) * ip->ftwidth, 0, fb->fb_width >> 3,
			 ip->ftheight, (ip->cols - sx) * ip->ftwidth, RR_COPY);
    }		
}



/*** (M<8)-by-N routines ***/

/* NOTE: This routine assumes a cursor overlay plane,
   but it does allow cursors up to 32 pixels wide.  */
static void
cc_le32n_cursor(struct ite_softc *ip, int flag)
{
  struct ccite *cci = (struct ccite *) ip->priv;
  struct ccfb *fb = cci->fb;
  /* the cursor is always drawn in the last plane */
  unsigned char *ovplane, opclr, opset;
  
  if (flag == START_CURSOROPT || flag == END_CURSOROPT)
    return;

  ovplane = fb->fb + (fb->fb_z - 1) * (fb->fb_planesize);

  if (flag != DRAW_CURSOR)
    {
      /* erase the cursor */
      u_char *pl = ovplane + ((fb->fb_y + ip->cursory * ip->ftheight) * (fb->fb_width >> 3));
      int ofs = fb->fb_x + ip->cursorx * ip->ftwidth;
      int h;

      for (h = ip->ftheight-1; h >= 0; h--)
	{
	  asm("bfclr %0@{%1:%2}"
	      : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
	  pl += fb->fb_width >> 3;
	}
    }
  if (flag == DRAW_CURSOR || flag == MOVE_CURSOR)
    {
      u_char *pl;
      int ofs, h;

      /* store the position */
      ip->cursorx = MIN(ip->curx, ip->cols-1);
      ip->cursory = ip->cury;

      /* draw the cursor */
      pl = ovplane + ((fb->fb_y + ip->cursory * ip->ftheight) * (fb->fb_width >> 3));
      ofs = fb->fb_x + ip->cursorx * ip->ftwidth;

      for (h = ip->ftheight-1; h >= 0; h--)
	{
	  asm("bfset %0@{%1:%2}"
	      : : "a" (pl), "d" (ofs), "d" (ip->ftwidth));
	  pl += fb->fb_width >> 3;
	}
    }
}

static void
cc_le8n_putc(struct ite_softc *ip, int c, int dy, int dx, int mode)
{
  if (c >= ip->font_lo && c <= ip->font_hi)
    {
      struct ccite *cci = (struct ccite *) ip->priv;
      struct ccfb *fb = cci->fb;
      u_char *pl = cci->buf_rows[dy].buf;
      int ofs = dx * ip->ftwidth;
      u_char *fontp = ip->font + (c - ip->font_lo) * ip->ftheight;
      int h;

      if (mode != ATTR_INV)
	{
          for (h = ip->ftheight-1; h >= 0; h--)
	    {
	      asm("bfins %3,%0@{%1:%2}"
	          : : "a" (pl), "d" (ofs), "d" (ip->ftwidth), "d" (*fontp++));
	      pl += fb->fb_width >> 3;
	    }
	}
      else
	{
          for (h = ip->ftheight-1; h >= 0; h--)
	    {
	      asm("bfins %3,%0@{%1:%2}"
	          : : "a" (pl), "d" (ofs), "d" (ip->ftwidth), "d" (~(*fontp++)));
	      pl += fb->fb_width >> 3;
	    }
	}
    }
}

static void
cc_le8n_clear(struct ite_softc *ip, int sy, int sx, int h, int w)
{
  struct ccite *cci = (struct ccite *) ip->priv;
  struct ccfb *fb = cci->fb;

  if ((sx == 0) && (w == ip->cols))
    {
      /* common case: clearing whole lines */
      while (h--)
	{
          bzero(cci->buf_rows[sy].buf, (fb->fb_width >> 3) * ip->ftheight);
	  sy++;
	}
    }
  else
    {
      /* clearing only part of a line */
      /* XXX could be optimized MUCH better, but is it worth the trouble? */
      while (h--)
	{
	  u_char *pl = cci->buf_rows[sy].buf;
          int ofs = sx * ip->ftwidth;
	  int i, j;
	  for (i = w-1; i >= 0; i--)
	    {
	      u_char *ppl = pl;
              for (j = ip->ftheight-1; j >= 0; j--)
	        {
	          asm("bfclr %0@{%1:%2}"
	              : : "a" (ppl), "d" (ofs), "d" (ip->ftwidth));
	          ppl += fb->fb_width >> 3;
	        }
	      ofs += ip->ftwidth;
	    }
	  sy++;
	}
    }
}

/* Note: sx is only relevant for SCROLL_LEFT or SCROLL_RIGHT.  */
static void
cc_le8n_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
  if (dir == SCROLL_UP) 
    {
      int dy = sy - count;
      int bot = ip->inside_margins ? ip->bottom_margin : ip->rows - 1;
      int height = bot - sy + 1;
      int i;

      for (i = 0; i < height; i++)
	swap_rows(ip, sy + i, dy + i);
    }
  else if (dir == SCROLL_DOWN) 
    {
      int dy = sy + count;
      int bot = ip->inside_margins ? ip->bottom_margin : ip->rows - 1;
      int height = bot - dy + 1;
      int i;

      for (i = (height - 1); i >= 0; i--)
	swap_rows(ip, sy + i, dy + i);
    }
  else if (dir == SCROLL_RIGHT) 
    {
      struct ccite *cci = (struct ccite *) ip->priv;
      struct ccfb *fb = cci->fb;
      u_char *pl = cci->buf_rows[sy].buf;
      int sofs = (ip->cols - count) * ip->ftwidth;
      int dofs = (ip->cols) * ip->ftwidth;
      int i, j;

      cc_le32n_cursor(ip, ERASE_CURSOR);
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
	  pl += fb->fb_width >> 3;
	}
    }
  else /* SCROLL_LEFT */
    {
      struct ccite *cci = (struct ccite *) ip->priv;
      struct ccfb *fb = cci->fb;
      u_char *pl = cci->buf_rows[sy].buf;
      int sofs = (sx) * ip->ftwidth;
      int dofs = (sx - count) * ip->ftwidth;
      int i, j;

      cc_le32n_cursor(ip, ERASE_CURSOR);
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
	  pl += fb->fb_width >> 3;
	}
    }		
}

#endif
