/*	$NetBSD: null.h,v 1.4.30.1 2004/08/03 10:56:29 skrll Exp $	*/

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
