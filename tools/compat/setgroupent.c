/*	$NetBSD: setgroupent.c,v 1.2 2002/01/31 19:23:14 tv Exp $	*/

#include "config.h"

#if !HAVE_SETGROUPENT
#include <grp.h>

int setgroupent(int stayopen) {
	setgrent();
	return 1;
}
#endif
