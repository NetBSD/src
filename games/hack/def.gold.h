/*	$NetBSD: def.gold.h,v 1.4 1997/10/19 16:57:03 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _DEF_GOLD_H_
#define _DEF_GOLD_H_
struct gold {
	struct gold *ngold;
	xchar gx,gy;
	long amount;
};

#define newgold()	(struct gold *) alloc(sizeof(struct gold))
extern struct gold *fgold;
#endif /* _DEF_GOLD_H_ */
