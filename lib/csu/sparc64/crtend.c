/*	$NetBSD: crtend.c,v 1.1 1998/09/11 03:36:24 eeh Exp $	*/

#include <sys/cdefs.h>

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */
