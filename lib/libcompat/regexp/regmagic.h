/*	$NetBSD: regmagic.h,v 1.3 1997/10/09 10:21:24 lukem Exp $ */

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define	MAGIC	0234
