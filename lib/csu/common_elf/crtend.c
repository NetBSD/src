/*	$NetBSD: crtend.c,v 1.7 2001/05/11 22:44:15 ross Exp $	*/

#include <sys/cdefs.h>
#include "dot_init.h"

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */

MD_INIT_SECTION_EPILOGUE;

MD_FINI_SECTION_EPILOGUE;
