/*	$NetBSD: hack.mfndpos.h,v 1.4 1997/10/19 16:58:19 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _HACK_MFNDPOS_H_
#define _HACK_MFNDPOS_H_
#define	ALLOW_TRAPS	0777
#define	ALLOW_U		01000
#define	ALLOW_M		02000
#define	ALLOW_TM	04000
#define	ALLOW_ALL	(ALLOW_U | ALLOW_M | ALLOW_TM | ALLOW_TRAPS)
#define	ALLOW_SSM	010000
#define	ALLOW_ROCK	020000
#define	NOTONL		040000
#define	NOGARLIC	0100000
#endif /* _HACK_MFNDPOS_H_ */
