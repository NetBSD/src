/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "cc_blitter.h"

void
cc_init_blitter (void)
{
}

#if defined (AMIGA_TEST)
void
cc_deinit_blitter (void)
{
}
#endif

/* test twice to cover blitter bugs if BLTDONE (BUSY) is set it is not done. */
int
is_blitter_busy (void)
{
    volatile u_word bb = (custom.dmaconr & DMAF_BLTDONE);
    if ((custom.dmaconr & DMAF_BLTDONE) || bb) {
	return (1);
    }
    return (0);
}

void inline
wait_blit (void)
{
    /* V40 state this covers all blitter bugs. */
    while (is_blitter_busy ()) {
	;
    }
}
				  
void
blitter_handler (void)
{
    custom.intreq = INTF_BLIT;
}


void inline
do_blit (u_word size)
{
    custom.bltsize = size;
}

void inline
set_blitter_control (u_word con0, u_word con1)
{
    custom.bltcon0 = con0;
    custom.bltcon1 = con1;
}

void inline
set_blitter_mods (u_word a, u_word b,
		  u_word c, u_word d)
{
    custom.bltamod = a;
    custom.bltbmod = b;
    custom.bltcmod = c;
    custom.bltdmod = d;
}

void inline
set_blitter_masks (u_word fm, u_word lm)
{
    custom.bltafwm = fm;
    custom.bltalwm = lm;
}

void inline
set_blitter_data (u_word da, u_word db, u_word dc)
{
    custom.bltadat = da;
    custom.bltbdat = db;
    custom.bltcdat = dc;
}

void inline
set_blitter_pointers ( void *a, void *b, void *c, void *d)
{
    custom.bltapt = a;
    custom.bltbpt = b;
    custom.bltcpt = c;
    custom.bltdpt = d;
}

