/*	$NetBSD: ipt.h,v 1.1.1.11 1998/11/22 14:21:48 mrg Exp $	*/

/*
 * Copyright (C) 1993-1998 by Darren Reed.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and due credit is given
 * to the original author and the contributors.
 * Id: ipt.h,v 2.0.2.7.2.1 1998/11/22 01:50:38 darrenr Exp 
 */

#ifndef	__IPT_H__
#define	__IPT_H__

#include <fcntl.h>
#ifdef	__STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif


struct	ipread	{
	int	(*r_open) __P((char *));
	int	(*r_close) __P((void));
	int	(*r_readip) __P((char *, int, char **, int *));
};

extern	void	debug __P((char *, ...));
extern	void	verbose __P((char *, ...));

#endif /* __IPT_H__ */
