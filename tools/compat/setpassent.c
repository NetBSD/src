/*	$NetBSD: setpassent.c,v 1.2 2002/01/31 19:23:14 tv Exp $	*/

#include "config.h"

#if !HAVE_SETPASSENT
#include <pwd.h>

int setpassent(int stayopen) {
	setpwent();
	return 1;
}
#endif
