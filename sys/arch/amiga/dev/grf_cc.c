#include "grf.h"
#if NGRF > 0

/* Graphics routines for the AMIGA native custom chip set. */

#include "sys/param.h"
#include "sys/errno.h"
#include "grfioctl.h"
#include "grfvar.h"
#include "grf_ccreg.h"
#include "../include/cpu.h"
#include "../amiga/custom.h"
#include "../amiga/cia.h"

extern caddr_t CHIPMEMADDR;
extern caddr_t chipmem_steal ();

struct ccfb ccfb = {
  DEF_DISP_WIDTH, 
  DEF_DISP_HEIGHT, 
  DEF_DISP_X, DEF_DISP_Y, 
  DEF_DISP_Z,
  0,
  DEF_FB_WIDTH, 
  DEF_FB_HEIGHT, 
  0,
  DEF_FB_X, DEF_FB_Y, DEF_FB_Z,
#if 0
  DEF_DIWSTRT, DEF_DIWSTOP, DEF_DDFSTRT, DEF_DDFSTOP,
#endif
  DEF_COL0, DEF_COL1, DEF_COL2, DEF_COL3, 0,0,0,0,0,0,0,0,0,0,0,0,
  DEF_COL10, DEF_COL11, DEF_COL12, DEF_COL13, 0,0,0,0,0,0,0,0,0,0,0,0,
  0,				/* chip ram for beep sample */
  DEF_PERIOD, DEF_VOLUME,	/* beep sample period and volume */
  0,DEF_ABEEP,			/* beep timer, timer init value */
  0,DEF_DBEEP,			/* beep timer, timer init value */
  0,0,				/* cop1, cop2 */
  0,				/* pointer */
  0,0,				/* mouseH, mouseV */
  0,0,				/* lastMouseH, lastMouseV */
  0,0,				/* mouseX, mouseY */
  0,0,0,			/* mouseb1, mouseb2, mouseb3 */
  0,0,				/* joy1, joy2 */
  DEF_SCREEN,DEF_MOUSE,		/* screen/mouse blank timer init */
  0,0,				/* screenblank, mouseblank */
  0,0,				/* enableFlag, pad */
};

/*
 * custom copper list structure.  It replaces the macro method of
 * building copper lists for a good reason.  You want to change
 * diwstrt in an ioctl() handler?  well, with this struct, it is
 * trivial :-)
 *
 * YOU DON'T WANT! ioctl's to the console should NOT use any
 * implementation dependant data format to set values, they
 * should pass hi-level information that is processed by
 * the different console drivers. This driver would recalculate
 * diwstrt (for example) from given disp_* values.
 */
typedef struct {
  u_short planes[6][4];		/* move + hi word, move + lo word */
  u_short bplcon0[2];		/* move + viewmode */
  u_short bplcon1[2];		/* move + BPLCON1 */
  u_short bpl1mod[2];		/* move + BPL1MOD */
  u_short bpl2mod[2];		/* move + BPL2MOD */
  u_short diwstrt[2];		/* move + DIWSTRT */
  u_short diwstop[2];		/* move + DIWSTOP */
  u_short ddfstrt[2];		/* move + DDFSTRT */
  u_short ddfstop[2];		/* move + DDFSTOP */
  u_short sprites[4*8];		/* 8  sprites (0 = mouseptr, 7 unused) */
  u_short colors[32*2];		/* move + color, 32 color regs */
  u_short copother[4];		/* move + COP1LC (to point to other copper list) */
  u_short finish[6];		/* COPEND instruction, -or-
				   move + (COP2LC, COP2LC + 2, COPJMP2) */
} COPPERLIST;

/*
 * custom struct to describe the mousepointer sprite in chipram.
 * the header is tweaked by the vbl handler to move the mouse sprite
 * around.  the image[] array can be modified by the ioctl() handler
 * to change the image for the sprite!
 *
 * Again, we should probably have a much higher resolution, generic
 * sprite, and scale that down if necessary in the invidial drivers.
 */
typedef struct {
  u_char header[4];
  u_short image[16*2];
  u_short footer[2];
} SPRITEPTR;

/*
 * initializer values for the pointer struct in chip ram.  It is a stupid
 * crosshair sprite, in just one color.  Do NOT change the first 4 bytes!
 */
static SPRITEPTR pointerInit = {
  0x50,0x50,0x60,0x00,		/* header */
  0x0000,0x0000,		/* image */
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x7f7f,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0080,0x0000,
  0x0000,0x0000,		/* footer */
};

/* 
 * void initbeep(struct ccfb *fb);
 *
 * synopsis:
 *  allocates 20 bytes for a sine wave sample (in chip ram) and
 *  initializes it.  The audio hardware is turned on to play
 *  the sine wave sample in an infinite loop!  The volume is just
 *  set to zero so you don't hear it...  The sample is played in
 *  channels 0 and 1 so it goes out left+right audio jacks in the
 *  back of the machine.  The DMA is not enabled here... it is
 *  enabled in cc_init() below...  To make an audible beep, all
 *  that is needed is to turn on the volume, and then have the
 *  vbl handler turn off the volume after the desired beep duration
 *  has elapsed.
 *
 *  The custom chip console should really be broken down into a 
 *  physical and logical layer.  The physical layer should have things
 *  like the bitplanes, copper list, mousepointer chipram, and the
 *  audible beep.  The logical layers should have their own private
 *  mousepointer image, color palette, and beep parameters.  The logical
 *  layer can keep an image of chipram for its own context - layers of
 *  sorts, in amigaos parlance.
 */
static inline void
initbeep (fb) 
    struct ccfb *fb;
{
  static char sample[20] = {
    0,39,75,103,121,127,121,103,75,39,0,
    -39,-75,-103,-121,-127,-121,-103,-75,-39
  };
  short i;
  char *ptr = chipmem_steal(20);

  if (!ptr) panic("Can't chipmem_steal 20 bytes!\n");
  fb->beepSample = ptr;
  for (i=0; i<20; i++) *ptr++ = sample[i];
  fb->beepTimer = fb->beepTime;
  custom.aud[0].lc = custom.aud[1].lc = 
    (void *)((caddr_t)fb->beepSample - CHIPMEMADDR);
  custom.aud[0].len = custom.aud[1].len = 10;
  custom.aud[0].per = custom.aud[1].per = fb->beepPeriod;
  custom.aud[0].vol = custom.aud[1].vol = 0;
  fb->beepTimer = fb->dbeepTimer = 0;
  /* make SURE to disallow any audio interrupts - we don't need them */
  custom.intena = INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3;
}

/*
 * void initpointer (struct ccfb *fb);
 *
 * synopsis:
 *  this routine initializes the mouse pointer part of the ccfb.
 *  currently, it only needs to copy the initializer data to the
 *  allocated chip ram.
 */
static inline void
initpointer (fb)
    struct ccfb *fb;
{
  SPRITEPTR *pointer = (SPRITEPTR *)fb->pointer;

  /* initialize pointer structure */
  *pointer = pointerInit;
}

/*
 * void initcop (COPPERLIST *cop, COPPERLIST *othercop, int shf, 
 *               struct ccfb *fb);
 *
 * synopsis:
 *  this function initializes one copperlist, treated as short-
 *  frame list if SHF is TRUE.
 *  it is assumed that initpointer has been called by the time
 *  initcop() is called.
 *
 *  This is REALLY basic stuff... even teenaged eurodemo coders
 *  understand it :-)  Normally, I'd have done this in assembly
 *  as a bunch of dc.w statements...  it is just translated into
 *  struct form here...
 *
 *  (yep, since this *is no* eurodemo here :-)) Hey, and we
 *  even have symbolic names for registers too :-))
 */
static void inline
initcop (cop, othercop, shf, fb) 
    COPPERLIST *cop, *othercop;
    int shf;
    struct ccfb *fb;
{
  SPRITEPTR *pointer = (SPRITEPTR *)fb->pointer;
  unsigned long screen;
  unsigned long rowbytes = fb->fb_width >> 3; /* width of display, in bytes */
  u_short *plptr;
  u_short c, i, strt, stop;

  /* get PA of display area */
  screen = (unsigned long) fb->fb - (unsigned long) CHIPMEMADDR;
  fb->fb_planesize = fb->fb_height * rowbytes;

  /* account for possible interlaced half-frame */
  if (shf)
    screen += rowbytes;

  /* account for oversized framebuffers */
  screen += (fb->fb_x >> 3) + (fb->fb_y * rowbytes);  

#define MOVE COP_MOVE

  /* initialize bitplane pointers for all planes */
  for (plptr = &cop->planes[0][0], i = 0; i < fb->fb_z; i++)
    {
      MOVE (plptr, bplpth(i), HIADDR (screen));
      plptr += 2;
      MOVE (plptr, bplptl(i), LOADDR (screen));
      plptr += 2;
      screen += fb->fb_planesize;
    }
  /* set the other bitplane pointers to 0, I hate this fixed size array.. */
  while (i < 6)
    {
      MOVE (plptr, bplpth(i), 0);
      plptr += 2;
      MOVE (plptr, bplptl(i), 0);
      plptr += 2;
      i++;
    }

  c =   0x8000 				/* HIRES */
      | ((fb->fb_z & 7) << 12)		/* bitplane use */
      | 0x0200				/* composite COLOR enable (whatever this is..) */
      | 0x0004;				/* LACE */
  MOVE (cop->bplcon0, bplcon0, c);
  MOVE (cop->bplcon1, bplcon1, 0);	/* nothing */

  /* modulo is one line for interlaced displays, plus difference between
     virtual and effective framebuffer size */
  MOVE (cop->bpl1mod, bpl1mod, (fb->fb_width + (fb->fb_width - fb->disp_width)) >> 3); 
  MOVE (cop->bpl2mod, bpl2mod, (fb->fb_width + (fb->fb_width - fb->disp_width)) >> 3);

  /* these use pre-ECS register interpretation. Might want to go ECS ? */
  strt = (((fb->disp_y >> 1) & 0xff)<<8) | ((fb->disp_x >> 1) & 0xff);
  MOVE (cop->diwstrt, diwstrt, strt);
  stop = (((((fb->disp_y + fb->disp_height + 1-shf)>>1) & 0xff)<<8)
          | (((fb->disp_x + fb->disp_width)>>1) & 0xff));
  MOVE (cop->diwstop, diwstop, stop);
  /* NOTE: default values for strt: 0x2c81, stop: 0xf4c1 */

  /* these are from from HW-manual.. */
  strt = ((strt & 0xff) - 9) >> 1;
  MOVE (cop->ddfstrt, ddfstrt, strt);
  stop = strt + (((fb->disp_width >> 4) - 2) << 2);
  MOVE (cop->ddfstop, ddfstop, stop);

  /* sprites */
  {
    /* some cleverness... footer[0] is a ZERO longword in chip */
    u_short *spr = &cop->sprites[0];
    u_short addr = CUSTOM_OFS(sprpt[0]);
    u_short i;
    for (i=0; i<8; i++) {	/* for all sprites (8 of em) do */
      *spr++ = addr; *spr++ = HIADDR(&pointer->footer[0]);
      addr += 2;
      *spr++ = addr; *spr++ = LOADDR(&pointer->footer[0]);
      addr += 2;
    }
  }
  cop->sprites[0*4+1] = HIADDR((caddr_t)pointer-CHIPMEMADDR);
  cop->sprites[0*4+3] = LOADDR((caddr_t)pointer-CHIPMEMADDR);

  /* colors */
  for (i = 0; i < 32; i++)
    MOVE (cop->colors+i*2, color[i], fb->col[i]);

  /* setup interlaced display by constantly toggling between two copperlists */
  MOVE (cop->copother,   cop1lch, HIADDR ((unsigned long) othercop - (unsigned long) CHIPMEMADDR));
  MOVE (cop->copother+2, cop1lcl, LOADDR ((unsigned long) othercop - (unsigned long) CHIPMEMADDR));

  /* terminate copper list */
  COP_END (cop->finish);
}

/*
 * Install a sprite.
 * The sprites to be loaded on the alternate frames
 * can be specified separately,
 * so interlaced sprites are possible.
 */
cc_install_sprite(gp, num, spr1, spr2)
	struct grf_softc *gp;
	int num;
	u_short *spr1, *spr2;
{
  struct ccfb *fb = &ccfb;
  COPPERLIST *cop;

  cop = (COPPERLIST*)fb->cop1;
  cop->sprites[num*4+1] = HIADDR((caddr_t)spr1-CHIPMEMADDR);
  cop->sprites[num*4+3] = LOADDR((caddr_t)spr1-CHIPMEMADDR);

  cop = (COPPERLIST*)fb->cop2;
  cop->sprites[num*4+1] = HIADDR((caddr_t)spr2-CHIPMEMADDR);
  cop->sprites[num*4+3] = LOADDR((caddr_t)spr2-CHIPMEMADDR);
}

/*
 * Uninstall a sprite.
 */
cc_uninstall_sprite(gp, num)
	struct grf_softc *gp;
	int num;
{
  struct ccfb *fb = &ccfb;
  SPRITEPTR *pointer = (SPRITEPTR*)fb->pointer;
  COPPERLIST *cop;

    /* some cleverness... footer[0] is a ZERO longword in chip */
  cop = (COPPERLIST*)fb->cop1;
  cop->sprites[num*4+1] = HIADDR(&pointer->footer[0]);
  cop->sprites[num*4+3] = LOADDR(&pointer->footer[0]);

  cop = (COPPERLIST*)fb->cop2;
  cop->sprites[num*4+1] = HIADDR(&pointer->footer[0]);
  cop->sprites[num*4+3] = LOADDR(&pointer->footer[0]);
}

/*
 * Install a copper list extension.
 */
cc_install_cop_ext(gp, cl1, cl2)
	struct grf_softc *gp;
	u_short *cl1, *cl2;
{
  struct ccfb *fb = &ccfb;
  COPPERLIST *cop;

  cop = (COPPERLIST*)fb->cop1;
  COP_MOVE (cop->finish+0, cop2lch, HIADDR((caddr_t)cl1-CHIPMEMADDR));
  COP_MOVE (cop->finish+2, cop2lcl, LOADDR((caddr_t)cl1-CHIPMEMADDR));
  COP_MOVE (cop->finish+4, copjmp2, 0);

  cop = (COPPERLIST*)fb->cop2;
  COP_MOVE (cop->finish+0, cop2lch, HIADDR((caddr_t)cl2-CHIPMEMADDR));
  COP_MOVE (cop->finish+2, cop2lcl, LOADDR((caddr_t)cl2-CHIPMEMADDR));
  COP_MOVE (cop->finish+4, copjmp2, 0);
}

/*
 * Uninstall a copper list extension.
 */
cc_uninstall_cop_ext(gp, cl1, cl2)
	struct grf_softc *gp;
	u_short *cl1, *cl2;
{
  register struct ccfb *fb = &ccfb;
  COPPERLIST *cop;

  cop = (COPPERLIST*)fb->cop1;
  COP_END (cop->finish);

  cop = (COPPERLIST*)fb->cop2;
  COP_END (cop->finish);
}

/*
 * Call this function any time a key is hit to ensure screen blanker unblanks
 */
void 
cc_unblank () 
{
  if (!ccfb.screenBlank) {	/* screenblank timer 0 means blank! */
    COPPERLIST *c1 = (COPPERLIST *)ccfb.cop1, *c2 = (COPPERLIST *)ccfb.cop2;
    /* turn on sprite and raster DMA */
    custom.dmacon = DMAF_SETCLR | DMAF_RASTER | DMAF_SPRITE;
    ccfb.mouseBlank = ccfb.mouseTime; /* start mouseblank timer */
    /* screen was black, reset background color to the one in ccfb! */
    c1->colors[1] = c2->colors[1] = ccfb.col[0];
  }
  /* restart the screenblank timer */
  ccfb.screenBlank = ccfb.screenTime;
}

/*
 * void cc_bell(void);
 *
 * Synopsis:
 *  trigger audible bell
 * Description
 *  Call this function to start a beep tone.  The beep lasts for
 *  ccfb.beepTime 60ths of a second (can adjust it in the ccfb structure
 *  in an ioctl().  The sample is playing in left+right aud0+aud1 hardware
 *  channels all the time, just the volume is off when the beep isn't
 *  heard.  So here we just turn on the volume (ccfb.beepVolume, it can
 *  also be set by ioctl() call) and set the timer (ccfb.beepTime can
 *  be set by ioctl() as well).  The cc_vbl() routine counts down the
 *  timer and shuts off the volume when it reaches zero.
 */
void 
cc_bell () 
{
  custom.aud[0].vol = ccfb.beepVolume;
  custom.aud[1].vol = ccfb.beepVolume;
  ccfb.beepTimer = ccfb.beepTime;
}

/*
 * void cc_vbl(void);
 *
 * synopsis:
 *  vertical blank service routine for the console.
 *  provides the following:
 *   samples mouse counters and positions mouse sprite
 *   samples joystick inputs
 *   counts down mouseblanker timer and blanks mouse if it is time
 *   counts down screenblanker timer and blanks if it is time
 *   counts down audio beep timer and shuts of the volume if the beep is done
 *   unblanks mouse/screen if mouse is moved
 * not implemented yet:
 *  it should adjust color palette in copper list over time to effect
 *   display beep.
 *
 * There's black magic going on here with assembly-in-C.. Since this
 * is an interrupt handler, and it should be fast, ignore the obscure but
 * probably fast processing of the mouse for now...
 */
void 
cc_vbl () 
{
  u_short w0, w1;
  u_char *b0 = (u_char *)&w0, *b1 = (u_char *)&w1;
  SPRITEPTR *p = (SPRITEPTR *)ccfb.pointer;

  ccfb.lastMouseH = ccfb.mouseH;
  ccfb.lastMouseV = ccfb.mouseV;

  /* horizontal mouse counter */
  w1 = custom.joy0dat;
  b0[1] = ccfb.mouseH;		/* last counter val */
  ccfb.mouseH = b1[1];		/* current is now last */
  b1[1] -= b0[1];		/* current - last */
  b1[0] = (b1[1] & 0x80) ? 0xff : 0x00; /* ext.w */
  ccfb.mouseX += w1;
  if (ccfb.mouseX < 0) ccfb.mouseX = 0;
  if (ccfb.mouseX > ccfb.fb_width-1) ccfb.mouseX = ccfb.fb_width-1;

  /* vertical mouse counter */
  w1 = custom.joy0dat;
  b1[1] = b1[0];
  b0[1] = ccfb.mouseV;
  ccfb.mouseV = b1[1];
  b1[1] -= b0[1];
  b1[0] = (b1[1] & 0x80) ? 0xff : 0x00; /* ext.w */
  ccfb.mouseY += w1;
  if (ccfb.mouseY < 0) ccfb.mouseY = 0;
  if (ccfb.mouseY > ccfb.fb_height-1) ccfb.mouseY = ccfb.fb_height-1;

  /* mouse buttons (should renumber them, middle button should be #2!) */
  ccfb.mouseb1 = (ciaa.pra & (1<<6)) ? 0 : !0;
  ccfb.mouseb2 = (custom.pot1dat & (1<<2)) ? 0 : !0;
  ccfb.mouseb3 = (custom.pot1dat & (1<<0)) ? 0 : !0;

  /* position pointer sprite */
  w0 = ccfb.mouseY >> 1;
  b0[1] += 0x24;
  p->header[0] = b0[1];
  b0[1] += 16;
  p->header[2] = b0[1];

  w0 = ccfb.mouseX >> 1;
  w0 += 120;
  if (w0 & 1) p->header[3] |= 1; else p->header[3] &= ~1;
  w0 >>= 1;
  p->header[1] = b0[1];

  /* joystick #1 */
  ccfb.joy0 = 0;
  w0 = custom.joy1dat;
  w1 = w0 >> 1;
  w1 ^= w0;
  if (w1 & (1<<9)) ccfb.joy0 |= JOYLEFT;
  if (w1 & (1<<1)) ccfb.joy0 |= JOYRIGHT;
  if (w1 & (1<<8)) ccfb.joy0 |= JOYUP;
  if (w1 & (1<<0)) ccfb.joy0 |= JOYDOWN;
  if ( (ciaa.pra & (1<<7)) == 0 ) ccfb.joy0 |= JOYBUTTON;
	
  /* joystick #2 (normally mouse port) */
  ccfb.joy1 = 0;
  w0 = custom.joy0dat;
  w1 = w0 >> 1;
  w1 ^= w0;
  if (w1 & (1<<9)) ccfb.joy1 |= JOYLEFT;
  if (w1 & (1<<1)) ccfb.joy1 |= JOYRIGHT;
  if (w1 & (1<<8)) ccfb.joy1 |= JOYUP;
  if (w1 & (1<<0)) ccfb.joy1 |= JOYDOWN;
  if ( (ciaa.pra & (1<<6)) == 0 ) ccfb.joy1 |= JOYBUTTON;

  /* only do screenblanker/mouseblanker/display beep if screen is enabled */
  if (ccfb.enableFlag) {
    /* handle screen blanker */
    if (ccfb.screenBlank) {
      COPPERLIST *c1 = (COPPERLIST *)ccfb.cop1, *c2 = (COPPERLIST *)ccfb.cop2;
      ccfb.screenBlank--;
      if (!ccfb.screenBlank) {
	custom.dmacon = DMAF_RASTER | DMAF_SPRITE;
	c1->colors[1] = c2->colors[1] = 0; /* make screen BLACK */
      }
    }

    /* handle mouse blanker */
    if (ccfb.mouseBlank) {
      ccfb.mouseBlank--;
      if (!ccfb.mouseBlank) custom.dmacon = DMAF_SPRITE; 
    }
    else if (ccfb.lastMouseH != ccfb.mouseH || ccfb.lastMouseV != ccfb.mouseV) {
      cc_unblank();
      ccfb.mouseBlank = ccfb.mouseTime;
      custom.dmacon = DMAF_SETCLR | DMAF_SPRITE;
    }

    /* handle visual beep (not implemented yet) */
  }

  /* handle audible beep */
  if (ccfb.beepTimer) ccfb.beepTimer--;
  if (!ccfb.beepTimer) custom.aud[0].vol = custom.aud[1].vol = 0;
}

/* useful function for debugging.. */
int
amiga_mouse_button (num)
     int num;
{
  switch (num)
    {
    case 1:
      return ccfb.mouseb1;

    case 2:
      return ccfb.mouseb2;
      
    case 3:
      return ccfb.mouseb3;

    default:
      return 0;
    }
}


/* Initialize hardware.
 * Must point g_display at a grfinfo structure describing the hardware.
 * Returns 0 if hardware not present, non-zero ow.
 */
cc_init(gp, ad)
	struct grf_softc *gp;
	struct amiga_device *ad;
{
  register struct ccfb *fb = &ccfb;
  struct grfinfo *gi = &gp->g_display;
  u_char *fbp, save;
  int fboff, fbsize;
  int s;

  /* if already initialized, fail */
  if (fb->fb) return 0;

  /* disable dma */
  custom.dmacon  = DMAF_BLTDONE 
    | DMAF_BLTNZERO | DMAF_BLITHOG | DMAF_BLITTER | DMAF_DISK
      | DMAF_AUD3 | DMAF_AUD2 | DMAF_AUD1 | DMAF_AUD0;

  fb->mouseBlank = fb->mouseTime;
  fb->screenBlank = fb->screenTime;

  /* testing for the result is really redundant because chipmem_steal
     panics if it runs out of memory.. */
  fbsize = (fb->fb_width >> 3) * fb->fb_height * fb->fb_z;
  if (! (fb->fb = (u_char *) chipmem_steal (fbsize))
      || !(fb->cop1 = (u_short *) chipmem_steal (sizeof(COPPERLIST)))
      || !(fb->cop2 = (u_short *) chipmem_steal (sizeof(COPPERLIST)))
      || !(fb->pointer = (u_short *)chipmem_steal (sizeof(SPRITEPTR)))
      )
    return 0;

  /* clear the display. bzero only likes regions up to 64k, so call multiple times */
  for (fboff = 0; fboff < fbsize; fboff += 64*1024)
    bzero (fb->fb + fboff, fbsize - fboff > 64*1024 ? 64*1024 : fbsize - fboff);

  /* init the audio beep */
  initbeep(fb);
  /* initialize the sprite pointer */
  initpointer(fb);

  /* initialize the copper lists  */
  initcop (fb->cop1, fb->cop2, 0, fb);
  initcop (fb->cop2, fb->cop1, 1, fb);

  /* start the new display */

  /* ok, this is a bit rough.. */
  /* mtk: not any more! :-) */
  /* mykes: phew, thanks :-) */
  s = splhigh ();

  /* install dummy, to get display going (for vposr to count.. ) */
  custom.cop1lc  = (void *) ((unsigned long)fb->cop1 - (unsigned long) CHIPMEMADDR);
  custom.copjmp1 = 0;

  /* enable DMA (so the copperlists are executed and eventually
     cause a switch to an interlaced display on system not already booting that
     way. THANKS HAMISH for finding this bug!!) */
  custom.dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER |
		  DMAF_COPPER | DMAF_SPRITE | DMAF_AUD0 | DMAF_AUD1;

  /* this is real simple:  wait for LOF bit of vposr to go high - then start
     the copper list! :-) */
  while (custom.vposr & 0x8000);
  while (!(custom.vposr & 0x8000));

  custom.cop1lc  = (void *) ((unsigned long)fb->cop1 - (unsigned long) CHIPMEMADDR);
  custom.copjmp1 = 0;

  custom.intreq = INTF_VERTB;

  splx (s);

#if 0
  /* tame the blitter. Copying one word onto itself should put it into
     a consistent state. This is black magic... */
  custom.bltapt =
    custom.bltbpt =
      custom.bltcpt =
        custom.bltdpt = 0;
  custom.bltamod =
    custom.bltbmod =
      custom.bltcmod =
        custom.bltdmod = 0;
  custom.bltafwm =
    custom.bltalwn = 0xffff;
  custom.bltcon0 = 0x09f0;
  custom.bltcon1 = 0;
  custom.bltsize = 1;
#endif

  /* enable VBR interrupts. This is also done in the serial driver, but it
     really belongs here.. */
  custom.intena = INTF_SETCLR | INTF_VERTB; /* under amigaos, INTF_INTEN is needed */

#if 0
#ifdef DEBUG
  /* prove the display is up.. */
  for (fboff = 0; fboff < fbsize; fboff++)
    {
      fb->fb[fboff] = 0xff;
      DELAY(10);
    }
  for (fboff = 0; fboff < fbsize; fboff++)
    {
      fb->fb[fboff] = 0;
      DELAY(10);
    }
#endif
#endif

  gi->gd_regaddr = (caddr_t) fb; /* XXX */
  gi->gd_regsize = 0;

  gi->gd_fbaddr  = fb->fb - (u_char *) CHIPMEMADDR;
#if 0
  /* mykes kludges here to make gi look like 1 bitplane */
  gi->gd_fbsize  = fbsize/2;
#else
  /* don't see why we should kludge here.. we have 
     disp_z to indicate the real depth of the display */
  gi->gd_fbsize  = fbsize;
#endif
  
  gi->gd_colors = 1 << fb->disp_z;
  gi->gd_planes = fb->disp_z;
  
  gi->gd_fbwidth  = fb->fb_width;
  gi->gd_fbheight = fb->fb_height;
  gi->gd_fbx	  = fb->fb_x;
  gi->gd_fby	  = fb->fb_y;
  gi->gd_dwidth   = fb->disp_width;
  gi->gd_dheight  = fb->disp_height;
  gi->gd_dx	  = fb->disp_x;
  gi->gd_dy	  = fb->disp_y;
  
  gp->g_regkva = 0;		/* builtin */
  gp->g_fbkva  = fb->fb;
  
  fb->enableFlag = !0;
  return(1);
}

cc_config(gp, di)
	register struct grf_softc *gp;
	struct grfdyninfo *di;
{
  register struct ccfb *fb = &ccfb;
  struct grfinfo *gi = &gp->g_display;
  u_char *fbp, save;
  int fboff, fbsize;
  int s;

  /* bottom missing... */

}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
cc_mode(gp, cmd, arg)
	register struct grf_softc *gp;
	int cmd;
	void *arg;
{
  switch (cmd)
    {
    case GM_GRFON:
      ccfb.enableFlag = !0; 
      ccfb.screenBlank = ccfb.screenTime; 
      ccfb.mouseBlank = ccfb.mouseTime;
      custom.dmacon  = DMAF_SETCLR | DMAF_RASTER | DMAF_COPPER | DMAF_SPRITE;
      return 0;
      
    case GM_GRFOFF:
      ccfb.enableFlag = 0;
      custom.dmacon  = DMAF_RASTER | DMAF_COPPER | DMAF_SPRITE;
      return 0;
      
    case GM_GRFCONFIG:
      return cc_config (gp, (struct grfdyninfo *) arg);

    default:
      break;
    }
    
  return EINVAL;
}

#endif

