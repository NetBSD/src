/*	$NetBSD: def.edog.h,v 1.4 1997/10/19 16:56:51 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#ifndef _DEF_EDOG_H_
#define _DEF_EDOG_H_
struct edog {
	long hungrytime;	/* at this time dog gets hungry */
	long eattime;		/* dog is eating */
	long droptime;		/* moment dog dropped object */
	unsigned dropdist;		/* dist of drpped obj from @ */
	unsigned apport;		/* amount of training */
	long whistletime;		/* last time he whistled */
};
#define	EDOG(mp)	((struct edog *)(&(mp->mextra[0])))

#endif /* _DEF_EDOG_H_ */
