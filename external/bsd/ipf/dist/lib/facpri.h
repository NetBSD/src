/*	$NetBSD: facpri.h,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#ifndef	__FACPRI_H__
#define	__FACPRI_H__

#ifndef	__P
# define P_DEF
# ifdef	__STDC__
#  define	__P(x) x
# else
#  define	__P(x) ()
# endif
#endif

extern	char	*fac_toname __P((int));
extern	int	fac_findname __P((char *));

extern	char	*pri_toname __P((int));
extern	int	pri_findname __P((char *));

#ifdef P_DEF
# undef	__P
# undef	P_DEF
#endif

#if LOG_CRON == (9<<3)
# define	LOG_CRON1	LOG_CRON
# define	LOG_CRON2	(15<<3)
#endif
#if LOG_CRON == (15<<3)
# define	LOG_CRON1	(9<<3)
# define	LOG_CRON2	LOG_CRON
#endif

#endif /* __FACPRI_H__ */
