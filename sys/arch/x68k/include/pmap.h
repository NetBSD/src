/*	$NetBSD: pmap.h,v 1.27.234.1 2026/11/04 23:21:30 perseant Exp $	*/

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#define VM_KERNEL_PT_PAGES      ((vsize_t)4)
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */
