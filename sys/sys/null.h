/*	$NetBSD: null.h,v 1.4.6.2 2000/11/20 18:11:33 bouyer Exp $	*/

#ifndef	NULL
#if !defined(__GNUG__) || __GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 90)
#if defined(__AUDIT__) && !defined(__cplusplus)
/*
 * This will cause GCC to emit a warning if you attempt to use NULL
 * with some non-pointer object, return value, etc.
 */
#define	NULL	(void *)0
#else
#define	NULL	0
#endif /* __AUDIT__ && !__cplusplus */
#else
#define	NULL	__null
#endif
#endif
