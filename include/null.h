/*	$NetBSD: null.h,v 1.1 1999/12/22 21:26:17 kleink Exp $	*/

#ifndef	NULL
#if !defined(__GNUG__) || __GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 90)
#define	NULL	0
#else
#define	NULL	__null
#endif
#endif
