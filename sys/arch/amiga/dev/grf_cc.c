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


extern caddr_t CHIPMEMADDR;
extern caddr_t chipmem_steal ();

struct ccfb ccfb = {
	DEF_DISP_WIDTH, DEF_DISP_HEIGHT, DEF_DISP_X, DEF_DISP_Y, DEF_DISP_Z,
	0,
	DEF_FB_WIDTH, DEF_FB_HEIGHT, DEF_FB_X, DEF_FB_Y, DEF_FB_Z,
	DEF_COL0, DEF_COL1
};

/* Initialize one copper list.  We'll need two to make a nice interlaced display.  */

/* maximum size needed for a copper list (4 planes hires laced) */
#define COPENTS (4 * (2 * 4 + 2 + 1 + 2 + 2 + 2 + 16 * 1 + 1))

/* copper instructions */
#define MOVE(cl, reg, val)  \
	do { *cl++ = CUSTOM_OFS(reg); *cl++ = val; } while (0)
#define WAIT(cl, vp, hp, bfd, ve, he) \
	do { *cl++ = ((vp & 0xff)<<8)|(hp & 0xfe)|1; \
	*cl++ = (bfd<<15)|((ve & 0x7f)<<8)|(hp & 0xfe)|1; } while (0)
#define STOP(cl) \
	do { *cl++ = 0xffff; *cl++ = 0xffff; } while (0)

static void 
initcop (cop, othercop, shf, fb)
	u_short *cop;
	u_short *othercop;
	int	shf;
	struct ccfb *fb;
{
  long scrmem;
  int i;
  u_short c, strt, stop, *orig_cop = cop;

  /* get PA of display area */
  scrmem = (long) fb->fb - (long) CHIPMEMADDR;

  othercop = (u_short *) ((long)othercop - (long) CHIPMEMADDR);
  
  /* account for possible interlaced half-frame */
  if (shf)
    scrmem += fb->fb_width >> 3;
    
  /* account for oversized framebuffers */
  scrmem += (fb->fb_x >> 3) + (fb->fb_y * (fb->fb_width >> 3));  

  /* initialize bitplane pointers for all planes */
  /* remember offset in copperlist to patch later */
  if (! fb->bplstart_off)
    fb->bplstart_off = cop - orig_cop;
  for (i = 0; i < fb->disp_z; i++)
    {
      MOVE (cop, bplpth(i), scrmem >> 16);
      MOVE (cop, bplptl(i), scrmem & 0xffff);
      scrmem += (fb->fb_width >> 3) * fb->fb_height;
    }

  /* modulo is one line for interlaced displays, plus difference between
     virtual and effective framebuffer size */
  MOVE (cop, bpl1mod, (fb->fb_width + (fb->fb_width - fb->disp_width)) >> 3); 
  MOVE (cop, bpl2mod, (fb->fb_width + (fb->fb_width - fb->disp_width)) >> 3);
  
  c =   0x8000 				/* HIRES */
      | ((fb->disp_z & 7) << 12)	/* bitplane use */
      | 0x0200				/* composite COLOR enable (whatever this is..) */
      | 0x0004;				/* LACE */
        
  MOVE (cop, bplcon0, c);
  
  /* these use pre-ECS register interpretation. Might want to go ECS ? */
  strt = (((fb->disp_y >> 1) & 0xff)<<8) | ((fb->disp_x >> 1) & 0xff);
  MOVE (cop, diwstrt, strt);
  stop = (((((fb->disp_y + fb->disp_height + 1-shf)>>1) & 0xff)<<8)
          | (((fb->disp_x + fb->disp_width)>>1) & 0xff));
  MOVE (cop, diwstop, stop);
  /* NOTE: default values for strt: 0x2c81, stop: 0xf4c1 */

  /* these are from from HW-manual.. */
  strt = ((strt & 0xff) - 9) >> 1;
  MOVE (cop, ddfstrt, strt);
  stop = strt + (((fb->disp_width >> 4) - 2) << 2);
  MOVE (cop, ddfstop, stop);

  /* setup interlaced display by constantly toggling between two copperlists */
  MOVE (cop, cop1lch, (long)othercop >> 16);
  MOVE (cop, cop1lcl, (long)othercop & 0xffff);

  for (i = 0; i < (1 << fb->disp_z); i++)
    MOVE (cop, color[i], fb->col[i]);

  /* wait forever */
  STOP (cop);
}


#ifdef DEBUG
void
dump_copperlist (cl)
    u_int *cl;
{
  while (*cl != 0xffffffff)
    {
      if (!(*cl & 0x00010000))
        printf ("MOVE (%x, %x)\t", *cl & 0xffff, *cl >> 16);
      else
        printf ("WAIT (%d, %d, %d, %d, %d)\t", *cl >> 24, (*cl & 0x00fe0000)>>16,
		(*cl & 0x8000)>> 15, (*cl & 0x7f00)>>8, (*cl & 0xfe));
      cl++;
    }
  printf ("STOP ()\n");

}
#endif


/*
 * Initialize hardware.
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
  if (fb->fb)
    return 0;

  /* testing for the result is really redundant because chipmem_steal
     panics if it runs out of memory.. */
  fbsize = (fb->fb_width >> 3) * fb->fb_height * fb->fb_z;
  if (! (fb->fb = (u_char *) chipmem_steal (fbsize))
      || !(fb->cop1 = (u_short *) chipmem_steal (COPENTS))
      || !(fb->cop2 = (u_short *) chipmem_steal (COPENTS)))
    return 0;

  /* clear the display. bzero only likes regions up to 64k, so call multiple times */
  for (fboff = 0; fboff < fbsize; fboff += 64*1024)
    bzero (fb->fb + fboff, fbsize - fboff > 64*1024 ? 64*1024 : fbsize - fboff);

  initcop (fb->cop1, fb->cop2, 0, fb);
  initcop (fb->cop2, fb->cop1, 1, fb);

  /* Make sure no ex-sprites are streaking down the screen */
  {
    int i;
    for(i = 0;i < 8;i++)
      {
        custom.spr[i].data = 0;
        custom.spr[i].datb = 0;
      }
  }

  /* start the new display */
  /* disable these */
  custom.dmacon  = (DMAF_BLTDONE | DMAF_BLTNZERO | DMAF_BLITHOG | DMAF_BLITTER
		    | DMAF_SPRITE | DMAF_DISK
 		    | DMAF_AUD3 | DMAF_AUD2 | DMAF_AUD1 | DMAF_AUD0);
  /* enable these */
  custom.dmacon  = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER;

#if 0
  /* ok, this is a bit rough.. */
  s = splhigh ();
  /* load a first guess copperlist, verify later whether we got the right
     one */
  custom.cop1lc  = (void *) ((long)fb->cop1 - (long) CHIPMEMADDR);
  custom.copjmp1 = 0;
  /* reset VBL */
  custom.intreq = INTF_VERTB;
  /* wait for VBL */
  while (! (custom.intreqr & INTF_VERTB)) ;
  /* reset VBL */
  custom.intreq = INTF_VERTB;
  /* now, in a safe location, set correct copperlist based on longframe/shortframe
     bit */
  if (custom.vposr & 0x8000)
    {
      custom.cop1lc = (void *) ((long)fb->cop1 - (long) CHIPMEMADDR);
      custom.copjmp1 = 0;	/* strobe it */
    }
  else
    {
      custom.cop1lc = (void *) ((long)fb->cop2 - (long) CHIPMEMADDR);
      custom.copjmp1 = 0;
    }
  /* wait for another VBL and reset int, then go back to previous int level */
  while (! (custom.intreqr & INTF_VERTB)) ;
  custom.intreq = INTF_VERTB;
  splx (s);
#else
  s = splhigh();
  /* set up copper */
  custom.cop1lc = (void *) ((long)fb->cop1 - (long) CHIPMEMADDR);
  custom.copjmp1 = 0;

  /* reset vertical blank interrupt */
  custom.intreq = INTF_VERTB;

  /* wait for vertical blank interrupt */
  while ((custom.intreqr & INTF_VERTB) != INTF_VERTB)
	;

  /* set bitplane pointers based on LOF/SHF bit */
  if (custom.vposr & 0x8000)
    {
      custom.cop1lc = (void *) ((long)fb->cop1 - (long) CHIPMEMADDR);
      custom.copjmp1 = 0;
    }
  else
    {
      custom.cop1lc = (void *) ((long)fb->cop2 - (long) CHIPMEMADDR);
      custom.copjmp1 = 0;
    }
  splx (s);
#endif

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

  /* enable VBR interrupts. This is also done in the serial driver, but it
     really belongs here.. */
  custom.intena = INTF_SETCLR | INTF_VERTB;

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

  gi->gd_regaddr = (caddr_t) fb;	/* XXX */
  gi->gd_regsize = 0;

  gi->gd_fbaddr  = fb->fb - (u_char *) CHIPMEMADDR;
  gi->gd_fbsize  = fbsize;
  
  gi->gd_colors  = 1 << fb->fb_z;
  gi->gd_planes  = fb->fb_z;
  
  gi->gd_fbwidth  = fb->fb_width;
  gi->gd_fbheight = fb->fb_height;
  gi->gd_fbx	  = fb->fb_x;
  gi->gd_fby	  = fb->fb_y;
  gi->gd_dwidth   = fb->disp_width;
  gi->gd_dheight  = fb->disp_height;
  gi->gd_dx	  = fb->disp_x;
  gi->gd_dy	  = fb->disp_y;
  
  gp->g_regkva = 0;	/* builtin */
  gp->g_fbkva  = fb->fb;
  
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
      custom.dmacon  = DMAF_SETCLR | DMAF_RASTER | DMAF_COPPER;
      return 0;
      
    case GM_GRFOFF:
      custom.dmacon  = DMAF_RASTER | DMAF_COPPER;
      return 0;
      
    case GM_GRFCONFIG:
      return cc_config (gp, (struct grfdyninfo *) arg);

    default:
      break;
    }
    
  return EINVAL;
}

#endif
