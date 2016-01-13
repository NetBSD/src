/*	$NetBSD: quotesys.h,v 1.1.1.1 2016/01/13 03:15:30 christos Exp $	*/

/* quotesys.h -- declarations for quoting system arguments */

#if defined __STDC__ || __GNUC__
# define __QUOTESYS_P(args) args
#else
# define __QUOTESYS_P(args) ()
#endif

size_t quote_system_arg __QUOTESYS_P ((char *, char const *));
