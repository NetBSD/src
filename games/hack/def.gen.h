/*	$NetBSD: def.gen.h,v 1.4 1997/10/19 16:57:01 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _DEF_GEN_H_
#define _DEF_GEN_H_
struct gen {
	struct gen *ngen;
	xchar gx,gy;
	unsigned gflag;		/* 037: trap type; 040: SEEN flag */
				/* 0100: ONCE only */
#define	TRAPTYPE	037
#define	SEEN	040
#define	ONCE	0100
};
extern struct gen *fgold, *ftrap;
#define newgen()	(struct gen *) alloc(sizeof(struct gen))
#endif /* _DEF_GEN_H_ */
