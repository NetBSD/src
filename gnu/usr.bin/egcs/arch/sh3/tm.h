/*	$NetBSD: tm.h,v 1.3 2001/02/16 21:11:13 msaitoh Exp $	*/

#ifdef DEFAULT_ELF
#ifdef TARGET_LITTLE_ENDIAN_DEFAULT
#include "sh/netbsdelelf.h"
#else
#include "sh/netbsdelf.h"
#endif
#else
#ifdef TARGET_LITTLE_ENDIAN_DEFAULT
#include "sh/netbsdelcoff.h"
#else
#include "sh/netbsdcoff.h"
#endif
#endif
