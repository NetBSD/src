#include "ite.h"
#if NITE > 0

#include "param.h"
#include "conf.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "itevar.h"

#include "machine/cpu.h"

/* XXX */
#include "grfioctl.h"
#include "grfvar.h"
#include "grf_ccreg.h"

extern unsigned char kernel_font_width, kernel_font_height;
extern unsigned char kernel_font_lo, kernel_font_hi;
extern unsigned char kernel_font[], kernel_cursor[];


customc_init(ip)
	register struct ite_softc *ip;
{
  struct ccfb *fb;
  int fboff, fbsize;

  if (ip->grf == 0)
    ip->grf = &grf_softc[ip - ite_softc];

  ip->priv = ip->grf->g_display.gd_regaddr;
  fb = (struct ccfb *) ip->priv;
  fbsize = ip->grf->g_display.gd_fbsize;
  
#if 0
  /* already done in the grf layer */

  /* clear the display. bzero only likes regions up to 64k, so call multiple times */
  for (fboff = 0; fboff < fbsize; fboff += 64*1024)
    bzero (fb->fb + fboff, fbsize - fboff > 64*1024 ? 64*1024 : fbsize - fboff);
#endif

  /* this is a dreadful font, but I don't have an alternative at the moment.. */
  ip->font     = kernel_font;
  ip->font_lo  = kernel_font_lo;
  ip->font_hi  = kernel_font_hi;
  ip->ftwidth  = kernel_font_width;
  ip->ftheight = kernel_font_height;
  ip->cursor   = kernel_cursor;
  
  ip->rows     = fb->disp_height / ip->ftheight;
  ip->cols     = fb->disp_width  / ip->ftwidth;
  
  
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
}


void static inline
customc_windowmove (src, srcx, srcy, srcmod,
		    dst, dstx, dsty, dstmod, h, w, op)
    unsigned char *src, *dst;
    unsigned short srcx, srcy, srcmod;
    unsigned short dstx, dsty, dstmod;
    unsigned short h, w;
    unsigned char op;
{
  int i;

  src += srcmod*srcy + (srcx >> 3);
  dst += dstmod*dsty + (dstx >> 3);

#if 0
printf("ccwm: %x-%x-%x-%x-%c\n", src, dst, h, w,
	op == RR_XOR ? '^' : op == RR_COPY ? '|' : op == RR_CLEAR ? 'C' : 'I');
#endif
  
  /* currently, only drawing to byte slots is supported... */
  if ((srcx & 07) || (dstx & 07) || (w & 07))
    panic ("customc_windowmove: odd offset");
    
  w >>= 3;
  while (h--)
    {
      if (src > dst)
	for (i = 0; i < w; i++)
	  switch (op)
	    {
	    case RR_COPY:
	      dst[i] = src[i];
	      break;
	    
	    case RR_CLEAR:
	      dst[i] = 0;
	      break;
	    
	    case RR_XOR:
	      dst[i] ^= src[i];
	      break;
	    
	    case RR_COPYINVERTED:
	      dst[i] = ~src[i];
	      break;
	    }
      else
	for (i = w - 1; i >= 0; i--)
	  switch (op)
	    {
	    case RR_COPY:
	      dst[i] = src[i];
	      break;
	    
	    case RR_CLEAR:
	      dst[i] = 0;
	      break;
	    
	    case RR_XOR:
	      dst[i] ^= src[i];
	      break;
	    
	    case RR_COPYINVERTED:
	      dst[i] = ~src[i];
	      break;
	    }

	  
      src += srcmod;
      dst += dstmod;
    }

}


customc_putc(ip, c, dy, dx, mode)
	register struct ite_softc *ip;
        register int dy, dx;
	int c, mode;
{
  register int wrr = ((mode == ATTR_INV) ? RR_COPYINVERTED : RR_COPY);
  struct ccfb *fb = (struct ccfb *) ip->priv;

  if (c >= ip->font_lo && c <= ip->font_hi)
    {
      c -= ip->font_lo;

      customc_windowmove (ip->font, 0, c * ip->ftheight, 1,
    			  fb->fb, fb->fb_x + dx * ip->ftwidth,
    			  fb->fb_y + dy * ip->ftheight,
    			  fb->fb_width >> 3,
    			  ip->ftheight, ip->ftwidth, wrr);
    }
}

customc_cursor(ip, flag)
	register struct ite_softc *ip;
        register int flag;
{
  struct ccfb *fb = (struct ccfb *) ip->priv;

  if (flag != DRAW_CURSOR)
    {
      /* erase it */
      customc_windowmove (ip->cursor, 0, 0, 1,
	  		  fb->fb, fb->fb_x + ip->cursorx * ip->ftwidth,
    			  fb->fb_y + ip->cursory * ip->ftheight,
    			  fb->fb_width >> 3,
    			  ip->ftheight, ip->ftwidth, RR_XOR);
    }
  if (flag == DRAW_CURSOR || flag == MOVE_CURSOR)
    {
      /* draw it */
      customc_windowmove (ip->cursor, 0, 0, 1,
	    		  fb->fb, fb->fb_x + ip->curx * ip->ftwidth,
    			  fb->fb_y + ip->cury * ip->ftheight,
    			  fb->fb_width >> 3,
    			  ip->ftheight, ip->ftwidth, RR_XOR);
      ip->cursorx = ip->curx;
      ip->cursory = ip->cury;
    }
}

customc_clear(ip, sy, sx, h, w)
	struct ite_softc *ip;
	register int sy, sx, h, w;
{
  struct ccfb *fb = (struct ccfb *) ip->priv;

  customc_windowmove (0, 0, 0, 0,
  		      fb->fb, fb->fb_x + sx * ip->ftwidth,
    		      fb->fb_y + sy * ip->ftheight,
    		      fb->fb_width >> 3,
    		      h * ip->ftheight, w * ip->ftwidth, RR_CLEAR);
}

customc_blockmove(ip, sy, sx, dy, dx, h, w)
	register struct ite_softc *ip;
	int sy, sx, dy, dx, h, w;
{
  struct ccfb *fb = (struct ccfb *) ip->priv;

  customc_windowmove(fb->fb, fb->fb_x + sx * ip->ftwidth,
		     fb->fb_y + sy * ip->ftheight, 
		     fb->fb_width >> 3,
		     fb->fb, fb->fb_x + dx * ip->ftwidth,
		     fb->fb_y + dy * ip->ftheight,
		     fb->fb_width >> 3,
		     h * ip->ftheight, w * ip->ftwidth, RR_COPY);
}

customc_scroll(ip, sy, sx, count, dir)
        register struct ite_softc *ip;
        register int sy;
        int dir, sx, count;
{
  register int height, dy, i;
	
  customc_cursor(ip, ERASE_CURSOR);

  if (dir == SCROLL_UP) 
    {
      dy = sy - count;
      height = ip->bottom_margin - sy + 1;
      for (i = 0; i < height; i++)
	customc_blockmove(ip, sy + i, sx, dy + i, 0, 1, ip->cols);
    }
  else if (dir == SCROLL_DOWN) 
    {
      dy = sy + count;
      height = ip->bottom_margin - dy + 1;
      for (i = (height - 1); i >= 0; i--)
	customc_blockmove(ip, sy + i, sx, dy + i, 0, 1, ip->cols);
    }
  else if (dir == SCROLL_RIGHT) 
    {
      customc_blockmove(ip, sy, sx, sy, sx + count, 1, ip->cols - (sx + count));
    }
  else 
    {
      customc_blockmove(ip, sy, sx, sy, sx - count, 1, ip->cols - sx);
    }		
}

#endif
