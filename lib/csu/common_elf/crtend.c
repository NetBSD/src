/*	$NetBSD: crtend.c,v 1.6.10.1 2001/12/09 17:11:17 he Exp $	*/

#include <sys/cdefs.h>
#include "dot_init.h"

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((__unused__))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */

#ifdef DWARF2_EH
static unsigned int __FRAME_END__[]
    __attribute__((__unused__))
    __attribute__((section(".eh_frame"))) = { 0 };
#endif

MD_INIT_SECTION_EPILOGUE;

MD_FINI_SECTION_EPILOGUE;
