/*	$NetBSD: crtend.c,v 1.5 1998/05/06 20:45:54 ross Exp $	*/

#include <sys/cdefs.h>
#include "crt.h"

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */
