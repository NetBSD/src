/*	$NetBSD: platid.h,v 1.4 2005/12/11 12:17:29 christos Exp $	*/

#ifdef MIPS
#define	hpcmips
#endif
#ifdef SHx
#define	hpcsh
#endif
#ifdef ARM
#define	hpcarm
#endif
#include "../../../include/platid.h"
