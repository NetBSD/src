/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$Id: def.wseg.h,v 1.2 1993/08/02 17:16:55 mycroft Exp $
 */

#ifndef NOWORM
/* worm structure */
struct wseg {
	struct wseg *nseg;
	xchar wx,wy;
	unsigned wdispl:1;
};

#define newseg()	(struct wseg *) alloc(sizeof(struct wseg))
#endif NOWORM
