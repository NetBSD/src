/*	$NetBSD: ntp_malloc.h,v 1.2 1998/01/09 06:06:10 perry Exp $	*/

/*
 * Define malloc and friends.
 */
#ifndef  _ntp_malloc_h
#define  _ntp_malloc_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#else /* HAVE_STDLIB_H */
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif /* HAVE_STDLIB_H */

#endif /* _ntp_malloc_h */
