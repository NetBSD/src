/*	$NetBSD: setpassent.c,v 1.3 2002/02/26 22:29:39 tv Exp $	*/

#include "config.h"

#if !HAVE_SETPASSENT || !HAVE_DECL_SETPASSENT
#include <pwd.h>

int setpassent(int stayopen) {
	setpwent();
	return 1;
}
#endif
