/*	$NetBSD: setgroupent.c,v 1.3 2002/02/26 22:29:39 tv Exp $	*/

#include "config.h"

#if !HAVE_SETGROUPENT || !HAVE_DECL_SETGROUPENT
#include <grp.h>

int setgroupent(int stayopen) {
	setgrent();
	return 1;
}
#endif
