/*	$NetBSD: crtend.c,v 1.1.2.1 1998/11/07 00:19:27 cgd Exp $	*/

#include <sys/cdefs.h>

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */
