/*	$NetBSD: setpassent.c,v 1.1 2002/01/29 10:20:32 tv Exp $	*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !HAVE_SETPASSENT
#include <pwd.h>

int setpassent(int stayopen) {
	setpwent();
	return 1;
}
#endif
