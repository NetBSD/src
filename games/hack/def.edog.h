/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$Id: def.edog.h,v 1.2 1993/08/02 17:16:39 mycroft Exp $
 */

struct edog {
	long hungrytime;	/* at this time dog gets hungry */
	long eattime;		/* dog is eating */
	long droptime;		/* moment dog dropped object */
	unsigned dropdist;		/* dist of drpped obj from @ */
	unsigned apport;		/* amount of training */
	long whistletime;		/* last time he whistled */
};
#define	EDOG(mp)	((struct edog *)(&(mp->mextra[0])))
