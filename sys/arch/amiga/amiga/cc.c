/* Copyright (c) 1993 by Integral Insights Co.  All Rights Reserved */

/* -------------------------------------------------- 
 |  NAME
 |    cc
 |  PURPOSE
 |    main file for custom chip interface of kernel.
 |  NOTES
 |    
 |  HISTORY
 |         chopps - Oct 22, 1993: Created.
 +--------------------------------------------------- */

#include "cc.h"
#include "cc_copper.h"
#include "cc_blitter.h"
#include "cc_chipmem.h"
#include "cc_audio.h"

#if ! defined (AMIGA_TEST)
void
custom_chips_init (void)
{
    cc_init_chipmem ();
    cc_init_vbl ();
    cc_init_audio ();
    cc_init_blitter ();
    cc_init_copper ();
}

#else /* AMIGA_TEST */

#include <stdarg.h>
#include <stdio.h>

void
custom_chips_init (void)
{
    cc_init_copper ();
    cc_init_chipmem ();
    cc_init_vbl ();
    cc_init_blitter ();
}

void
custom_chips_deinit (void)
{
    cc_deinit_blitter ();
    cc_deinit_vbl ();
    cc_deinit_chipmem ();
    cc_deinit_copper ();
}

void amiga_test_panic (char *s,...)
{
    va_list ap;

    va_start (ap, s); {
	printf ("panic: ");
	vprintf (s, ap);
	printf ("\n");
    } va_end (ap);
    
    exit (0);
}

int nothing (void)
{
    return (0);
}
#endif
