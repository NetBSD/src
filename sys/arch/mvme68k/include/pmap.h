/*	$NetBSD: pmap.h,v 1.28 2025/11/17 06:21:59 thorpej Exp $	*/

#ifndef _MVME68K_PMAP_H_
#define _MVME68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#define	__HAVE_MACHINE_BOOTMAP

#endif /* _MVME68K_PMAP_H_ */
