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
#include "grf_rtreg.h"

void retina_init(struct ite_softc *ip)
{
  struct MonDef *md;

  if (ip->grf == 0)
    ip->grf = &grf_softc[ip - ite_softc];

  ip->priv = ip->grf->g_data;
  md = (struct MonDef *) ip->priv;
  
  ip->cols = md->TX;
  ip->rows = md->TY;
}


void retina_cursor(struct ite_softc *ip, int flag)
{
      volatile u_char *ba = ip->grf->g_regkva;
      
      if (flag == ERASE_CURSOR)
        {
	  /* disable cursor */
          WCrt (ba, CRT_ID_CURSOR_START, RCrt (ba, CRT_ID_CURSOR_START) | 0x20);
        }
      else
	{
	  int pos = ip->curx + ip->cury * ip->cols;

	  /* make sure to enable cursor */
          WCrt (ba, CRT_ID_CURSOR_START, RCrt (ba, CRT_ID_CURSOR_START) & ~0x20);

	  /* and position it */
	  WCrt (ba, CRT_ID_CURSOR_LOC_HIGH, (u_char) (pos >> 8));
	  WCrt (ba, CRT_ID_CURSOR_LOC_LOW,  (u_char) pos);

	  ip->cursorx = ip->curx;
	  ip->cursory = ip->cury;
	}
}



static void screen_up (struct ite_softc *ip, int top, int bottom, int lines)
{	
	volatile u_char * ba = ip->grf->g_regkva;
	volatile u_char * fb = ip->grf->g_fbkva;
	const struct MonDef * md = (struct MonDef *) ip->priv;

	/* do some bounds-checking here.. */
	if (top >= bottom)
	  return;
	  
	if (top + lines >= bottom)
	  {
	    retina_clear (ip, top, 0, bottom - top, ip->cols);
	    return;
	  }


	/* the trick here is to use a feature of the NCR chip. It can
	   optimize data access in various read/write modes. One of
	   the modes is able to read/write from/to different zones.

	   Thus, by setting the read-offset to lineN, and the write-offset
	   to line0, we just cause read/write cycles for all characters
	   up to the last line, and have the chip transfer the data. The
	   `addqb' are the cheapest way to cause read/write cycles (DONT
	   use `tas' on the Amiga!), their results are completely ignored
	   by the NCR chip, it just replicates what it just read. */
	
		/* write to primary, read from secondary */
	WSeq (ba, SEQ_ID_EXTENDED_MEM_ENA, (RSeq(ba, SEQ_ID_EXTENDED_MEM_ENA) & 0x1f) | 0 ); 
		/* clear extended chain4 mode */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR, RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) & ~0x02);  
	
		/* set write mode 1, "[...] data in the read latches is written
		   to memory during CPU memory write cycles. [...]" */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 1); 
	
	{
		/* write to line TOP */	
		long toploc = top * (md->TX / 16);
		WSeq (ba, SEQ_ID_PRIM_HOST_OFF_LO, ((unsigned char)toploc)); 
		WSeq (ba, SEQ_ID_PRIM_HOST_OFF_HI, ((unsigned char)(toploc >> 8))); 
	}
	{
		/* read from line TOP + LINES */
		long fromloc = (top+lines) * (md->TX / 16);
		WSeq (ba, SEQ_ID_SEC_HOST_OFF_LO, ((unsigned char)fromloc)) ; 
		WSeq (ba, SEQ_ID_SEC_HOST_OFF_HI, ((unsigned char)(fromloc >> 8))) ; 
	}
	{
		unsigned char * p = (unsigned char *) fb;
		/* transfer all characters but LINES lines, unroll by 16 */
		short x = (1 + bottom - (top + lines)) * (md->TX / 16) - 1;
		do {
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@+" : "=a" (p) : "0" (p)); 
		} while (x--);
	}
	
		/* reset to default values */
	WSeq (ba, SEQ_ID_SEC_HOST_OFF_HI, 0); 
	WSeq (ba, SEQ_ID_SEC_HOST_OFF_LO, 0); 
	WSeq (ba, SEQ_ID_PRIM_HOST_OFF_HI, 0); 
	WSeq (ba, SEQ_ID_PRIM_HOST_OFF_LO, 0); 
		/* write mode 0 */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 0);
		/* extended chain4 enable */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR , RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) | 0x02);  
		/* read/write to primary on A0, secondary on B0 */
	WSeq (ba, SEQ_ID_EXTENDED_MEM_ENA, (RSeq(ba, SEQ_ID_EXTENDED_MEM_ENA) & 0x1f) | 0x40 ); 
	
	
	/* fill the free lines with spaces */
	
	{  /* feed latches with value */
		unsigned short * f = (unsigned short *) fb;
		
		f += (1 + bottom - lines) * md->TX * 2;
		*f = 0x2010;
		{
			volatile unsigned short dummy = *((volatile unsigned short *)f);
		}
	}
	
	   /* clear extended chain4 mode */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR, RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) & ~0x02);  
	   /* set write mode 1, "[...] data in the read latches is written
	      to memory during CPU memory write cycles. [...]" */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 1); 
	
	{
		unsigned long * p = (unsigned long *) fb;
		short x = (lines * (md->TX/16)) - 1;
		const unsigned long dummyval = 0;
		
		p += (1 + bottom - lines) * (md->TX/4);
		
		do {
			*p++ = dummyval;
			*p++ = dummyval;
			*p++ = dummyval;
			*p++ = dummyval;
		} while (x--);
	}
	
	   /* write mode 0 */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 0);
	   /* extended chain4 enable */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR , RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) | 0x02);  
	
};

static void screen_down (struct ite_softc *ip, int top, int bottom, int lines)
{	
	volatile u_char * ba = ip->grf->g_regkva;
	volatile u_char * fb = ip->grf->g_fbkva;
	const struct MonDef * md = (struct MonDef *) ip->priv;

	/* do some bounds-checking here.. */
	if (top >= bottom)
	  return;
	  
	if (top + lines >= bottom)
	  {
	    retina_clear (ip, top, 0, bottom - top, ip->cols);
	    return;
	  }

	/* see screen_up() for explanation of chip-tricks */

		/* write to primary, read from secondary */
	WSeq (ba, SEQ_ID_EXTENDED_MEM_ENA, (RSeq(ba, SEQ_ID_EXTENDED_MEM_ENA) & 0x1f) | 0 ); 
		/* clear extended chain4 mode */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR, RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) & ~0x02);  
	
		/* set write mode 1, "[...] data in the read latches is written
		   to memory during CPU memory write cycles. [...]" */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 1); 
	
	{
		/* write to line TOP + LINES */	
		long toloc = (top + lines) * (md->TX / 16);
		WSeq (ba, SEQ_ID_PRIM_HOST_OFF_LO, ((unsigned char)toloc)); 
		WSeq (ba, SEQ_ID_PRIM_HOST_OFF_HI, ((unsigned char)(toloc >> 8))); 
	}
	{
		/* read from line TOP */
		long fromloc = top * (md->TX / 16);
		WSeq (ba, SEQ_ID_SEC_HOST_OFF_LO, ((unsigned char)fromloc)); 
		WSeq (ba, SEQ_ID_SEC_HOST_OFF_HI, ((unsigned char)(fromloc >> 8))) ; 
	}
	
	{
		unsigned char * p = (unsigned char *) fb;
		short x = (1 + bottom - (top + lines)) * (md->TX / 16) - 1;
		p += (1 + bottom - (top + lines)) * md->TX;
		do {
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
			asm volatile("addqb #1,%0@-" : "=a" (p) : "0" (p)); 
		} while (x--);
	}
	
	WSeq (ba, SEQ_ID_PRIM_HOST_OFF_HI, 0); 
	WSeq (ba, SEQ_ID_PRIM_HOST_OFF_LO, 0); 
	WSeq (ba, SEQ_ID_SEC_HOST_OFF_HI, 0); 
	WSeq (ba, SEQ_ID_SEC_HOST_OFF_LO, 0); 
	
		/* write mode 0 */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 0);
		/* extended chain4 enable */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR , RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) | 0x02);  
		/* read/write to primary on A0, secondary on B0 */
	WSeq (ba, SEQ_ID_EXTENDED_MEM_ENA, (RSeq(ba, SEQ_ID_EXTENDED_MEM_ENA) & 0x1f) | 0x40 ); 
	
	/* fill the free lines with spaces */
	
	{  /* feed latches with value */
		unsigned short * f = (unsigned short *) fb;
		
		f += top * md->TX * 2;
		*f = 0x2010;
		{
			volatile unsigned short dummy = *((volatile unsigned short *)f);
		}
	}
	
	   /* clear extended chain4 mode */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR, RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) & ~0x02);  
	   /* set write mode 1, "[...] data in the read latches is written
	      to memory during CPU memory write cycles. [...]" */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 1); 
	
	{
		unsigned long * p = (unsigned long *) fb;
		short x = (lines * (md->TX/16)) - 1;
		const unsigned long dummyval = 0;
		
		p += top * (md->TX/4);
		
		do {
			*p++ = dummyval;
			*p++ = dummyval;
			*p++ = dummyval;
			*p++ = dummyval;
		} while (x--);
	}
	
	   /* write mode 0 */
	WGfx (ba, GCT_ID_GRAPHICS_MODE, (RGfx(ba, GCT_ID_GRAPHICS_MODE) & 0xfc) | 0);
	   /* extended chain4 enable */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR , RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) | 0x02);  
	
};

void retina_deinit(struct ite_softc *ip)
{
  ip->flags &= ~ITE_INITED;
}


void retina_putc(struct ite_softc *ip, int c, int dy, int dx, int mode)
{
	volatile u_char * ba = ip->grf->g_regkva;
	volatile u_char * fb = ip->grf->g_fbkva;
	register u_char attr;
	
	attr = (mode & ATTR_INV) ? 0x21 : 0x10;
	if (mode & ATTR_UL)     attr  = 0x01;	/* ???????? */
	if (mode & ATTR_BOLD)   attr |= 0x08;
	if (mode & ATTR_BLINK)	attr |= 0x80;
	
	fb += 4 * (dy * ip->cols + dx);
	*fb++ = c; *fb = attr;
}

void retina_clear(struct ite_softc *ip, int sy, int sx, int h, int w)
{
	volatile u_char * ba = ip->grf->g_regkva;
	u_short * fb = (u_short *) ip->grf->g_fbkva;
	short x;
	const u_short fillval = 0x2010;
	/* could probably be optimized just like the scrolling functions !! */
	fb += 2 * (sy * ip->cols + sx);
	while (h--)
	  {
	    for (x = 2 * (w - 1); x >= 0; x -= 2)
	      fb[x] = fillval;
	    fb += 2 * ip->cols;
	  }
}

void retina_scroll(struct ite_softc *ip, int sy, int sx, int count, int dir)
{
  volatile u_char * ba = ip->grf->g_regkva;
  u_long * fb = (u_short *) ip->grf->g_fbkva;
  register int height, dy, i;
	
  retina_cursor(ip, ERASE_CURSOR);

  if (dir == SCROLL_UP) 
    {
      screen_up (ip, sy - count, ip->bottom_margin, count);
      /* bcopy (fb + sy * ip->cols, fb + (sy - count) * ip->cols, 4 * (ip->bottom_margin - sy + 1) * ip->cols); */
      /* retina_clear (ip, ip->bottom_margin + 1 - count, 0, count, ip->cols); */
    }
  else if (dir == SCROLL_DOWN)
    {
      screen_down (ip, sy, ip->bottom_margin, count);
      /* bcopy (fb + sy * ip->cols, fb + (sy + count) * ip->cols, 4 * (ip->bottom_margin - sy - count + 1) * ip->cols); */
      /* retina_clear (ip, sy, 0, count, ip->cols); */
    }
  else if (dir == SCROLL_RIGHT) 
    {
      bcopy (fb + sx + sy * ip->cols, fb + sx + sy * ip->cols + count, 4 * (ip->cols - (sx + count)));
      retina_clear (ip, sy, sx, 1, count);
    }
  else 
    {
      bcopy (fb + sx + sy * ip->cols, fb + sx - count + sy * ip->cols, 4 * (ip->cols - sx));
      retina_clear (ip, sy, ip->cols - count, 1, count);
    }		
}

#endif



