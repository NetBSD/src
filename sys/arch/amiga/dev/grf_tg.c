/*	$Id: grf_tg.c,v 1.2 1993/08/02 18:33:34 mycroft Exp $ */

#include "errno.h"

/* to be written.. */

tg_init (gp, ad)
	struct grf_softc *gp;
	struct amiga_device *ad;
{
  return 0;
}

tg_mode (gp, cmd)
	register struct grf_softc *gp;
	int cmd;
{
  return EINVAL;
}
