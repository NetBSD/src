/*	$NetBSD: crtend.c,v 1.1 1997/03/05 03:45:06 jonathan Exp $	*/

#ifndef ECOFF_COMPAT

#include <sys/cdefs.h>

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */

#endif /* !ECOFF_COMPAT */
