/*	$NetBSD: ipt.h,v 1.7 2012/01/30 16:12:02 darrenr Exp $	*/

/*
 * Copyright (C) 2007 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ipt.h,v 2.9.2.1 2012/01/26 05:29:13 darrenr Exp
 */

#ifndef	__IPT_H__
#define	__IPT_H__

#ifndef	__P
# define P_DEF
# ifdef	__STDC__
#  define	__P(x) x
# else
#  define	__P(x) ()
# endif
#endif

#include <fcntl.h>


struct	ipread	{
	int	(*r_open) __P((char *));
	int	(*r_close) __P((void));
	int	(*r_readip) __P((mb_t *, char **, int *));
	int	r_flags;
};

#define	R_DO_CKSUM	0x01

#ifdef P_DEF
# undef	__P
# undef	P_DEF
#endif

#endif /* __IPT_H__ */
