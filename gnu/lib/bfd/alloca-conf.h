/*	$NetBSD: alloca-conf.h,v 1.1.1.1 1997/09/26 02:38:49 gwr Exp $	*/

/* NetBSD configuration for alloca.  */

#ifdef __GNUC__
# define alloca __builtin_alloca
#else /* __GNUC__ */
# ifdef __STDC__
PTR alloca (size_t);
# else
PTR alloca ();			/* must agree with functions.def */
# endif
#endif /* __GNUC__ */
