/*	$NetBSD: quotesys.h,v 1.1.1.1 2003/01/26 00:43:14 wiz Exp $	*/

/* quotesys.h -- declarations for quoting system arguments */

#if defined __STDC__ || __GNUC__
# define __QUOTESYS_P(args) args
#else
# define __QUOTESYS_P(args) ()
#endif

size_t quote_system_arg __QUOTESYS_P ((char *, char const *));
