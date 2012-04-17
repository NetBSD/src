/*	$NetBSD: ipt.h,v 1.6.42.1 2012/04/17 00:02:24 yamt Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ipt.h,v 2.6.4.2 2006/03/26 23:42:04 darrenr Exp
 */

#ifndef	__IPT_H__
#define	__IPT_H__

#include <fcntl.h>


struct	ipread	{
	int	(*r_open)(char *);
	int	(*r_close)(void);
	int	(*r_readip)(char *, int, char **, int *);
	int	r_flags;
};

#define	R_DO_CKSUM	0x01

extern	void	debug(char *, ...);
extern	void	verbose(char *, ...);

#endif /* __IPT_H__ */
