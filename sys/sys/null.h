/*	$NetBSD: null.h,v 1.3 2000/05/19 05:04:32 thorpej Exp $	*/

#ifndef	NULL
#if !defined(__GNUG__) || __GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 90)
#ifdef __AUDIT__
/*
 * This will cause GCC to emit a warning if you attempt to use NULL
 * with some non-pointer object, return value, etc.
 */
#define	NULL	(void *)0
#else
#define	NULL	0
#endif
#else
#define	NULL	__null
#endif
#endif
