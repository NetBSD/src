/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: cc.c,v 1.2 1994/02/11 06:59:21 chopps Exp $
 */

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
