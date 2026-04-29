/*	$NetBSD: pmap.h,v 1.31 2026/04/29 12:33:04 thorpej Exp $	*/

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#define VM_KERNEL_PT_PAGES      ((vsize_t)4)
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */
