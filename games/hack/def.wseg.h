/*	$NetBSD: def.wseg.h,v 1.4 1997/10/19 16:57:25 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _DEF_WSEG_H_
#define _DEF_WSEG_H_
#ifndef NOWORM
/* worm structure */
struct wseg {
	struct wseg *nseg;
	xchar wx,wy;
	unsigned wdispl:1;
};

#define newseg()	(struct wseg *) alloc(sizeof(struct wseg))
#endif /* NOWORM */
#endif /* _DEF_WSEG_H_ */
