/*
 *	$Id: grf_tg.c,v 1.5 1994/02/13 21:10:29 chopps Exp $
 */

#include <sys/errno.h>

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
