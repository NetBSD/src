/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$Id: def.gold.h,v 1.2 1993/08/02 17:16:45 mycroft Exp $
 */

struct gold {
	struct gold *ngold;
	xchar gx,gy;
	long amount;
};

extern struct gold *fgold;
struct gold *g_at();
#define newgold()	(struct gold *) alloc(sizeof(struct gold))
