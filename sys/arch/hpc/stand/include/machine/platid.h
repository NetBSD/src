/*	$NetBSD: platid.h,v 1.1.6.1 2002/02/11 20:08:00 jdolecek Exp $	*/

#ifdef MIPS
#define hpcmips
#endif
#ifdef SHx
#define hpcsh
#endif
#ifdef ARM
#define hpcarm
#endif
#include "../../../include/platid.h"
