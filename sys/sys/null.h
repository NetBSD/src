/*	$NetBSD: null.h,v 1.4.30.2 2004/09/18 14:56:30 skrll Exp $	*/

#ifndef	NULL
#if !defined(__GNUG__) || __GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 90)
#if !defined(__cplusplus)
#define	NULL	(void *)0
#else
#define	NULL	0
#endif /* !__cplusplus */
#else
#define	NULL	__null
#endif
#endif
