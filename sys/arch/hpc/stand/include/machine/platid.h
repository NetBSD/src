/*	$NetBSD: platid.h,v 1.1.2.2 2001/02/11 19:10:19 bouyer Exp $	*/

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
