/*	$NetBSD: savelev.c,v 1.6 2008/02/03 19:29:50 dholland Exp $	*/

/* savelev.c		 Larn is copyrighted 1986 by Noah Morgan. */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: savelev.c,v 1.6 2008/02/03 19:29:50 dholland Exp $");
#endif				/* not lint */
#include "header.h"
#include "extern.h"

/*
 *	routine to save the present level into storage
 */
void
savelevel()
{
	struct cel *pcel;
	u_char  *pitem, *pknow, *pmitem;
	short *phitp, *piarg;
	struct cel *pecel;
	pcel = &cell[level * MAXX * MAXY];	/* pointer to this level's
						 * cells */
	pecel = pcel + MAXX * MAXY;	/* pointer to past end of this
					 * level's cells */
	pitem = item[0];
	piarg = iarg[0];
	pknow = know[0];
	pmitem = mitem[0];
	phitp = hitp[0];
	while (pcel < pecel) {
		pcel->mitem = *pmitem++;
		pcel->hitp = *phitp++;
		pcel->item = *pitem++;
		pcel->know = *pknow++;
		pcel->iarg = *piarg++;
		pcel++;
	}
}

/*
 *	routine to restore a level from storage
 */
void
getlevel()
{
	struct cel *pcel;
	u_char  *pitem, *pknow, *pmitem;
	short *phitp, *piarg;
	struct cel *pecel;
	pcel = &cell[level * MAXX * MAXY];	/* pointer to this level's
						 * cells */
	pecel = pcel + MAXX * MAXY;	/* pointer to past end of this
					 * level's cells */
	pitem = item[0];
	piarg = iarg[0];
	pknow = know[0];
	pmitem = mitem[0];
	phitp = hitp[0];
	while (pcel < pecel) {
		*pmitem++ = pcel->mitem;
		*phitp++ = pcel->hitp;
		*pitem++ = pcel->item;
		*pknow++ = pcel->know;
		*piarg++ = pcel->iarg;
		pcel++;
	}
}
