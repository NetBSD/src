/*	$NetBSD: def.permonst.h,v 1.5 2001/03/25 20:43:58 jsm Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 */
#ifndef _DEF_PERMONST_H_
#define _DEF_PERMONST_H_
struct permonst {
	const char *mname;
	char mlet;
	schar mlevel,mmove,ac,damn,damd;
	unsigned pxlth;
};

extern const struct permonst mons[];
#define PM_ACID_BLOB	&mons[7]
#define	PM_ZOMBIE	&mons[13]
#define	PM_PIERCER	&mons[17]
#define	PM_KILLER_BEE	&mons[26]
#define	PM_WRAITH	&mons[33]
#define	PM_MIMIC	&mons[37]
#define	PM_VAMPIRE	&mons[43]
#define	PM_CHAMELEON	&mons[47]
#define	PM_DEMON	&mons[54]
#define	PM_MINOTAUR	&mons[55]	/* last in mons array */
#define	PM_SHK		&mons[56]	/* very last */
#define	PM_GHOST	&pm_ghost
#define	PM_EEL		&pm_eel
#define	PM_WIZARD	&pm_wizard
#define	CMNUM		55		/* number of common monsters */

#endif /* _DEF_PERMONST_H_ */
