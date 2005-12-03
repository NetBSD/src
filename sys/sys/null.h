/*	$NetBSD: null.h,v 1.7 2005/12/03 17:10:46 christos Exp $	*/

#ifndef _SYS_NULL_H_
#define _SYS_NULL_H_
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
#endif /* _SYS_NULL_H_ */
