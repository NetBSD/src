/*	$NetBSD: def.eshk.h,v 1.4 1997/10/19 16:56:53 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _DEF_ESHK_H_
#define _DEF_ESHK_H_

#define	BILLSZ	200
struct bill_x {
	unsigned bo_id;
	unsigned useup:1;
	unsigned bquan:7;
	unsigned price;		/* price per unit */
};

struct eshk {
	long int robbed;	/* amount stolen by most recent customer */
	boolean following;	/* following customer since he owes us sth */
	schar shoproom;		/* index in rooms; set by inshop() */
	coord shk;		/* usual position shopkeeper */
	coord shd;		/* position shop door */
	int shoplevel;		/* level of his shop */
	int billct;
	struct bill_x bill[BILLSZ];
	int visitct;		/* nr of visits by most recent customer */
	char customer[PL_NSIZ];	/* most recent customer */
	char shknam[PL_NSIZ];
};
#endif /* _DEF_ESHK_H_ */
