/*	$NetBSD: platid.h,v 1.2.16.1 2004/08/12 11:41:11 skrll Exp $	*/

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
