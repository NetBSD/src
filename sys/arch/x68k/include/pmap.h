/*	$NetBSD: pmap.h,v 1.30 2025/11/24 16:58:01 thorpej Exp $	*/

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */
