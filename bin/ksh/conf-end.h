/*	$NetBSD: conf-end.h,v 1.7 2017/06/30 02:38:09 kamil Exp $	*/

/*
 * End of configuration stuff for PD ksh.
 *
 * RCSid: $NetBSD: conf-end.h,v 1.7 2017/06/30 02:38:09 kamil Exp $
 */

#if defined(EMACS) || defined(VI)
# define	EDIT
#else
# undef		EDIT
#endif

/* Editing implies history */
#if defined(EDIT) && !defined(HISTORY)
# define HISTORY
#endif /* EDIT */

#if defined(HISTORY) && (!defined(COMPLEX_HISTORY) || !defined(HAVE_FLOCK))
# undef COMPLEX_HISTORY
# define EASY_HISTORY			/* sjg's trivial history file */
#endif

#ifdef HAVE_GCC_FUNC_ATTR
# define GCC_FUNC_ATTR(x)	__attribute__((x))
# define GCC_FUNC_ATTR2(x,y)	__attribute__((x,y))
#else
# define GCC_FUNC_ATTR(x)
# define GCC_FUNC_ATTR2(x,y)
#endif /* HAVE_GCC_FUNC_ATTR */
