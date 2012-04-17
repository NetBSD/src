/*	$NetBSD: kmem.h,v 1.4.56.1 2012/04/17 00:02:24 yamt Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 * Id: kmem.h,v 2.5 2002/08/21 22:57:36 darrenr Exp
 */

#ifndef	__KMEM_H__
#define	__KMEM_H__

extern	int	openkmem(char *, char *);
extern	int	kmemcpy(char *, long, int);
extern	int	kstrncpy(char *, long, int);

#if defined(__NetBSD__) || defined(__OpenBSD)
# include <paths.h>
#endif

#ifdef _PATH_KMEM
# define	KMEM	_PATH_KMEM
#else
# define	KMEM	"/dev/kmem"
#endif

#endif /* __KMEM_H__ */
