/*	$NetBSD: hard-locale.h,v 1.1.1.1 2003/01/26 00:43:14 wiz Exp $	*/

#ifndef HARD_LOCALE_H_
# define HARD_LOCALE_H_ 1

# if HAVE_CONFIG_H
#  include <config.h>
# endif

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

int hard_locale PARAMS ((int));

#endif /* HARD_LOCALE_H_ */
