/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * The console device driver for Alice.
 * $Id: console.c,v 1.5 1993/12/21 03:16:01 briggs Exp $
 *
 * April 11th, 1992 LK
 *  Original
 * August 20-Sep 9, 1993 MF
 *  Added support for nubus, multiple monitors
 *  Added different width screen info
 *  ability to scroll on two monitors
 *  ioctls to move monitors
 * September 19th, 1993 LK
 *  Added multi-font support
 *  Integrated AKB 19 Sept.
 * September 28th-ish, 1993 LK
 *  Added reversepixel and updated reversecursor.
 *  Integrated AKB 1 Oct.
 * October 12th, 1993 AKB
 *  Fixed bugs in 6-bit code.  Also changed x,y in VT
 *  structure to SIGNED ints because of VT100 parsing...
 *  (see ESC [ ; H, e.g.)
 * October 14th, 1993 AKB
 *  Added support for T_REVERSE and T_UNDERLINE
 *  for drawweirdcharacter.
 */

/* Received from MacBSDBoot, stored by Locore: */
long videoaddr;
long videorowbytes;
long videobitdepth;
char serial_boot_echo=0;

#include "8x14.h"
#include "6x10.h"

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <types.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include "device.h"
#include "../mac68k/cons.h"
#include "../mac68k/myframe.h"
#include "serreg.h"
#include "console.h"

#include "asc.h" /* for sound */

#include "grf.h"
#include "grfioctl.h"
#include "grfvar.h"

extern struct grf_softc grf_softc[NGRF];

#define NFONT 2
#define OUTBUFLEN 128
#define CONBUFLEN 128
#define SCROLLBACK 16384  /* Size of scrollback buffer */
#include "con.h"
#define CL *(to++)=*(from++)

struct font {
  int width;			/* In pixels				*/
  int height;			/* In pixels				*/
  unsigned char *normalfont;	/* Pointer to bitmap for font		*/
  unsigned char *boldfont;	/* Pointer to bitmap for bold font	*/
  int sevenbit;			/* True if the font only has 0..127	*/
};

struct vt {
  int x;			/* Cursor position [0..numtcols-1]	*/
  int y;			/* Cursor position [0..numtrows-1]	*/
  unsigned int oldx, oldy;	/* Saved cursor position		*/
  unsigned int attr, fgcolor, bgcolor;  /* Current attribute & color	*/
  unsigned int hanging_cursor;	/* Cursor at last column		*/
  unsigned int vt100;		/* Status/length of vt100 sequence	*/
  unsigned char vtstr[200];	/* vt100 sequence			*/
  unsigned char scr[50][132];	/* Contents of screen			*/
  unsigned char att[50][132];	/* Attributes of screen			*/
  unsigned char sb[SCROLLBACK];	/* Scrollback buffer			*/
  unsigned int sbhead, sbtail, sblen; /* Scrollback is ring buffer	*/

  unsigned int numtcols;	/* # of text columns on the screen	*/
  unsigned int numtrows;	/* # of text lines on the screen	*/
  unsigned int toptrow;		/* Top line of text scroll region	*/
  unsigned int bottrow;		/* Bottom line of text scroll region	*/
  unsigned int numgcols;	/* # of visible pixels across		*/
  unsigned int numgrows;	/* # of visible pixels down		*/
  unsigned int linelen;		/* # of bytes/line (incl. non-visible)	*/
  unsigned int numbits;		/* # of bits per pixel			*/
  unsigned char *screen;	/* Pointer to the screen		*/
  struct font *font;		/* Current font for this vt		*/
  int visible;			/* Is this vt visible (on a screen) ?	*/
};

extern long videoaddr; /* in srt0.c */
unsigned long videosize;
extern int gNumGrfDev;

extern long videoaddr; /* in srt0.c */
static unsigned char outbuf[OUTBUFLEN];
static unsigned char conbuf[OUTBUFLEN];
static int outhead = 0, outtail = 0, outlen = 0, write_ack = 0;
static int conhead = 0, contail = 0, conlen = 0, do_conintr = 0, intr_enabled = 0;
static unsigned char *sccaddr = (unsigned char *)0x50004000;
static struct vt vt[NCON];
static struct font font[NFONT]; /* for now, 0 = large, 1 = small */
int curvt; /* Current virtual terminal -- Used by adb */
int numsb;  /* Number of lines scrolling back -- Used by adb */
static int cursoron, cursorlit, cursortype = C_BLOCK | C_SOLID;

void	macconputchar(int, u_char);

static getvideoparams()
{
  /* Get these from hardware and set video to 1 bit if possible */
	int i=0;

  /* numgcols=640; */
  /* numgrows=480; */

  for(i=0;i<NCON;i++)
  {
	  vt[i].numgcols=videosize & 0xffff;
	  vt[i].numgrows=(videosize >>16) & 0xffff;

	  vt[i].numbits=videobitdepth;
	  vt[i].linelen=videorowbytes;
	  vt[i].screen = (unsigned char *)videoaddr;
  }
  vt[0].visible=1;
}

static putpixel(struct vt *v, int xx, int yy, int c)
{
  unsigned int mask, i;

  mask = (1<<v->numbits)-1;
  c &= mask;
  switch (v->numbits)
  {
    case 1: i = 7 - (xx & 7);
            c <<= i;
            mask <<= i;
            i = yy * v->linelen + (xx>>3);
            v->screen[i] &= ~mask;
            v->screen[i] |= c;
            break;
    case 2: i = 6 - ((xx & 3) << 1);
            c <<= i;
            mask <<= i;
            i = yy * v->linelen + (xx>>2);
            v->screen[i] &= ~mask;
            v->screen[i] |= c;
            break;
    case 4: i = 4 - ((xx & 1) << 2);
	    c <<= i;
	    mask <<= i;
	    i = yy * v->linelen + (xx>>1);
	    v->screen[i] &= ~mask;
	    v->screen[i] |= c;
            break;
    case 8: v->screen[yy * v->linelen + xx] = c; 
	    break;
  }
}

static void reversepixel (struct vt *v, unsigned int xx, unsigned int yy)
{
  unsigned int i;

  switch (v->numbits)
  {
    case 1: i = yy * v->linelen + (xx / 8);
            v->screen[i] ^= 0x80 >> (xx % 8);
            break;
    case 2: i = yy * v->linelen + (xx / 4);
            v->screen[i] ^= 0xC0 >> ((xx % 4) * 2);
            break;
    case 4: i = yy * v->linelen + (xx / 2);
            v->screen[i] ^= 0xF0 >> ((xx % 2) * 4);
            break;
    case 8: v->screen[yy * v->linelen + xx] ^= 0xFF; 
            break;
  }
}

static reversecursor(int vtnum)
{
  unsigned int i, p, linelongs, px, py, offset, plx;
  int x, y;
  struct vt *v = &vt[vtnum];
  register unsigned long *sc;
  unsigned long mask;
  static int	__p=0;

  if (v->y + numsb >= v->numtrows)
  {
    cursorlit = 0;
    return;
  }

  cursorlit = !cursorlit;

  px = v->x * v->font->width;
  linelongs = v->linelen/4;

  switch (v->numbits)
  {
    case 1:
      plx = px & ~31;
      if (((px + v->font->width - 1) & ~31) > plx)
        plx += 16;
      p = (v->y+numsb) * v->font->height * v->linelen + plx/8;
      offset = 32 - v->font->width + (plx % 32) - (px % 32);
      mask = ((1L << v->font->width) - 1) << offset;
      if (cursortype & C_BLOCK)
      {
        sc = (unsigned long *)&(v->screen[p]);
        if (v->font->height == 14)
        {
#define RL *sc ^= mask; sc += linelongs;
          RL; RL; RL; RL; RL; RL; RL;
          RL; RL; RL; RL; RL; RL; RL;
#undef RL
        }
        else
        {
          for (i = 0; i < v->font->height; i++) {
            *sc ^= mask;
            sc += linelongs;
          }
        }
      }
      else
      {
        sc = (unsigned long *)&(v->screen[p+(v->font->height-2)*v->linelen]);
        *sc ^= mask;
        *(sc+linelongs) ^= mask;
      }
      break;
    case 2:
    case 4:
    case 8:
      /* Okay, this is a real quick hack so that people in non-1-bit mode
      will see a cursor.  It's real slow and should be fixed later. */
      px = v->x * v->font->width;
      py = v->y * v->font->height;
      for (y = v->font->height-1; y >= 0; y--) {
        for (x = v->font->width-1; x >= 0; x--) {
          reversepixel (v, x + px, y + py);
        }
      }
      break;
  }
}


static drawcursor(int vtnum)
{
  if (vtnum != curvt) return;
  if (!cursorlit) reversecursor(vtnum);
  cursoron = 1;
}

static erasecursor(int vtnum)
{
  if (!vt[vtnum].visible) return;
  if (cursorlit) reversecursor(vtnum);
  cursoron = 0;
}

static void cls(struct vt *v)
{
  register int i;
  register unsigned long *sc;

  for (sc = (unsigned long *)v->screen, i = v->linelen*v->numgrows/4; i; i--)
    *sc++ = 0;
}

static clearscreen(int vtnum, int which)
{
  /* which=0=to end of screen, 1=to beginning of screen, 2=all screen */

  int i, j, start, end;
  struct vt *v = &vt[vtnum];

  switch (which)
  {
    case 0: start = v->y;
            end = v->numtrows;
            break;
    case 1: start = 0;
            end = v->y;
            break;
    case 2: start = 0;
            end = v->numtrows;
            break;
  }
  for (i = start; i < end; i++)
    for (j = 0; j < v->numtcols; j++)
    {
      v->scr[i][j] = ' ';
      v->att[i][j] = T_NORMAL;
    }

  if (!v->visible )
    return;

  switch (which)
  {
    case 0: start=v->y*v->font->height*v->linelen; end=v->linelen*v->numgrows; break;
    case 1: start=0; end=v->y*v->font->height*v->linelen; break;
    case 2: cls(v); return;
  }
  for (i = start; i < end; i++)
    v->screen[i] = 0;
}

extern void conattach(int n)
{
static int	initt=0;
  int i;

if (initt) {
	return;
}
  initt = 1;
  getvideoparams();
  cursoron = 0;
  cursorlit = 0;

  font[0].width = 8;
  font[0].height = 14;
  font[0].normalfont = Font8x14;	/* In chars.h */
  font[0].boldfont = Font8x14B;		/* In chars.h */
  font[0].sevenbit = 0;
  font[1].width = 6;
  font[1].height = 10;
  font[1].normalfont = Font6x10;	/* In 6x10.h */
  font[1].boldfont = Font6x10;		/* Same font for now */
  font[1].sevenbit = 1;

  curvt = 0; /* Start at vt 0 */
  for (i = 0; i < NCON; i++)
  {
    vt[i].x = 0;
    vt[i].y = 0;
    vt[i].oldx = 0;
    vt[i].oldy = 0;
    vt[i].attr = T_NORMAL;
    vt[i].fgcolor = 37;
    vt[i].bgcolor = 40;
    vt[i].hanging_cursor = 0;
    vt[i].vt100 = 0;
    vt[i].vtstr[0] = 0;
    vt[i].sbhead = 0;
    vt[i].sbtail = 0;
    vt[i].sblen = 0;
    /* BARF: Here should check if SE/30 and set it to font[1] as default: */
    if (vt[i].numgcols >= 640) 
      vt[i].font = &font[0];
    else
      vt[i].font = &font[1];
    vt[i].numtcols = vt[i].numgcols/vt[i].font->width;
    vt[i].numtrows = vt[i].numgrows/vt[i].font->height;
    vt[i].toptrow = 1;
    vt[i].bottrow = vt[i].numtrows;
    if (i==0) {
    	vt[i].visible=1;
    }
    else
    	vt[i].visible=0;

    clearscreen(i,2);
  }

  drawcursor(curvt);
}

static void flashscreen(int vtnum)
{
  register int i;
  register unsigned long *sc;
  struct vt *v=&vt[vtnum];

  if (!v->visible )
    return;
  
  for (sc = (unsigned long *)v->screen, i = v->linelen*v->numgrows/4; i; i--)
    *sc++ ^= 0xFFFFFFFF;
  for (sc = (unsigned long *)v->screen, i = v->linelen*v->numgrows/4; i; i--)
    *sc++ ^= 0xFFFFFFFF;
}

static beep(int vtnum)
{
  struct vt *v=&vt[vtnum];

  if (v->visible) asc_ringbell();
}

static void drawweirdcharacter (struct vt *v, int x, int y, int attr,
                                unsigned char *ca)
{
  /*
   * LAK: This function is for character sets that are not 8 pixels across.
   * Does GCC have inline functions?  If so, we should make this inline to
   * save a function call (it's only called from one place).
   */

  unsigned int px, py;
  signed int offset;
  unsigned long mask, attrmask;
  register unsigned long *sc, *savesc;
  unsigned char *saveca;
  struct font *font = v->font;

  /* Position of the upper-left hand of the character, in pixels: */
  px = x * font->width;
  py = y * font->height;

  sc = (unsigned long *) (v->screen + (px/32)*4 + py*v->linelen);

  /* offset is the distance (right of the char) to the next long boundary */
  offset = 31 - font->width - px % 32;

  /* e.g.: px = 25, font->width = 6, offset = 32-6-25 = 1 */
  /*   0         1         2         3        */
  /*   01234567890123456789012345678901234567 */
  /*   ^                        ****** ^      */
  /*   +------ Long Boundary ----------+      */

  /*
   * If offset < 0, then the character is split across long boundaries, and
   * we must do two writes.  I don't know how much more expensive a long
   * write is than a word or byte write, but if it's much more expensive, we
   * may want to check to see if the character can be written with one word
   * write or one byte write.
   */

  /*
   * The following loops should be optimized in assembly (eventually...).
   * Maybe the case for height = 10 should be unrolled.  A minimal
   * effort has been made to make it easy for the compiler to optimize this.
   */
  attrmask = (((unsigned long) 1 << (font->width+1)) - 1);
  if (offset >= 0) { /* Easy case first... */
    /* Thank goodness mac is big-endian... :-) */
    mask = ~((((unsigned long) 1 << font->width) - 1) << (offset+1));
    if (attr & T_REVERSE) {
      for (y = font->height; y > 0; y--) {
        *sc &= mask;
        *sc |= ((unsigned long)*ca ^ attrmask) << offset;
        ca++;
        sc += v->linelen/4;
      }
    } else {
      for (y = font->height; y > 0; y--) {
        *sc &= mask;
        *sc |= (unsigned long)*ca << offset;
        ca++;
        sc += v->linelen/4;
      }
    }
    if (attr & T_UNDERLINE) {
        sc -= v->linelen/4;
        *sc |= (attrmask << offset);
    }
  } else {
    /* First long... */
    offset = -offset;
    mask = ~((((unsigned long) 1 << font->width) - 1) >> (offset-1));
    savesc = sc;
    saveca = ca;
    if (attr & T_REVERSE) {
      for (y = font->height; y > 0; y--) {
        *sc &= mask;
        *sc |= ((unsigned long)*ca ^ attrmask) >> offset;
        ca++;
        sc += v->linelen/4;
      }
    } else {
      for (y = font->height; y > 0; y--) {
        *sc &= mask;
        *sc |= (unsigned long)*ca >> offset;
        ca++;
        sc += v->linelen/4;
      }
    }
    if (attr & T_UNDERLINE) {
        sc -= v->linelen/4;
        *sc |= (attrmask >> offset);
    }
    /* Second long... */
    offset = 32 - offset;
    mask = ~((((unsigned long) 1 << font->width) - 1) << offset);
    sc = savesc + 1;
    ca = saveca;
    if (attr & T_REVERSE) {
      for (y = font->height; y > 0; y--) {
        *sc &= mask;
        *sc |= ((unsigned long)*ca ^ attrmask) << offset;
        ca++;
        sc += v->linelen/4;
      }
    } else {
      for (y = font->height; y > 0; y--) {
        *sc &= mask;
        *sc |= (unsigned long)*ca << offset;
        ca++;
        sc += v->linelen/4;
      }
    }
    if (attr & T_UNDERLINE) {
        sc -= v->linelen/4;
        *sc |= (attrmask << offset);
    }
  }
}

static drawcharacter(struct vt *v, int x, int y, int attr, unsigned char c)
{
  register unsigned int i, j, width, height, linelen;
  register unsigned char *ca, *sc;
  struct font *font;

  font = v->font;

  width = font->width;
  height = font->height;
  linelen = v->linelen;

  if (font->sevenbit && c >= 128) {
    /* LAK: What should we do here?  Maybe c &= 0x7F. */
    c = ' ';
  }

  if (attr & T_BOLD)
    ca = &font->boldfont[c*(long)height];
  else
    ca = &font->normalfont[c*(long)height];

  /*
   * LAK: The fastest way to do this is to have a horrible tree of
   * possible cases.  If numbits != 1, then we put up each pixel (yuk).
   * Otherwise, if width != 8, then we have to do fancy shifting.  If
   * width == 8, then we can just stick it up there.  Also handle reverse
   * video and underlining.  Plus unroll common heights (10 and 14).
   */
  if (v->numbits == 1)
  {
    if (width == 8)
    {
      j = y * height * linelen + (x * width) / 8;
      if (attr & T_REVERSE)
      {
        for (i = 0; i < height; i++)
          v->screen[j + i * linelen] = ~ca[i];
      }
      else
      {
        if (height == 14) {
          /* Look at assembly to optimize this: */
          sc = &v->screen[j];
#define CB *sc = *ca++; sc += linelen
          CB; CB; CB; CB; CB; CB; CB;
          CB; CB; CB; CB; CB; CB; CB;
#undef CB
        } else {
          for (i = 0; i < height; i++)
            v->screen[j + i * linelen] = ca[i];
        }
      }
      if (attr & T_UNDERLINE)
        v->screen[j + (height - 1) * linelen] = 255;  /* ^= 255 ? */
    }
    else /* width != 8 */
    {
      drawweirdcharacter (v, x, y, attr, ca);
    }
  }
  else
  {
    for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
      {
        if (ca[i] & (1<<j))
          putpixel (v, (x+1)*width-j-1, y*height+i, 255);
        else
          putpixel (v, (x+1)*width-j-1, y*height+i, 0);
      }
    }
  }
}

void drawsb(int vtnum)
{
  register int x, y;
  register struct vt *v = &vt[vtnum];
  int len, head, i;

  if (numsb < 0) numsb = 0;

  /* Draw scroll back: */
  len = v->sblen;
  head = v->sbhead;
  i = numsb+1;
  while (i)
  {
    if (len == 0)
    {
      i--;
      break;
    }
    head--;
    if (head < 0) head = SCROLLBACK - 1;
    len--;
    if (v->sb[head] == '\n') i--;
    if (i == 0) head = (head + 1) % SCROLLBACK;
  }
  numsb -= i; /* Can't scroll back more than what's in buffer */
  for (y = 0; y < numsb && y < v->numtrows; y++)
  {
    for (x = 0; v->sb[head] != '\n'; x++)
    {
      drawcharacter(v,x, y, 0, v->sb[head]);
      head = (head + 1) % SCROLLBACK;
    }
    head = (head + 1) % SCROLLBACK; /* Skip \n */
  }

  /* Draw part of screen: */
  if (numsb < v->numtrows)
  {
    for (y = 0; y < v->numtrows - numsb; y++)
      for (x = 0; x < v->numtcols; x++)
        drawcharacter(v, x, y+numsb, v->att[y][x], v->scr[y][x]);
  }

  if (cursoron)
  {
    if (cursortype & (C_FAST|C_SLOW))
      cursorlit = 0;
    else if (cursorlit)
    {
      cursorlit = 1;
      reversecursor(vtnum);
    }
  }
}

static void restoresb(void)
{
  /* Turn off scrollback. */

  /* Don't do if numsb == 0 because this routine is called a lot */
  if (numsb)
  {
    numsb = 0;
    drawsb(curvt);
  }
}

static putcharacter(int vtnum, char c)
{
  struct vt *v = &vt[vtnum];

  v->scr[v->y][v->x] = c;
  v->att[v->y][v->x] = v->attr;

  if (v->visible )
    drawcharacter(v, v->x, v->y, v->attr, c);
}

#define splconsole	spl3

static scrollup(int vtnum)
{
  /* scrolls the screen up by one text line */

  register int i, j, x, maxx;
  register long *from, *to;
  register struct vt *v = &vt[vtnum];
  char tempbuf[129];
  int buflen, s;

  /* Save the top line: */
  for (j = 0; j < v->numtcols; j++)
    tempbuf[j] = v->scr[v->toptrow-1][j];
  tempbuf[j++] = '\n'; /* End of line */
  buflen = j;
  if (v->toptrow == 1) {
    s = splconsole(); /* Exclusive access to sblen */
    while (v->sblen + buflen > SCROLLBACK) {
      while (v->sb[v->sbtail] != '\n') {
	v->sbtail = (v->sbtail + 1) % SCROLLBACK;
     	v->sblen--;
      }
      v->sbtail = (v->sbtail + 1) % SCROLLBACK;
      v->sblen--;
    }
    for (i = 0; i < buflen; i++) {
      v->sb[v->sbhead] = tempbuf[i];
      v->sbhead = (v->sbhead + 1) % SCROLLBACK;
      v->sblen++;
    }
    splx(s);
  }

  /* Scroll: */
  for (i = v->toptrow-1; i < v->bottrow-1; i++)
    for (j = 0; j < v->numtcols; j++)
    {
      v->scr[i][j] = v->scr[i+1][j];
      v->att[i][j] = v->att[i+1][j];
    }
  for (j = 0; j < v->numtcols; j++)
  {
    v->scr[v->bottrow-1][j] = 32;
    v->att[v->bottrow-1][j] = T_NORMAL;
  }

  if (!v->visible)
    return;

  from = (long *)&v->screen[v->toptrow*v->font->height * v->linelen];
  to = (long *)&v->screen[(v->toptrow-1)*v->font->height * v->linelen];
  maxx = v->numbits*(v->numgcols+31)/32;
  j = v->linelen / 4 - maxx;
  for (i = (v->bottrow-v->toptrow)*v->font->height; i; i--)
  {
    int depth;
    for (depth = v->numbits ; depth ; depth --) {
      x = maxx;
      while (x >= 16) { /* assume at least 512 pixel wide screen. */
        CL; CL; CL; CL;
        CL; CL; CL; CL;
        CL; CL; CL; CL;
        CL; CL; CL; CL;
        x -= 16;
      }
      if (x)
      switch (x) {
	case 15: CL; case 14: CL; case 13: CL;
        case 12: CL; case 11: CL; case 10: CL;
        case 9: CL; case 8: CL; case 7: CL;
        case 6: CL; case 5: CL; case 4: CL;
	case 3: CL; case 2: CL; case 1: CL;
	default:;
      }
    }
    to += j;
    from += j;
  }
  to = (long *)&v->screen[(v->bottrow-1)*v->font->height * v->linelen];
  for (i = v->font->height * v->linelen/4; i; i--)
    *to++ = 0;
}

static clearline(int vtnum, int which)
{
  /* which=0=to end of line, 1=to beginning of line, 2=all line */

  int start , end, i, j, b;
  struct vt *v = &vt[vtnum];

  switch (which)
  {
    case 0: start=v->x; end=v->numtcols-1; break;
    case 1: start=0; end=v->x; break;
    case 2: start=0; end=v->numtcols-1; break;
  }
  for (j = start; j <= end; j++)
  {
    v->scr[v->y][j] = 32;
    v->att[v->y][j] = T_NORMAL;
  }

  if (!v->visible )
    return;

  if (v->font->width%8 == 0 || v->numbits == 8) {
    start = v->y*v->font->height*v->linelen + start*v->font->width*v->numbits/8;
    end   = v->y*v->font->height*v->linelen + end*v->font->width*v->numbits/8;
    for (i = start; i <= end; i++)
      for (j = 0; j < v->font->height; j++)
        v->screen[i + j * v->linelen] = 0;
  } else {
    if (b = (start*v->font->width*v->numbits % 8)) {
      start = i = v->y*v->font->height*v->linelen + start*v->font->width*v->numbits/8;
      for (j = 0; j < v->font->height; j++)
	v->screen[i + j * v->linelen] &= ~((1<<(8-b))-1);
      start++;
    } else {
    	start = v->y*v->font->height*v->linelen + start*v->font->width*v->numbits/8;
    }
    end   = v->y*v->font->height*v->linelen + end*v->font->width*v->numbits/8;
    for (i = start; i <= end; i++)
      for (j = 0; j < v->font->height; j++)
        v->screen[i + j * v->linelen] = 0;
  }
}

static movecursordown(int vtnum)
{
  struct vt *v = &vt[vtnum];

  v->y++;
  if (v->y >= v->numtrows)
  {
    v->y--;
    scrollup(vtnum);
  }
}

static movecursorforward(int vtnum)
{
  struct vt *v = &vt[vtnum];

  /* If x == numtcols - 1, then we were just on the last column.
  We should stay there until the next character.  If the
  next character is a C/R, then don't do the movecursordown().
  Otherwise, wrap around. */

  if (v->hanging_cursor)
  {
    v->x = 0;
    movecursordown(vtnum);
    v->hanging_cursor = 0;
  }
  if (v->x == v->numtcols - 1)
    v->hanging_cursor = 1;
  else
    v->x++;
}

static scrolldown(int vtnum)
{
  /* scrolls the screen down by one text line */

  register int i, j, x, maxx;
  register long *from, *to;
  struct vt *v = &vt[vtnum];

  for (i = v->bottrow - 1; i >= v->toptrow; i--)
    for (j = 0; j < v->numtcols; j++)
    {
      v->scr[i][j] = v->scr[i-1][j];
      v->att[i][j] = v->att[i-1][j];
    }
  for (j = 0; j < v->numtcols; j++)
  {
    v->scr[v->toptrow-1][j] = 32;
    v->att[v->toptrow-1][j] = T_NORMAL;
  }

  if (!v->visible )
    return;

  from = (long *)&v->screen[(v->bottrow-1)*v->font->height*v->linelen-v->linelen];
  to = (long *)&v->screen[v->bottrow*v->font->height*v->linelen-v->linelen];
  maxx = v->numbits*(v->numgcols+31)/32;
  j = v->linelen / 4 + maxx;
  for (i = (v->bottrow-v->toptrow)*v->font->height; i; i--)
  { 
    int depth;
    for (depth = v->numbits ; depth ; depth --) {
      x = maxx;
      while (x >= 16) { /* assume at least 512 pixel wide screen. */
        CL; CL; CL; CL;
        CL; CL; CL; CL;
        CL; CL; CL; CL;
        CL; CL; CL; CL;
        x -= 16;
      }
      if (x)
      switch (x) {
	case 15: CL; case 14: CL; case 13: CL;
        case 12: CL; case 11: CL; case 10: CL;
        case 9: CL; case 8: CL; case 7: CL;
        case 6: CL; case 5: CL; case 4: CL;
	case 3: CL; case 2: CL; case 1: CL;
	default:;
      }
    }
    to -= j;
    from -= j;
  }
  to = (long *)&v->screen[(v->toptrow-1)*v->font->height*v->linelen];
  for (i = v->font->height*v->linelen/4; i; i--)
    *to++ = 0;
}

static deletechar(int vtnum, int n)
{
  int i, j, p, endp;
  struct vt *v = &vt[vtnum];

  if (n == 0) return;

  for (j = v->x; j < v->numtcols-1; j++)
  {
    v->scr[v->y][j] = v->scr[v->y][j+1];
    v->att[v->y][j] = v->att[v->y][j+1];
  }
  v->scr[v->y][v->numtcols-1] = 32;
  v->att[v->y][v->numtcols-1] = T_NORMAL;

  if (!v->visible )
    return;

  p = v->y*v->font->height*v->linelen + v->x*v->font->width*v->numbits/8;
  endp = v->y*v->font->height*v->linelen + (v->numtcols-1)*v->font->width*v->numbits/8;

  for (i = p; i < endp; i++)
  {
    for (j = 0; j < v->font->height; j++)
      v->screen[i+j*v->linelen] = v->screen[i+j*v->linelen+1];
  }
  for (j = 0; j < v->font->height; j++)
    v->screen[endp+j*v->linelen] = 0;
}

static savecursorpos(int vtnum, int save)
{
  /* Should we save more here, e.g., text attribute, etc. ? */

  struct vt *v = &vt[vtnum];

  if (save)
  {
    v->oldx = v->x;
    v->oldy = v->y;
  }
  else
  {
    v->x = v->oldx;
    v->y = v->oldy;
  }
}

static setcolor(int vtnum, int col)
{
  struct vt *v = &vt[vtnum];

  switch (col)
  {
    case 0: v->attr = T_NORMAL;
            v->fgcolor = 37;
            v->bgcolor = 40;
            break;
    case 1: v->attr |= T_BOLD; break; /* This isn't documented anywhere. */
    case 2: break;   /* Half bright */ /* We could do this */
    case 4: v->attr |= T_UNDERLINE; break;
    case 5: break;   /* Blink */ /* Should we do this?  :-) */
    case 7: v->attr |= T_REVERSE; break;
    default:
            /* 0 = Black
             * 1 = Red
             * 2 = Green
             * 3 = Yellow           + 30 = Foreground
             * 4 = Blue             + 40 = Background
             * 5 = Magenta
             * 6 = Cyan
             * 7 = White
             */
            if (col >= 30 && col <= 37)
            {
              v->fgcolor = col;
              /* Clip to white for B/W screen: */
              if (v->fgcolor > 30) v->fgcolor = 37;
            }
            else if (col >= 40 && col <= 47)
            {
              v->bgcolor = col;
              /* Clip to white for B/W screen: */
              if (v->bgcolor > 40) v->bgcolor = 47;
            }
            if (v->fgcolor == 30 && v->bgcolor == 47) /* effective reverse */
            {
              /* Elvis uses 30,43 for reverse: (Black on Yellow) */
              v->fgcolor = 37;
              v->bgcolor = 40;
              v->attr |= T_REVERSE;
            }
            break;
  }
}

static int isalpha(char c)
{
   return(c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z');
}

static int isdigit(char c)
{
   return(c >= '0' && c <= '9');
}

static void puts(int vtnum, char *s)
{
  /* This function is for functions called by printf().  It seems
     that printf() in the kernel is non-reentrant, so I wrote this
     so that parseVT100 could print stuff.  Same as puts() except
     that it doesn't print a \n at the end. */

  while (*s)
    macconputchar(vtnum, (unsigned char)(*s++));
}

static parseVT100(int vtnum, char *s)
{
  int n[100], num, i, ques;
  char *old=s;
  struct vt *v = &vt[vtnum];

  ques = (*s == '?');
  if (ques) s++;
  for (num = 0; num < 100; num++)
    n[num] = 0;
  num = 0;
  /* Parse all of the numbers at the start of the sequence: */
  if (isdigit(*s) || *s == ';')
  {
    do
    {
      if (*s == ';')
      {
        s++;
        num++;
      }
      while (isdigit(*s))
      {
        n[num] = n[num]*10 + (*s-'0');
        s++;
      }
    }
    while (*s == ';');
    num++;
  }
  switch (*s)
  {
    case 'H':  /* Move Cursor */
    case 'f':  /* Move cursor */
      v->x=n[1]-1;
      v->y=n[0]-1 + v->toptrow-1;
      if (v->x < 0) v->x=0;
      if (v->y < 0) v->y=0;
      if (v->x >= v->numtcols) v->x = v->numtcols-1;
      if (v->y >= v->numtrows) v->y = v->numtrows-1;
      break;
    case 's': savecursorpos(vtnum, 1); break; /* Save cursor pos */
    case 'u': savecursorpos(vtnum, 0); break; /* Restore cursor pos */
    case 'J':  /* Clear part of screen */
      clearscreen(vtnum, n[0]);
      break;
    case 'K':  /* Clear part of line */
      clearline(vtnum, n[0]);
      break;
    case 'P':  /* Delete character */
      deletechar(vtnum, n[0]);
      break;
    case 'A':  /* Cursor up */
      if (num == 0) n[0] = 1;
      v->y -= n[0];
      if (v->y < 0) v->y = 0;
      break;
    case 'B':  /* Cursor down */
      if (num == 0) n[0] = 1;
      v->y += n[0];
      if (v->y >= v->numtrows) v->y = v->numtrows;
      break;
    case 'C':  /* Cursor right */
      if (num == 0) n[0] = 1;
      v->x -= n[0];
      if (v->x < 0) v->x = 0;
      break;
    case 'c':  /* What are you? */
      /* Say something like ^[[?5c (I am GIGI) */
      break;
    case 'D':  /* Cursor left */
      if (num == 0) n[0]=1;
      v->x += n[0];
      if (v->x >= v->numtcols) v->y =v-> numtcols;
      break;
    case 'h': break;				/* Keyboard enable?	*/
    case 'l': break;				/* Keyboard disable?	*/
    case 'm':					/* Set color		*/
      if (num == 0)
        setcolor(vtnum, 0); /* Should this always be done? */
      else for (i = 0; i < num; i++)
        setcolor(vtnum, n[i]);
      break;
    case 'n':					/* Reports		*/
      if (n[0] == 5)				/* Status report	*/
        /* Returns ^[0n */;
      if (n[0] == 6)			/* Cursor position report	*/
        /* Returns ^[[Pl;PcR */;
      break;
    case 'r':
      if (num == 0) {
	/* I think this should return something like ncols;nrows */
      } else {
      	if (num == 1)
          n[1] = v->bottrow;
	if (n[0] < 1) n[0] = 1;
	if (n[1] < 1) n[1] = 1;
	if (n[0] > v->bottrow) n[0] = v->bottrow;
	if (n[1] > v->bottrow) n[1] = v->bottrow;
	if (n[0] < n[1]) {
	  v->toptrow = n[0];
	  v->bottrow = n[1];
	} else {
	  /* This isn't right.		      */
	  /* But it's better than nothing :-) */
	  /* And I don't know what "right" is */
	  v->toptrow = n[1];
	  v->bottrow = n[0];
	}
      }
      break;
    default:
      puts (vtnum, "Unknown VT100 code: \"");
      puts (vtnum, old);
      puts (vtnum, "\", \"");
      puts (vtnum, s);
      puts (vtnum, "\".\n");
  }
}

static int writechar(int vtnum, unsigned char c)
{
  char s[20];
  struct vt *v;

  if (c == 0) return;  /* Ignore nulls */

  v = &vt[vtnum];
  if (v->vt100 >= 2)  /* Got ESC and [ */
  {
    if (c < 32) /* Invalid character in VT100 sequence -- Abort */
      v->vt100 = 0;
    else
    {
      v->vtstr[v->vt100++ - 2] = c;
      if (isalpha(c))
      {
        v->vtstr[v->vt100 - 2] = 0;
        v->vt100 = 0;
        parseVT100(vtnum, v->vtstr);
      }
      return;
    }
  }
  if (v->vt100 == 1) /* Only got ESC */
  {
    v->vt100 = 0; /* This should finish sequence except if [ */
    switch (c)
    {
      case '=': break;  /* Turn keypad on */
      case '>': break;  /* Turn keypad off */
      case 'D': movecursordown(vtnum); break;  /* Line feed */
      case 'E': break;  /* New line */
      case 'M': scrolldown(vtnum); break;      /* Scroll down */
      case '7': savecursorpos(vtnum,1); break;  /* Save cursor pos */
      case '8': savecursorpos(vtnum,0); break;  /* Restore cursor pos */
      case '[':  /* Misc stuff */
        v->vt100 = 2;
        v->vtstr[0] = 0;
        break;
    }
    return;
  }
  switch (c)
  {
    case 7:  beep(vtnum);			/* Bell			*/
             break;
    case 8:  if (v->x > 0) v->x--;		/* Backspace (^H)	*/
             v->hanging_cursor = 0;
             break;
    case 9:  v->hanging_cursor = 0;		/* Tab			*/
             do
               writechar(vtnum, ' ');
             while (v->x%8);
             break;
    case 10: movecursordown(vtnum);			/* Line feed	*/
             /* v->x=0; <-- Should not always do this, but when?	*/
             v->hanging_cursor = 0;	/* Should I do this here?	*/
             break;
    case 13: v->x=0;				/* Carriage return	*/
             v->hanging_cursor = 0;
             break;
    case 27: v->vt100 = 1;			/* Start of VT-100	*/
             v->hanging_cursor = 0;
             break;
    case 127: break;				/* Rubout (Delete)	*/
    default: if (c < 32)
             {
               /*c=1;*/ /* smiley face */
               sprintf (s, "<%d>", (int)c);
               puts(vtnum, s);
             }
             else
             {
               if (v->hanging_cursor)
               {
                 v->x = 0;
                 movecursordown(vtnum);
                 v->hanging_cursor = 0;
               }
               putcharacter(vtnum, c);
               movecursorforward(vtnum);
             }
             break;
  }
}

static void make_visible(unsigned char *screen)
{
/* try to find a vt that belongs on this screen, and make it visible    */
/* in order to redraw we call vtselect, but that changes the curvt,     */
/* so save that and restore it, what the heck i'm already going to hell */
/* make sure that is the screen is not changing, that we do nothing     */
	int i=0;

	if (screen==vt[curvt].screen)
		return;
	for(i=0;i< NCON;i++)
	{
		if ((vt[i].screen==screen) )
		{
			int realcurvt=curvt;
			if (i == curvt)
				return;
			vt[i].visible=1;
 			vtselect(i);
			erasecursor(i);
			curvt=realcurvt;
			return;
		}
	}

}

static void fix_visible(struct vt *v)
{
/* make sure there is only one visible vt per screen */
	int i=0;
	unsigned char *screen=v->screen;
	

	for(i=0;i< NCON;i++)
	{
		if ((vt[i].screen==screen) && vt[i].visible)
			vt[i].visible=0;
	}

	v->visible=1;
}

vtselect(int newvt)
{
/* ADB calls this when you select a virtual terminal */
  int x, y;
  struct vt *v;

  if (newvt == curvt) restoresb();

  if (newvt < 0 || newvt >= NCON || newvt == curvt)
    return;

  numsb = 0;
  erasecursor(curvt);
  curvt = newvt;
  v = &vt[curvt];
 
  fix_visible(v);

#if 1
  cls(v);
  for (y = 0; y < v->numtrows; y++)
    for (x = 0; x < v->numtcols; x++)
      if (v->scr[y][x] != ' ' || v->att[y][x] != T_NORMAL)
        drawcharacter(v,x, y, v->att[y][x], v->scr[y][x]);
#else
  for (y = 0; y < numtrows; y++)
    for (x = 0; x < numtcols; x++)
      drawcharacter(x, y, v->att[y][x], v->scr[y][x]);
#endif
  drawcursor(curvt);
}

static struct tty *maccon_tty[NCON];
static int maccon_flags = 0;

/* These routines are the actual device driver: */

static char maccon_id_string[128] ;

coninit(register struct macdriver *md)
{
   md->hwfound = 1;
/*
   md->name = maccon_id_string;
   sprintf(md->name,"Video Device  -- Width: %d Height: %d Depth: %d Bit%c", 
	vt[0].numgcols,
	vt[0].numgrows,
	videobitdepth,
	videobitdepth==1?' ':'s');
*/
   grfconfig();
}

constart(register struct tty *tp)
{
   void		ttrstrt(struct tty *);
   int		s, unit, c, buflen, i;
   u_char	buf[CONBUFLEN];
 
   unit = minor(tp->t_dev);
   if (unit >= NCON)
	return ENODEV;
   if (unit == curvt) restoresb();

   s = spltty();
   if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
      goto out;

   tp->t_state |= TS_BUSY;
   splx(s);

   buflen = q_to_b(&tp->t_outq, buf, sizeof(buf)/sizeof(buf[0]));
   for (i=0 ; i<buflen ; i++)
	if (buf[i]) macputchar(tp->t_dev, buf[i]&0xff);

   s = spltty();
   tp->t_state &= ~TS_BUSY;
   if (tp->t_outq.c_cc) {
	tp->t_state |= TS_TIMEOUT;
	timeout((timeout_t) ttrstrt, (caddr_t) tp, 1);
   }
   if (tp->t_outq.c_cc <= tp->t_lowat) {
      if (tp->t_state&TS_ASLEEP) {
         tp->t_state &= ~TS_ASLEEP;
         wakeup((caddr_t)&tp->t_outq);
      }
      selwakeup(&(tp->t_wsel));
   }
out:
   splx(s);
}

conparam(
	register struct tty *tp,
	register struct termios *t)
{
	register int cfcr, cflag;
	int unit;
	int ospeed;

	cflag = t->c_cflag;
	unit = minor(tp->t_dev);
   	if (unit >= NCON)
		return ENODEV;
 
	/* check requested parameters */
        if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
                return (EINVAL);
        /* and copy to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	return (0);
}

int adb_poll_setup = 0;

static void flashcursor(caddr_t unused)
{
  if (cursortype & C_SOLID)
  {
    /* Turn it back on for solid */
    if (cursoron && !cursorlit)
      reversecursor(curvt);
  }
  else
  {
    if (cursoron)
      reversecursor(curvt);
    timeout(flashcursor, 0, (cursortype & C_SLOW) ? 30 : 15);
  }
}

conopen(dev_t dev, int mode, int flag, struct proc *p)
{
   register struct tty *tp;
   register int unit = minor(dev);
   int error = 0;
   char s[100];
 
   if (unit >= NCON)
	return (ENODEV);

   if (!maccon_tty[unit]) {
   	tp = maccon_tty[unit] = ttymalloc();
   } else {
   	tp = maccon_tty[unit];
   }
   tp->t_oproc = constart;
   tp->t_param = conparam;
   tp->t_dev = dev;
   
/* MF BARF This is gonna be zeroed later, where does this go? */
   /* tp->t_winsize.ws_xpixel=numgcols; */
   /* tp->t_winsize.ws_ypixel=numgrows; */

   if ((tp->t_state & TS_ISOPEN) == 0) {
      tp->t_state |= TS_WOPEN | TS_CARR_ON;
      ttychars(tp);
      if (tp->t_ispeed == 0) {
         tp->t_iflag = TTYDEF_IFLAG;
         tp->t_oflag = TTYDEF_OFLAG;
         tp->t_cflag = TTYDEF_CFLAG;
         tp->t_lflag = TTYDEF_LFLAG;
         tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
      }
      conparam(tp, &tp->t_termios);
      ttsetwater(tp);
   } else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
      return (EBUSY);
   (void) spltty();	/* BARF -- mutual exclusion. */
   while ((mode&O_NONBLOCK) == 0 && (tp->t_cflag&CLOCAL) == 0 &&
          (tp->t_state & TS_CARR_ON) == 0) {
      tp->t_state |= TS_WOPEN;
      if (error = ttysleep(tp, (caddr_t)&tp->t_rawq, TTIPRI | PCATCH,
          ttopen, 0))
         break;
   }
   (void) spl0();
   if (error == 0)
   {
      error = (*linesw[tp->t_line].l_open)(dev, tp);
      if(!adb_poll_setup){
         adb_poll_timeout();
         adb_poll_setup = 1;
      }
   }

   return (error);
}

conclose(dev_t dev, int flag, int mode, struct proc *p)
{
   register struct tty *tp;
   register struct dcadevice *dca;
   register int unit = minor(dev);
   char s[100];

   if (unit >= NCON)
	return (ENODEV);
   tp = maccon_tty[unit];
   (*linesw[tp->t_line].l_close)(tp, flag);
   ttyclose(tp);
   ttyfree(tp);
   maccon_tty[unit] = NULL;
   return (0);
}

conread(dev_t dev, struct uio *uio, int flag)
{
   u_int unit = minor(dev);

   if (unit >= NCON)
	return (ENODEV);
   return ((*linesw[maccon_tty[unit]->t_line].l_read)(maccon_tty[unit], uio, flag));
}

conwrite(dev_t dev, struct uio *uio, int flag)
{
   int unit = minor(dev);

   if (unit >= NCON)
	return (ENODEV);
   return ((*linesw[maccon_tty[unit]->t_line].l_write)(maccon_tty[unit], uio, flag));
}

conioctl(dev_t dev, int cmd, caddr_t data, int flag)
{
	register struct tty *tp;
	register int unit = minor(dev);
	register struct dcadevice *dca;
	register int error;
	int tmp, oldcursoron, oldcursortype;
 
	if (unit >= NCON)
		return (ENODEV);
	tp = maccon_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		return (error);

	switch (cmd) {
		case CON_SETCURSOR: /* Change cursor type */
			tmp = *(int *)data;
			if (tmp != cursortype)
			{
				oldcursoron = cursoron;
				oldcursortype = cursortype;
				if (oldcursoron) erasecursor(curvt);
				cursortype = tmp;
				if (oldcursoron) drawcursor(curvt);
				if ((oldcursortype & C_SOLID) &&
					(cursortype & (C_SLOW | C_FAST)))
					timeout(flashcursor, 0, 1);
			}
			break;
		case CON_GETCURSOR: /* Get cursor type */
			*(int *)data = cursortype;
			break;
		case CON_SETVT:
/* MF this is hacked, The interface should be better, fix later */
			{
			int i=*(int *)data;
			unsigned char *oldscreen;
			if ( (i <0) || (i >=gNumGrfDev) )
			{
				return EINVAL;
			}
			/* printf("set vt to %d\n",*(int *)data ); */
			/* printf("address %x\n",grf_softc[*(int *)data].g_display.gd_fbaddr); */
			/* printf("rowbytes = %d\n",grf_softc[*(int *)data].g_display.gd_fbrowbytes); */
/* set numgcols, numgrows, numtcols, numtrows, screen, linelen, numbits */
			oldscreen=vt[curvt].screen;
			vt[curvt].screen=(unsigned char *)grf_softc[i].g_display.gd_fbaddr;
			vt[curvt].linelen=grf_softc[i].g_display.gd_fbrowbytes;
			vt[curvt].numgcols=grf_softc[i].g_display.gd_fbwidth;
			vt[curvt].numgrows=grf_softc[i].g_display.gd_fbheight;
			vt[curvt].numbits=grf_softc[i].g_display.gd_planes;
			if (vt[curvt].numgcols >= 640) /* Big screen */
	                        vt[curvt].font=&font[0];
			else
	                        vt[curvt].font=&font[1];
			vt[curvt].numtrows=vt[curvt].numgrows/vt[curvt].font->height;
			vt[curvt].numtcols=vt[curvt].numgcols/vt[curvt].font->width;
			make_visible(oldscreen);

			/* redraw screen at new place */
			/* all this is to trick it into redrawing the screen */
			{
				int newvt=curvt;
				curvt++;
 				vtselect(newvt);
			}
			/* printf("screen = %x, rowbytes= %d\n", */
				/* vt[curvt].screen, */
				/* vt[curvt].linelen); */
			/* printf("gcol %d, grow %d, tcol %d, trow %d\n", */
				/* vt[curvt].numgcols, */
				/* vt[curvt].numgrows, */
				/* vt[curvt].numtcols, */
				/* vt[curvt].numtrows); */

			}
			break;
		case CON_SETBEEP:
			{
				struct beeps	beep;
				int		i;

				beep = *(struct beeps *) data;
				return (asc_setbellparams(beep.freq,
							  beep.length,
							  beep.vol));
			}
			break;
		case CON_GETBEEP:
			{
				int	freq, len, vol;

				asc_getbellparams(&freq, &len, &vol);
				((struct beeps *)data)->freq   = freq;
				((struct beeps *)data)->length = len;
				((struct beeps *)data)->vol    = vol;
			}
			break;
		default:
			printf ("ioctl(): default cmd = 0x%x\n",cmd);
			return (ENOTTY);
	}
	return (0);
}

conselect(dev_t dev, int rw, struct proc *p)
{
   long len;
   unsigned char buf[8];

   if (minor(dev) >= NCON) return ENODEV;
   return (ttselect(dev, rw, p));
}

conintr(register int unit, int data)
{
   /* Jeez. Should this be doing more?  -XXX */
   if (unit >= NCON)
	return 0;
   if ((maccon_tty[unit]->t_state & TS_ISOPEN) != 0)
      (*linesw[maccon_tty[unit]->t_line].l_rint)(data, maccon_tty[unit]);
   return 1;
}

ser_intr(struct my_frame *fp)
{
   /* This function is called by locore.s on a level 4 interrupt. */

   int reg0, ch, s;
   unsigned char c;
   char str[20];

   if(!serial_boot_echo)
	return(serintr());

   while(*sccaddr & SER_R0_RXREADY)
   {
      ch = *(sccaddr+4);
      *sccaddr = SER_W0_RSTESINTS;  /* Reset external/status interrupt */
      conintr(0, ch);
   }
   reg0 = *sccaddr;  /* Get status */
   if(reg0 & SER_R0_TXUNDERRUN)
   {
      *sccaddr = SER_W0_RSTTXUNDERRUN;
      *sccaddr = SER_W0_RSTESINTS;
      /* macconputchar(0, (int)'!'); */
   }
   if(reg0 & SER_R0_TXREADY)
   {
/*      sprintf(str,"<%d>",outlen);
      puts(vtnum, str); */
      write_ack = 0;
      *sccaddr = SER_W0_RSTTXPND;
      *sccaddr = SER_W0_RSTIUS; /* Reset highest interrupt pending */
      if (outlen > 0)
      {
        c = outbuf[outtail];
        outtail = (outtail + 1) % OUTBUFLEN;
        s = splhigh();
        *(sccaddr+4) = c;
        write_ack = 1;
        outlen--;
        splx(s);
      }
   }
   return(1);
}

/* Routines to link with cons.c of standalone stuff */

static unsigned char ser_init_str[]={
	9,0x64,
	10,0,
	11,0x50,
	4,0x44,
	3,0xc0,
	5,0xea,
	14,0x83,
	15,0,
	12,0x04,
	13,0,
	1,0xa,
	0,0x10,
	0,0x20,
};

macprobe(struct consdev *cp)
{
   int maj, unit;
  /*
   * macprobe():  This function returns a priority to tell the console
   * init function cninit() whether it is functional or not.  In the
   * standalone this is all we must do, but I think that in the kernel
   * we must also fill in the rest of the *cp structure, especially
   * the tty pointer.  It should also set the cn_dev field.  I don't
   * exactly know how this function should pick a number, but 0 seems
   * just fine to me.
   */

   /* locate the major number */
   for (maj = 0; maj < nchrdev; maj++)
      if (cdevsw[maj].d_open == conopen)
         break;

   /* BARF -- what happens if maj == nchrdev? */
   if (maj == nchrdev)
	return;

   cdevsw[maj].d_ttys = maccon_tty;

   /* double BARF panic -- I guess we can assume 0 */
   unit = 0;

   /* initialize required fields */
   cp->cn_dev = makedev(maj, unit);
   cp->cn_tp = NULL; /* Was maccon_tty[unit] but this isn't allocated yet. */
		     /* Hopefully we don't need the tty structure until the */
		     /* open routine is called after VM is setup. */
   cp->cn_pri = CN_INTERNAL;
}

macinit(struct consdev *cntab)
{
  unsigned char *chr;
  static int alreadyinit = 0;  /* is this necessary? */

  if (alreadyinit) return;
  alreadyinit = 1;

  conattach(NCON);

  if (serial_boot_echo) {
    chr = ser_init_str;

    while (chr < ser_init_str + sizeof(ser_init_str))
    {
      *sccaddr = *chr++;
      *sccaddr = *chr++;
    }
  }
}

#if 0
extern unsigned char keyboard[128][3];

int mactestkey(unsigned char key)
{
  /* Naturally we assume that this array is initialized to zero: */
  /* Which is fine since UNIX stupidly zeros the bss.  Thanks, UCB..  -XXX */
  static unsigned char keydown[128];

  if (key != 255)
  {
    if (key & 0x80)
      keydown[key & 0x7F] = 0;
    else
    {
      keydown[key] = 1;
      if (keyboard[key][0] != 0)
      {
        if (keydown[0x36]) /* CTRL */
          return(keyboard[key][2]);
        else if (keydown[0x38]) /* SHIFT */
          return(keyboard[key][1]);
        else
          return(keyboard[key][0]);
      }
    }
  }
  return(-1);
}
#endif

macgetchar() /* THIS WILL BLOCK!!!! */
{
   char c;
   c = adb_poll_for_char();
   return c;
}

enable_interrupt_console()
{
  intr_enabled = 1;
}

void
con_intr(caddr_t unused)  /* One arg here we could use */
{
  int s;
  char str[20], *ss;

  if (conlen > 0)
  {
    s = splconsole();
    erasecursor(0);
    while (conlen > 0)
    {
      writechar(0, (char)conbuf[contail]);
      contail = (contail + 1) % CONBUFLEN;
      conlen--;
    }
    drawcursor(0); /* <-- Fix vtnum */
    splx(s);
  }
  do_conintr = 0;
}

void
macconputchar(int vtnum, u_char c)
{
  int s;

  if (0 && intr_enabled)  /* Disable for now because of vt */
  {
    /* Put character in queue: */
    while (conlen == CONBUFLEN)  /* Wait if buffer full */
      /* DO NOTHING */;
    conbuf[conhead] = c;
    conhead = (conhead + 1) % CONBUFLEN;
    s = splconsole();
    conlen++; /* Must be exclusive access to "conlen" */
    splx(s);

    /* Schedule the interrupt: */
    if (!do_conintr)
    {
      do_conintr = 1;
      timeout(con_intr, 0, 1);
    }
  }
  else
  {
    /* This should be atomic because interrupt could want to
    display something: */

    s = splconsole();
    erasecursor(vtnum);
    writechar(vtnum, (char)c);
    drawcursor(vtnum);
    splx(s);
  }

}

macserputchar(unsigned char c)
{
  int delay, s;

#if 0
  delay = 100000;
  while (outlen == OUTBUFLEN && delay--)  /* Wait if buffer full */
    /* DO NOTHING */;
  if (outlen < OUTBUFLEN)
  {
    outbuf[outhead] = c;
    outhead = (outhead + 1) % OUTBUFLEN;
    s = splhigh();
    outlen++; /* Must be exclusive access to "outlen" */
    splx(s);
  }

  /* If delay == 0, then we timed out and output next character: */
  if (write_ack == 0 || delay == 0) /* If nothing pending */
  {
    c = outbuf[outtail];
    outtail = (outtail + 1) % OUTBUFLEN;
    s = splhigh(); /* Don't want interrupt after write: */
    *(sccaddr+4) = c;
    write_ack = 1;
    outlen--;
    splx(s);
  }
#endif

/* What was there and working before: */
  for(delay = 1; delay < 3000 && write_ack == 0; delay++);
  write_ack = 0;
  *(sccaddr+4) = c;
}

void
macputchar(dev_t dev, u_char c)
{
  if (serial_boot_echo && minor(dev) == 0)
	macserputchar((unsigned char)c); 
  restoresb();  /* Try to take this line out */
  macconputchar(minor(dev), c);
}

macconputstr(char *str)
{
  char *s=str;

  vtselect(0);
  while (*s) macconputchar(0, *s++);
}
