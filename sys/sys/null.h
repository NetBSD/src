/*	$NetBSD: null.h,v 1.2 2000/05/19 04:37:20 thorpej Exp $	*/

#ifndef	NULL
#if !defined(__GNUG__) || __GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 90)
#ifdef __AUDIT__
#define	NULL	(void *)0
#else
#define	NULL	0
#endif
#else
#define	NULL	__null
#endif
#endif
