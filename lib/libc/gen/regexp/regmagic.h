/*	$Id: regmagic.h,v 1.2 1993/08/02 17:49:32 mycroft Exp $ */

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define	MAGIC	0234
