/*	$NetBSD: platid.h,v 1.3 2004/08/06 18:33:09 uch Exp $	*/

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
