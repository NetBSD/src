/*
 *	$Id: grf_cc.c,v 1.9 1994/02/13 21:10:26 chopps Exp $
 */

#include "grf.h"
#if NGRF > 0

/* Graphics routines for the AMIGA native custom chip set. */

/* NOTE: this is now only a more or less rough interface to Chris Hopps'
         view driver. Due to some design problems with RTG view can't
	 currently replace grf, but that's an option for the future. */

#include <sys/param.h>
#include <vm/vm_param.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <machine/cpu.h>

#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/dlists.h>

#include <amiga/dev/grfioctl.h>
#include <amiga/dev/grfvar.h>
#include <amiga/dev/grf_ccreg.h>
#include <amiga/dev/grfabs_reg.h>
#include <amiga/dev/viewioctl.h>

/* Initialize hardware.
 * Must point g_display at a grfinfo structure describing the hardware.
 * Returns 0 if hardware not present, non-zero ow.
 */
cc_init(gp, ad)
	struct grf_softc *gp;
	struct amiga_device *ad;
{
  bmap_t bm;
  struct grfinfo *gi = &gp->g_display;
  int rc;
  long start, off;
  struct view_size vs;

  /* already initialized fail */
  if (gp->g_data == (caddr_t) 12345678)
    return 0;

  rc = grfcc_probe ();
  if (! rc)
    return 0;

  viewprobe ();

  /* now it gets REALLY nasty.. view[0] stays open for all time
     now.. */
  if (viewopen (0, 0))
    return 0;

  /* XXX 0 may need to change when virtual added */
  XXX_grf_cc_on (0);

  return(1);
}

cc_config(gp, di)
	register struct grf_softc *gp;
	struct grfdyninfo *di;
{
  struct grfinfo *gi = &gp->g_display;

  /* bottom missing... */

}

extern struct grf_softc grf_softc[];
#define GPUNIT(ip) (((u_long)ip-(u_long)grf_softc)/sizeof(struct grf_softc))

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
      XXX_grf_cc_on (GPUNIT (gp));
      return 0;
      
    case GM_GRFOFF:
      XXX_grf_cc_off (GPUNIT (gp));
      return 0;
      
    case GM_GRFCONFIG:
      return cc_config (gp, (struct grfdyninfo *) arg);

    default:
      break;
    }
    
  return EINVAL;
}


/* nasty nasty hack. right now grf's unit == ite's unit == view's unit. */
/* to make X work (I guess) we need to update the grf structure's HW */
/* address to the current view (as this may change without grf knowing.) */
/* Thanks to M Hitch. for finding this one. */
XXX_grf_cc_on (unit)
    int unit;
{
  struct grf_softc *gp = &grf_softc[unit];
  struct grfinfo *gi = &gp->g_display;
  bmap_t bm;
  struct view_size vs;

  viewioctl (unit, VIEW_GETBITMAP, &bm, 0, -1);
  
  gp->g_data = (caddr_t) 12345678; /* not particularly clean.. */
  
  gi->gd_regaddr = (caddr_t) 0xdff000;		/* no need to look at regs */
  gi->gd_regsize = round_page(sizeof (custom));	/* I mean it X people, for CC */
					/* use the view device. That way */
					/* we can have both X and a console. */
					/* and flip between them.  People */
					/* will be unhappy if this feature */
					/* is so easy and yet not availble. */
					/* (if you wern't using them then */
					/* thanks.) */
					/**** I will remove these soon.****/
  gi->gd_fbaddr  = bm.hardware_address;
  gi->gd_fbsize  = bm.depth*bm.bytes_per_row*bm.rows;

  if (viewioctl (unit, VIEW_GETSIZE, &vs, 0, -1))
    {
      /* fill in some default values... XXX */
      vs.width = 640; vs.height = 400; vs.depth = 2;
    }

  gi->gd_colors = 1 << vs.depth;
  gi->gd_planes = vs.depth;
  
  gi->gd_fbwidth  = vs.width;
  gi->gd_fbheight = vs.height;
  gi->gd_fbx	  = 0;
  gi->gd_fby	  = 0;
  gi->gd_dwidth   = vs.width;
  gi->gd_dheight  = vs.height;
  gi->gd_dx	  = 0;
  gi->gd_dy	  = 0;
  
  gp->g_regkva = 0;	/* builtin */
  gp->g_fbkva  = 0;	/* not needed, view internal */

  viewioctl (unit, VIEW_DISPLAY, NULL, 0, -1);
    
}

XXX_grf_cc_off (unit)
    int unit;
{
    viewioctl (unit, VIEW_REMOVE, NULL, 0, -1);
}


#endif

