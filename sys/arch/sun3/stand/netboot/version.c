/*	$NetBSD: version.c,v 1.3 1996/01/29 23:54:16 gwr Exp $ */

/*
 *	NOTE ANY CHANGES YOU MAKE TO THE BOOTBLOCKS HERE.
 */

#ifdef	DATECODE
char *version = DATECODE;
#else
char *version = "$Revision: 1.3 $";
#endif
