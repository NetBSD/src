/*	$NetBSD: opts.h,v 1.1.1.1.56.1 2012/04/17 00:02:24 yamt Exp $	*/

/*
 * Copyright (C) 2000 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: opts.h,v 2.12 2003/08/14 14:24:27 darrenr Exp
 */

#ifndef	__OPTS_H__
#define	__OPTS_H__

#ifndef	SOLARIS
#define	SOLARIS	(defined(sun) && (defined(__svr4__) || defined(__SVR4)))
#endif
#define	OPT_REMOVE	0x000001
#define	OPT_DEBUG	0x000002
#define	OPT_AUTHSTATS	0x000004
#define	OPT_RAW		0x000008
#define	OPT_LOG		0x000010
#define	OPT_SHOWLIST	0x000020
#define	OPT_VERBOSE	0x000040
#define	OPT_DONOTHING	0x000080
#define	OPT_HITS	0x000100
#define	OPT_BRIEF	0x000200
#define	OPT_ACCNT	0x000400
#define	OPT_FRSTATES	0x000800
#define	OPT_SHOWLINENO	0x001000
#define	OPT_PRINTFR	0x002000
#define	OPT_OUTQUE	FR_OUTQUE	/* 0x4000 */
#define	OPT_INQUE	FR_INQUE	/* 0x8000 */
#define	OPT_ZERORULEST	0x010000
#define	OPT_SAVEOUT	0x020000
#define	OPT_IPSTATES	0x040000
#define	OPT_INACTIVE	0x080000
#define	OPT_NAT		0x100000
#define	OPT_GROUPS	0x200000
#define	OPT_STATETOP	0x400000
#define	OPT_FLUSH	0x800000
#define	OPT_CLEAR	0x1000000
#define	OPT_HEX		0x2000000
#define	OPT_ASCII	0x4000000
#define	OPT_NORESOLVE	0x8000000

#define	OPT_STAT	OPT_FRSTATES
#define	OPT_LIST	OPT_SHOWLIST


#if defined(sun) && !SOLARIS
# define	STRERROR(x)	sys_errlist[x]
extern	char	*sys_errlist[];
#else
# define	STRERROR(x)	strerror(x)
#endif

extern	int	opts;

#endif /* __OPTS_H__ */
