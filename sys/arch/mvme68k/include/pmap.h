/*	$NetBSD: pmap.h,v 1.27.234.1 2026/11/04 23:21:01 perseant Exp $	*/

#ifndef _MVME68K_PMAP_H_
#define _MVME68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#endif /* _MVME68K_PMAP_H_ */
