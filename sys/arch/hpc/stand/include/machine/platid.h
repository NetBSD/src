/*	$NetBSD: platid.h,v 1.1 2001/02/09 18:35:30 uch Exp $	*/

#ifdef MIPS
#define hpcmips
#endif
#ifdef SHx
#define hpcsh
#endif
#ifdef ARM
#define hpcarm
#endif
#include "../../../hpc/platid.h"
